#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p1[2], p2[2];//parent write, child write
  char buffer[] = {"U"};
  int len = sizeof(buffer);

  pipe(p1);
  pipe(p2);
  if(fork() == 0){
    close(p1[1]);
    close(p2[0]);
    if(read(p1[0], buffer, len) != len){
      fprintf(2, "a---->b,read error\n");
      exit(1);
    }
    int chil_pid = getpid();
    fprintf(1, "%d: received ping\n", chil_pid);
    if(write(p2[1],buffer, len) != len){
      fprintf(2, "a<----b, write error\n");
      exit(1);
    }
    exit(0);
  } else{
    close(p1[0]);
    close(p2[1]);

    if(write(p1[1], buffer, len) != len){
      fprintf(2, "a---->b,write error\n");
      exit(1);
    } 
    wait(0);  
    if(read(p2[0],buffer, len) != len){
      fprintf(2, "a<----b, read error\n");
      exit(1); 
    }
    int pid = getpid();
    fprintf(1, "%d: received pong\n", pid);
  }    
  exit(0);
}
