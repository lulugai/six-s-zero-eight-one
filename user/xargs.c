#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
    char charac;
    char argv_arr[MAXARG];
    int offset=0;
    int i;

    for(i=0; i+1 < argc; i++){
        argv[i] = argv[i+1]; 
        // printf("argv[%d]=%s\n", i, argv[i]);
    }
    while(read(0, &charac, sizeof(charac)) > 0){
        if(charac == '\n'){
            argv[i] = argv_arr;
            // printf("argv[%d]=%s\n", i, argv[i]);
            // argv[i+1] = 0;
            int pid = fork();
            if(pid == 0){
                exec(argv[0], &argv[0]);
                exit(1);//error    
            }else{
                wait(0);
            }

            memset(argv_arr, 0x00, sizeof(argv_arr));
            offset = 0;
        }else{           //!!!!!!!
            argv_arr[offset++] = charac;  
        }
    }
   exit(0);
}