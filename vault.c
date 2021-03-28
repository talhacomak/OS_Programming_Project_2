#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <asm/switch_to.h>		/* cli(), *_flags */
#include <linux/uaccess.h>	/* copy_*_user */

#include "vault_ioctl.h"

#define VAULT_MAJOR 0
#define VAULT_NR_DEVS 4
#define VAULT_QUANTUM 4000
#define VAULT_QSET 1000
#define VAULT_KEY 1234 // we store the key as integer instead of char array

int key = VAULT_KEY;
int vault_major = VAULT_MAJOR;
int vault_minor = 0;
int vault_nr_devs = VAULT_NR_DEVS;
int vault_quantum = VAULT_QUANTUM;
int vault_qset = VAULT_QSET;
int vault_echo = 0;
int vault_switch = 0;

module_param(vault_major, int, S_IRUGO);
module_param(vault_minor, int, S_IRUGO);
module_param(vault_nr_devs, int, S_IRUGO);
module_param(vault_quantum, int, S_IRUGO);
module_param(vault_qset, int, S_IRUGO);

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

struct vault_dev {
    char **data;
    int quantum;
    int qset;
    unsigned long size;
    struct semaphore sem;
    struct cdev cdev;
};

struct vault_dev *vault_devices;


int vault_trim(struct vault_dev *dev)
{
    int i;

    if (dev->data) {
        for (i = 0; i < dev->qset; i++) {
            if (dev->data[i])
                kfree(dev->data[i]);
        }
        kfree(dev->data);
    }
    dev->data = NULL;
    dev->quantum = vault_quantum;
    dev->qset = vault_qset;
    dev->size = 0;
    return 0;
}


int vault_open(struct inode *inode, struct file *filp)
{
    struct vault_dev *dev;

    dev = container_of(inode->i_cdev, struct vault_dev, cdev);
    filp->private_data = dev;

    /* trim the device if open was write-only */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
        vault_trim(dev);
        up(&dev->sem);
    }
    return 0;
}


int vault_release(struct inode *inode, struct file *filp)
{
    return 0;
}

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


ssize_t vault_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos)
{
    struct vault_dev *dev = filp->private_data;
    int quantum = dev->quantum;
    int s_pos, q_pos, i, j;
    int k = key;
    int key_size = 0;
    ssize_t retval = 0;
    char* pointer;
    char x[100];
    int y[100];

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    if(vault_echo == 0){
        retval = -1;
        goto out;
    }

    if (*f_pos >= dev->size)
        goto out;
    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;

    s_pos = (long) *f_pos / quantum;
    q_pos = (long) *f_pos % quantum;

    if (dev->data == NULL || ! dev->data[s_pos])
        goto out;

    /* read only up to the end of this quantum */
    if (count > quantum - q_pos)
        count = quantum - q_pos;
    
    while(k > 0){ // finding the key size
        k /= 10;
        key_size++;
    }

    k = key;
    i = key_size-1;
    while(k > 0){       // converting key to char array
        x[i] = (char) (k%10);
        k /= 10;
        i--;
    }
    
	for(i=0; i<key_size; i++){ // array y for indexing according to key
		y[i] = i;
	}
    
	bubbleSort(x, y, key_size);

    k = q_pos;
    j = 0;
    for(i=0; i<count-1; i++){
        x[i] =  *(dev->data[s_pos] + k + y[j]);
        if(j == key_size-1){ // for divide input by key_size
            k += key_size;
            j = -1;
        }
        j++;
    }


    //creating temp buffer
    pointer = kmalloc((count)*sizeof(char), GFP_KERNEL);

    for(i=0; i<count-1; i++){ // ****
        *(pointer + i) = x[i];
    }
    *(pointer + count-1) = '\n';

    if (copy_to_user(buf, pointer, count)) {
        retval = -EFAULT;
        goto out;
    }
    
    *f_pos += count;
    retval = count;

  out:
    vault_switch = 0;
    up(&dev->sem);
    return retval;
}


ssize_t vault_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
    struct vault_dev *dev = filp->private_data;
    int quantum = dev->quantum, qset = dev->qset;
    int s_pos, q_pos, k = key, i, j, key_size = 0;
    ssize_t retval = -ENOMEM;
    char* pointer;
    char x[100];
    int y[100];

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    if(vault_switch == 1){ // write control
        retval = -1;
        goto out;
    }

    if (*f_pos >= quantum * qset) {
        retval = 0;
        goto out;
    }

    s_pos = (long) *f_pos / quantum;
    q_pos = (long) *f_pos % quantum;

    if (!dev->data) {
        dev->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
        if (!dev->data)
            goto out;
        memset(dev->data, 0, qset * sizeof(char *));
    }
    if (!dev->data[s_pos]) {
        dev->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
        if (!dev->data[s_pos])
            goto out;
    }
    /* write only up to the end of this quantum */
    if (count > quantum - q_pos)
        count = quantum - q_pos;   

    while(k > 0){
        k /= 10;
        key_size++;
    }

    k = key;
    i = key_size-1;
    
    while(k > 0){   // key to int array, for indexing
        y[i] = k%10 - 1;
        k /= 10;
        i--;
    }

    k = 0;
    if((count-1)%key_size != 0) k = key_size - (count-1)%key_size;

    //creating temp buffer. ecrypt it to writo to buffer.
    pointer = kmalloc((count+k)*sizeof(char), GFP_KERNEL);
    if (copy_from_user(pointer, buf, count)) {
        retval = -EFAULT;
        goto out;
    }
    /*for(i=0; i<count; i++){
        printk("%c", pointer[i]);
    }*/

    for(i=0; i<k; i++){ // filling empty cells
        *(pointer+count+i-1) = '0';
    }

    count+=k;

    k = 0;
    j = 0;
    for(i=0; i<count-1; i++){
        x[i] =  *(pointer + k + y[j]);
        if(j == key_size-1){ // to devide the input by key
            k += key_size;
            j = -1;
        }
        j++;
    }

    vault_echo = 1;
    vault_switch = 1;

    x[count-1] = '\n';

    for(i=0; i<count; i++){ // writing to device
        *(dev->data[s_pos] + q_pos + i) = x[i];
    }
    

    *f_pos += count;
    retval = count;

    /* update the size */
    if (dev->size < *f_pos)
        dev->size = *f_pos;

  out:
    up(&dev->sem);
    return retval;
}


long vault_ioctl(struct file *filp,
                unsigned int cmd, unsigned long arg)
{

	int err = 0;
	int retval = 0;
    struct vault_dev *dev = filp->private_data;

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != VAULT_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > VAULT_IOC_MAXNR) return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd) {

      case IOC_SET_KEY:
        if (!capable(CAP_SYS_ADMIN))
            return -EPERM;
        retval = __get_user(key, (int __user *)arg);
        printk("%d", key);
        break;
    
      case IOC_RESET:
        vault_trim(dev);
        break;

	  default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	return retval;
}


struct file_operations vault_fops = {
    .owner =    THIS_MODULE,
    .read =     vault_read,
    .write =    vault_write,
    .unlocked_ioctl =    vault_ioctl,
    .open =     vault_open,
    .release =  vault_release,
};


void vault_cleanup_module(void)
{
    int i;
    dev_t devno = MKDEV(vault_major, vault_minor);

    if (vault_devices) {
        for (i = 0; i < vault_nr_devs; i++) {
            vault_trim(vault_devices + i);
            cdev_del(&vault_devices[i].cdev);
        }
    kfree(vault_devices);
    }

    unregister_chrdev_region(devno, vault_nr_devs);
}


int vault_init_module(void)
{
    int result, i;
    int err;
    dev_t devno = 0;
    struct vault_dev *dev;

    if (vault_major) {
        devno = MKDEV(vault_major, vault_minor);
        result = register_chrdev_region(devno, vault_nr_devs, "vault");
    } else {
        result = alloc_chrdev_region(&devno, vault_minor, vault_nr_devs,
                                     "vault");
        vault_major = MAJOR(devno);
    }
    if (result < 0) {
        printk(KERN_WARNING "vault: can't get major %d\n", vault_major);
        return result;
    }

    vault_devices = kmalloc(vault_nr_devs * sizeof(struct vault_dev),
                            GFP_KERNEL);
    if (!vault_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(vault_devices, 0, vault_nr_devs * sizeof(struct vault_dev));

    /* Initialize each device. */
    for (i = 0; i < vault_nr_devs; i++) {
        dev = &vault_devices[i];
        dev->quantum = vault_quantum;
        dev->qset = vault_qset;
        sema_init(&dev->sem,1);

        devno = MKDEV(vault_major, vault_minor + i);
        cdev_init(&dev->cdev, &vault_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &vault_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        if (err)
            printk(KERN_NOTICE "Error %d adding vault%d", err, i);
    }

    return 0; /* succeed */

  fail:
    vault_cleanup_module();
    return result;
}

module_init(vault_init_module);
module_exit(vault_cleanup_module);
