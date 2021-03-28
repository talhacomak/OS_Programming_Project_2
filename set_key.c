#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "vault_ioctl.h"

void swapp(int *xp, int *yp)  
{  
    int temp = *xp;  
    *xp = *yp;  
    *yp = temp;  
}

void swapCh(char *xp, char *yp)  
{  
    char temp = *xp;  
    *xp = *yp;  
    *yp = temp;  
}

void bubbleSort(char ar[], int arr[], int n)  
{  
    int i, j;
    for (i = 0; i < n-1; i++)
    for (j = 0; j < n-i-1; j++)
        if (ar[j] > ar[j+1]){
        	swapCh(&ar[j], &ar[j+1]);
            swapp(&arr[j], &arr[j+1]);
		}
} 
void bubbleSort2(char ar[], int arr[], int n)  
{  
    int i, j;
    for (i = 0; i < n-1; i++)
    for (j = 0; j < n-i-1; j++)
        if (arr[j] > arr[j+1]){
        	swapCh(&ar[j], &ar[j+1]);
            swapp(&arr[j], &arr[j+1]);
		}
} 

int main(int argc, char *argv[]){
    int i;
    int index[100];
    
    char *p = argv[1];
    int size = (int)strlen(argv[1]);
    for(i=0; i<size; i++){
        index[i] = i;
    }

    bubbleSort(p, index, size);

    for(i=0; i<size; i++){
        p[i] = i+1;
    }

    bubbleSort2(p, index, size);
    
    int k = 0;
    int x = 1;
    for(i=size-1; 0<=i; i--){
        k += x*p[i];
        x*=10;
    }

    //printf("%d\n", k);

    int *res = (int*)malloc(sizeof(int));
    *res = k;
    

    int fd = open("/dev/vault0", O_RDWR);
    

	int status = ioctl(fd, IOC_SET_KEY, res);
    
	
    if(status == -1)
		perror("Error!");
		
	return status;
}
