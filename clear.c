#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "vault_ioctl.h"


int main(){

    int fd = open("/dev/vault0", O_RDWR);
    
    int status = ioctl(fd, IOC_RESET);
	
    if(status == -1)
		perror("Error!");
		
	return status;
}
