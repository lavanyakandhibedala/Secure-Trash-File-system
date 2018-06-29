#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <unistd.h>   
#include <sys/ioctl.h>     
#include "sgctl.h"


int main(int argc, char * const argv[])
{
    int f_desc, return_val, u_opt;
    char *temp, *f_path="/mnt/sgfs/.sg/sd.txt";
    while( (u_opt = getopt ( argc , argv , "u" )  ) != -1)
        {
            switch( u_opt )
            {
                case 'u': goto udel;
                          break;
                case '?': printf("\nERROR : Invalid option\n");
                          goto out;
                          break;
                         
                default : abort();

            }
        }

udel:
    if (argc > 3 )
    {
         printf("\nERROR : Too many arguments\n ");
         exit(0);
    }
    if (argc < 3 )
    {
         printf("\nERROR : missing arguments passed\n");
         exit(0);
    }

    f_path = argv[2];
    
    printf("\n@@@@@@@ file path : %s \n",f_path);
    if( f_path == NULL)
          printf("\nERROR : file name missing");
    f_desc = open (f_path, O_RDWR);
    if (f_desc < 0)
        printf("\nERROR in opening the file \n");
    return_val = ioctl(f_desc, SGCTL_IOCTL_UNDELETE, temp); 
    close(f_desc);
out:
    return 0;
}
