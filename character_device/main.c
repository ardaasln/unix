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
#include <linux/poll.h>
#include <asm/uaccess.h>
#include "memcache.h"		/* local definitions */

int memcache_major 		=   MEMCACHE_MAJOR;
int memcache_devs 		=   MEMCACHE_DEVS;	/* number of bare memcache devices */
int memcache_cache_size = 	MEMCACHE_CACHE_SIZE;
int memcache_data_size	= 	MEMCACHE_DATA_SIZE;

module_param(memcache_major, 		int, 0);
module_param(memcache_devs, 		int, 0);
module_param(memcache_cache_size, 	int, 0);
module_param(memcache_data_size,	int, 0);

MODULE_AUTHOR	("Arinc ELHAN");
MODULE_LICENSE	("Dual BSD/GPL");

struct 	memcache_dev *memcache_devices; /* allocated in memcache_init */

int 	memcache_trim	(struct memcache_dev *dev);
int 	spacefree		(struct memcache_dev *dev);
void 	memcache_cleanup(void);

static int id = 0;

/*
 * Open and close
 */
int memcache_open (struct inode *inode, struct file *filp)
{
	struct memcache_dev *dev; /* device information */
	/*  Find the device */
	dev = container_of(inode->i_cdev, struct memcache_dev, cdev);
	filp->private_data = dev;

	printk(KERN_INFO "MEMCACHE: Opening the MEMCACHE LKM\n");
    /* now trim to 0 the length of the device if open was write-only */
	
	if (dev->iseof)
	{
		return -1;
	}
	if (mutex_lock_interruptible(&dev->mutex))
	{
		mutex_unlock(&dev->mutex);
		return -ERESTARTSYS;
	}

	if (dev->data_buffer == NULL) {
		/* allocate the buffer */
		dev->data_buffer = kmalloc(memcache_data_size, GFP_KERNEL);
		if (!dev->data_buffer) 
		{
			mutex_unlock(&dev->mutex);
			return -ENOMEM;
		}
		dev->data_buffersize = memcache_data_size;
		dev->data_end = dev->data_buffer + dev->data_buffersize;
	}

	if (dev->cache_buffer == NULL) {
		/* allocate the buffer */
		dev->cache_buffer = kmalloc(memcache_cache_size, GFP_KERNEL);
		if (!dev->cache_buffer) 
		{
			mutex_unlock(&dev->mutex);
			return -ENOMEM;
		}
		dev->cache_buffersize = memcache_cache_size;
		dev->cache_end = dev->cache_buffer + dev->cache_buffersize;

		dev->rp = dev->wp = dev->data_buffer; /* rd and wr from the beginning */
		dev->size = 0;
		strcpy((char*)dev->cache_buffer, "");
		
		id++;
		dev->sayi = id;
	}

	/* use f_mode,not  f_flags: it's cleaner (fs/open.c tells why) */
	if (filp->f_mode & FMODE_READ)
		dev->nreaders++;
	if (filp->f_mode & FMODE_WRITE)
		dev->nwriters++;
	mutex_unlock(&dev->mutex);

	printk(KERN_INFO "MEMCACHE: Cache index is %s\n",(char*)dev->cache_buffer);
	printk(KERN_INFO "MEMCACHE: ID is %d\n",id);

	return nonseekable_open(inode, filp);
}

int memcache_release (struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "MEMCACHE: Finalizing the MEMCACHE LKM.\n");
	return 0;
}

/*
 * Data management: read and write
 */
ssize_t memcache_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct memcache_dev *dev = filp->private_data;

	printk(KERN_INFO "MEMCACHE: Reading from MEMCACHE LKM.\n");
	printk(KERN_INFO "MEMCACHE: ID is %d.\n", dev->sayi);

	if (mutex_lock_interruptible (&dev->mutex))
	{
		mutex_unlock (&dev->mutex);
		return -ERESTARTSYS;
	}

	printk(KERN_INFO "MEMCACHE: %p %p\n", dev->rp, dev->wp);

	if(dev->rp == dev->wp) 
	{ /* nothing to read */
		mutex_unlock (&dev->mutex);
		return 0;
	}

	/* ok, data is there, return something */
	if (dev->wp > dev->rp)
		count = min(count, (size_t)(dev->wp - dev->rp));
	else /* the write pointer has wrapped, return data up to dev->end */
		count = min(count, (size_t)(dev->data_end - dev->rp));

	if (copy_to_user(buf, dev->rp, count)) {
		mutex_unlock (&dev->mutex);
		return -EFAULT;
	}

	dev->rp += count;
	if (dev->rp == dev->data_end)
		dev->rp = dev->data_buffer; /* wrapped */
	mutex_unlock (&dev->mutex);

	printk(KERN_INFO "MEMCACHE: Did read %li bytes.\n", (long)count);
	printk(KERN_INFO "MEMCACHE: Buffer is %s\n", buf);
	printk(KERN_INFO "MEMCACHE: Cache index is %s\n", dev->cache_buffer);

	return count;
}

ssize_t memcache_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct memcache_dev *dev = filp->private_data;
	int result;

	printk(KERN_INFO "MEMCACHE: Writing to MEMCACHE LKM.\n");

	if (mutex_lock_interruptible(&dev->mutex))
	{
		mutex_unlock (&dev->mutex);
		return -ERESTARTSYS;
	}

	/* Make sure there's space to write */
	result = dev->size;
	printk(KERN_INFO "DEBUG3. %d\n", result);

	if (result >= dev->data_buffersize - 1)
	{
		printk(KERN_INFO "MEMCACHE: Buffer is full.");
		mutex_unlock (&dev->mutex);
		return -EFBIG; /* scull_getwritespace called mutex_unlock(&dev->mutex) */
	}

	if (dev->wp >= dev->rp)
		count = min(count, (size_t)(dev->data_end - dev->wp)); /* to end-of-buf */
	else /* the write pointer has wrapped, fill up to rp-1 */
		count = min(count, (size_t)(dev->rp - dev->wp - 1));

	if (copy_from_user(dev->wp, buf, count)) {
		mutex_unlock (&dev->mutex);
		return -EFAULT;
	}

	dev->wp += count;
	dev->size += count;
	if (dev->wp == dev->data_end)
	{
		dev->wp = dev->data_buffer; /* wrapped */
		dev->size = dev->data_buffersize;
	}
	mutex_unlock(&dev->mutex);
	
	printk(KERN_INFO "MEMCACHE: Did write %li bytes.\n", (long)count);
	printk(KERN_INFO "MEMCACHE: %p %p\n", dev->rp, dev->wp);
	printk(KERN_INFO "MEMCACHE: Cache index is %s\n", dev->cache_buffer);

	return count;
}

/*
 * The ioctl() implementation
 */
long memcache_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{

	int err = 0, ret = 0, i;

	struct memcache_dev *dev = filp->private_data; /* device information */

	/* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
	if (_IOC_TYPE(cmd) != MEMCACHE_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > MEMCACHE_IOC_MAXNR) return -ENOTTY;

	/*
	 * the type is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. Note that the type is user-oriented, while
	 * verify_area is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	switch(cmd) 
	{

	case MEMCACHE_IOCRESET:
		for (i = 0; i < memcache_devs; i++)
		{
			printk(KERN_INFO "MEMCACHE_IOCRESET: for loop of devices: %d\n", memcache_devices[i].sayi);
			//kfree(memcache_devices[i].cache_buffer);
			memcache_devices[i].cache_buffer = NULL;
			//kfree(memcache_devices[i].data_buffer);
			memcache_devices[i].data_buffer = NULL;

			memcache_devices[i].iseof = 1;
			memcache_devices[i].size = 0;
		}
		break;

	case MEMCACHE_IOCGTESTCACHE:
		for (i = 0; i < memcache_devs; i++)
		{
			printk(KERN_INFO "MEMCACHE_IOCGTESTCACHE: for loop of devices: %d\n", memcache_devices[i].sayi);
			if (strcmp((char*) arg, (char*) memcache_devices[i].cache_buffer) == 0)
			{
				return 0;
			}
		}
		return 1;
		break;

	case MEMCACHE_IOCGETCACHE:
		printk(KERN_INFO "MEMCACHE_IOCGETCACHE: %s\n", dev->cache_buffer);
		strcpy((char*)arg, (char*)dev->cache_buffer);
		break;

	case MEMCACHE_IOCSETCACHE:
		printk(KERN_INFO "MEMCACHE_IOCSETCACHE: %s\n", (char*)arg);
		strcpy((char*)dev->cache_buffer, (char*)arg);
		dev->rp -= dev->size;
		break;
	
	case MEMCACHE_IOCTRUNC:
		printk(KERN_INFO "MEMCACHE_IOCTRUNC\n");
		printk(KERN_INFO "MEMCACHE_IOCTRUNC: Before cache size is %zu.\n", 	strlen(dev->cache_buffer));
		printk(KERN_INFO "MEMCACHE_IOCTRUNC: Before cache is %s\n", 		(char*)dev->cache_buffer);
		printk(KERN_INFO "MEMCACHE_IOCTRUNC: Before buffer is %s\n", 		(char*)dev->data_buffer);
	
		dev->cache_buffer -= strlen(dev->cache_buffer);
		dev->data_buffer -= dev->size;
		dev->wp -= dev->size;
		dev->rp = dev->wp;
		dev->size = 0;
		
		strcpy((char*)dev->cache_buffer, "");
	
		printk(KERN_INFO "MEMCACHE_IOCTRUNC: Current cache size is %zu.\n", strlen(dev->cache_buffer));
		printk(KERN_INFO "MEMCACHE_IOCTRUNC: Current cache is %s\n", 		(char*)dev->cache_buffer);
		printk(KERN_INFO "MEMCACHE_IOCTRUNC: Current buffer is %s\n", 		(char*)dev->data_buffer);
		break;

	case MEMCACHE_IOCQBUFSIZE:
		printk(KERN_INFO "MEMCACHE_IOCQBUFSIZE: Current buffer size is %zu.\n", strlen(dev->data_buffer));
		return strlen(dev->data_buffer);
		break;

	default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}

	return ret;
}

/*
 * The "extended" operations
 */
loff_t memcache_llseek (struct file *filp, loff_t off, int whence)
{
	struct memcache_dev *dev = filp->private_data;
	long newpos;

	switch(whence) {
	case 0: /* SEEK_SET */
		newpos = off;
		break;

	case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		break;

	case 2: /* SEEK_END */
		newpos = dev->size + off;
		break;

	default: /* can't happen */
		return -EINVAL;
	}
	if (newpos<0) return -EINVAL;
	filp->f_pos = newpos;
	return newpos;
}

unsigned int memcache_poll(struct file *filp, poll_table *wait)
{
	return 0;
}

/*
 * The fops
 */
struct file_operations memcache_fops = {
	.owner 			=   THIS_MODULE,
	.llseek 		=   memcache_llseek,
	.read 			=	memcache_read,
	.write 			=   memcache_write,
	.unlocked_ioctl =   memcache_ioctl,
	.open 			=	memcache_open,
	.release 		=   memcache_release,
	.poll 			=	memcache_poll,
};

int memcache_trim(struct memcache_dev *dev)
{
	struct memcache_dev *next, *dptr;

	for (dptr = dev; dptr; dptr = next) 
	{ /* all the list items */
		if (dptr->cache_buffer != NULL)
		{		
			kfree(dptr->cache_buffer);
			dptr->cache_buffer = NULL;
		}
		if (dptr->data_buffer != NULL) 
		{
			kfree(dptr->data_buffer);
			dptr->data_buffer = NULL;
		}
		next=dptr->next;
		if (dptr != dev) kfree(dptr); /* all of them but the first */
	}
	dev->size = 0;
	dev->data_buffersize 	= 0;
	dev->cache_buffersize 	= 0;
	dev->next = NULL;
	return 0;
}

static void memcache_setup_cdev(struct memcache_dev *dev, int index)
{
	int err, devno = MKDEV(memcache_major, index);
    
	cdev_init(&dev->cdev, &memcache_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &memcache_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding memcache%d", err, index);
}

/*
 * Finally, the module stuff
 */
int memcache_init(void)
{
	int result, i;
	dev_t dev = MKDEV(memcache_major, 0);
	printk(KERN_INFO "MEMCACHE: Initializing the MEMCACHE LKM.\n");	
	/*
	 * Register your major, and accept a dynamic number.
	 */
	if (memcache_major)
		result = register_chrdev_region(dev, memcache_devs, "memcache");
	else 
	{
		result = alloc_chrdev_region(&dev, 0, memcache_devs, "memcache");
		memcache_major = MAJOR(dev);
	}
	if (result < 0)
		return result;

	
	/* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	memcache_devices = kmalloc(memcache_devs*sizeof (struct memcache_dev), GFP_KERNEL);
	if (!memcache_devices) 
	{
		result = -ENOMEM;
		goto fail_malloc;
	}
	memset(memcache_devices, 0, memcache_devs*sizeof (struct memcache_dev));
	for (i = 0; i < memcache_devs; i++) 
	{
		mutex_init (&memcache_devices[i].mutex);
		memcache_setup_cdev(memcache_devices + i, i);
	}

	return 0; /* succeed */

  fail_malloc:
	unregister_chrdev_region(dev, memcache_devs);
	return result;
}

void memcache_cleanup(void)
{
	int i;
	printk(KERN_INFO "MEMCACHE: Cleanup the MEMCACHE LKM.\n");
	for (i = 0; i < memcache_devs; i++)
	{
		cdev_del(&memcache_devices[i].cdev);
		memcache_trim(memcache_devices + i);
	}
	kfree(memcache_devices);

	unregister_chrdev_region(MKDEV (memcache_major, 0), memcache_devs);
}

module_init(memcache_init);
module_exit(memcache_cleanup);
