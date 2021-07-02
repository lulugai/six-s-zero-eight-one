#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int time;
  if(argc != 2){
  fprintf(2, "Error!Need 2 argument.\n");
  exit(1);
  }

  time = atoi(argv[1]);
  sleep(time);
  printf("(nothing happens for a little while)\n");
  exit(0);
}
