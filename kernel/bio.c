// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

int
get_hash_num(uint blockno)
{
  return blockno % NBUCKET;
}

struct {
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];
  struct spinlock steal_lock;

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  char buf[NBUCKET][20];
  for(int i=0; i<NBUCKET; i++){
    snprintf(buf[i], 20, "bcache.bucket%d", i);
    initlock(&bcache.lock[i], (char *)buf[i]);
  }

  initlock(&bcache.steal_lock, "bcache");

  for(int i=0; i<NBUCKET; i++){
        // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

  //avg distribute buf for bucket!!!
  int i;
  for(b = bcache.buf, i=0; b < bcache.buf+NBUF; b++, i = (i + 1)%NBUCKET){
    initsleeplock(&b->lock, "buffer");
    b->next = bcache.head[i].next;
    b->prev = &bcache.head[i];
    
    bcache.head[i].next->prev = b;
    bcache.head[i].next = b;
    
  }

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int idx = get_hash_num(blockno);
  acquire(&bcache.lock[idx]);
  // Is the block already cached?
  for(b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[idx]);
      acquiresleep(&b->lock);//read write
      return b;
    }
  }  
  
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.on bucket[idx].
  for(b = bcache.head[idx].prev; b != &bcache.head[idx]; b = b->prev){
    if(b->refcnt == 0) {  // buffer not in use
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      release(&bcache.lock[idx]);
      acquiresleep(&b->lock);
      return b;
    } 
  } 
  release(&bcache.lock[idx]);

  acquire(&bcache.steal_lock);
  //steal buffer from other bucket.
  for(int i=0; i<NBUCKET; i++){
    acquire(&bcache.lock[i]);
    if(i != idx){   
      for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
        if(b->refcnt == 0) {  // buffer not in use
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;

          //delete b
          b->next->prev = b->prev;
          b->prev->next = b->next;
         
          //insert b
          b->next = bcache.head[idx].next;
          b->prev = &bcache.head[idx];
          bcache.head[idx].next->prev = b;
          bcache.head[idx].next = b;          
          release(&bcache.lock[i]);
          acquiresleep(&b->lock);
          release(&bcache.steal_lock);
          return b;
        }
      }
    }    
    release(&bcache.lock[i]);
  }
  release(&bcache.steal_lock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);//read from disk
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  // acquire(&bcache.steal_lock);
  releasesleep(&b->lock);

  int idx = get_hash_num(b->blockno);
  acquire(&bcache.lock[idx]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[idx].next;
    b->prev = &bcache.head[idx];
    bcache.head[idx].next->prev = b;
    bcache.head[idx].next = b;
  }
  release(&bcache.lock[idx]);
  // release(&bcache.steal_lock);
}

void
bpin(struct buf *b) {
  int idx = get_hash_num(b->blockno);
  acquire(&bcache.lock[idx]);
  b->refcnt++;
  release(&bcache.lock[idx]);
}

void
bunpin(struct buf *b) {
  int idx = get_hash_num(b->blockno);
  acquire(&bcache.lock[idx]);
  b->refcnt--;
  release(&bcache.lock[idx]);
}


