#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct sysinfo si;
  if(argc != 1){
    fprintf(2, "%s need no argument\n", argv[0]);
    exit(1);
  }
  sysinfo(&si);
  printf("free memory: %d, number used processes: %d\n", si.freemem, si.nproc);
  exit(0);
}
