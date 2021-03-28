#ifndef __VAULT_H
#define __VAULT_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

#define VAULT_IOC_MAGIC  'k'
#define IOC_SET_KEY   _IOW(VAULT_IOC_MAGIC, 0, int)
#define IOC_RESET   _IO(VAULT_IOC_MAGIC, 1)
#define VAULT_IOC_MAXNR 1

#endif
