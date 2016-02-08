#include <linux/ioctl.h>
#include <linux/cdev.h>

#define MEMCACHE_MAJOR 		0   	        /* dynamic major by default */
#define MEMCACHE_DEVS 		4    	        /* memcache0 through memcache3 */
#define MEMCACHE_CACHE_SIZE     1024
#define MEMCACHE_DATA_SIZE	4096


struct memcache_dev 
{
        char *data_buffer, *data_end;       	/* begin of buf, end of buf */
        int data_buffersize;                    /* used in pointer arithmetic */

	char *cache_buffer, *cache_end;
	int cache_buffersize;

	struct memcache_dev *next; 
	int size;

        char *rp, *wp;                     	/* where to read, where to write */
        int nreaders, nwriters;            	/* number of openings for r/w */
        struct mutex mutex;              	/* mutual exclusion semaphore */
        struct cdev cdev;                  	/* Char device structure */

        int sayi;
        int iseof;
};

extern struct memcache_dev *memcache_devices;
extern struct file_operations memcache_fops;

/*
 * The different configurable parameters
 */
extern int memcache_major;     /* main.c */
extern int memcache_devs;
extern int memcache_cache_size;
extern int memcache_data_size;

/*
 * Prototypes for shared functions
 */
int     memcache_trim(struct memcache_dev *dev);
struct  memcache_dev *memcache_follow(struct memcache_dev *dev, int n);


/*
 * Ioctl definitions
 */
#define MEMCACHE_IOC_MAGIC 	'm'

#define MEMCACHE_IOCRESET	_IO (MEMCACHE_IOC_MAGIC,   0)
#define MEMCACHE_IOCGETCACHE	_IOR(MEMCACHE_IOC_MAGIC,   1,int)
#define MEMCACHE_IOCSETCACHE	_IOW(MEMCACHE_IOC_MAGIC,   2,int)
#define MEMCACHE_IOCTRUNC	_IO (MEMCACHE_IOC_MAGIC,   3)
#define MEMCACHE_IOCQBUFSIZE	_IO (MEMCACHE_IOC_MAGIC,   4)
#define MEMCACHE_IOCGTESTCACHE	_IOR(MEMCACHE_IOC_MAGIC,   5,int)

#define MEMCACHE_IOC_MAXNR      5
