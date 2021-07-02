#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define NULL    ((void *)0)

void primes(int *fd){
  int p, n, len;
  int fd1[2];

  if(read(fd[0], &p, sizeof(int)) <= 0){
    // printf("exit\n");
    close(fd[0]);
    exit(0);
  }
  printf("prime %d\n", p);
  pipe(fd1);
  if(fork() == 0){
    close(fd1[1]);
    primes(fd1);
  }else{
    close(fd1[0]);
    while((len = read(fd[0], &n, 4)) > 0 && n > 0){
      if(n%p != 0){
        write(fd1[1], &n, 4);
      }
    }
    close(fd1[1]);
    wait(0);
    exit(0);
  }
}

int
main(int argc, char *argv[])
{
  int i;
  int fd[2];

  pipe(fd);
  if(fork() == 0){
    close(fd[1]);
    primes(fd);
  }else{
    close(fd[0]);
    for(i=2; i <= 35; i++){
      write(fd[1], &i, 4);
    }
    close(fd[1]);
    wait(0);
  }
  exit(0);
}
