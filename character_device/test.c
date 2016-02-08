#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#define MEMCACHE_IOC_MAGIC      'm'
#define MEMCACHE_IOCRESET       _IO(MEMCACHE_IOC_MAGIC, 0)
#define MEMCACHE_IOCGETCACHE    _IOR(MEMCACHE_IOC_MAGIC, 1, int)
#define MEMCACHE_IOCSETCACHE    _IOW(MEMCACHE_IOC_MAGIC, 2, int)
#define MEMCACHE_IOCTRUNC       _IO(MEMCACHE_IOC_MAGIC, 3)
#define MEMCACHE_IOCQBUFSIZE    _IO(MEMCACHE_IOC_MAGIC, 4)
#define MEMCACHE_IOCGTESTCACHE  _IOR(MEMCACHE_IOC_MAGIC, 5, int)
#define MEMCACHE_IOC_MAXNR      5

int main(){

    int fd0 = open("/dev/memcache0", O_RDWR);
    int fd1 = open("/dev/memcache1", O_RDWR);
    int fd2 = open("/dev/memcache2", O_RDWR);
    int fd3 = open("/dev/memcache3", O_RDWR);

    char buf[1024],buf2[1024];
    int size, result;

    // memcache0
    snprintf(buf2, 13, "hi there fd0");
    write(fd0, buf2, 13);
    snprintf(buf2, 1, "");
    read(fd0, buf2, 13);
    printf("%s\n", buf2);

    ioctl(fd0, MEMCACHE_IOCSETCACHE, "i am memcache0");
    ioctl(fd0, MEMCACHE_IOCGETCACHE, buf);
    printf("%s\n", buf);
    size = ioctl(fd0, MEMCACHE_IOCQBUFSIZE);
    printf("%d\n", size);
    ioctl(fd0, MEMCACHE_IOCTRUNC);
    ioctl(fd0, MEMCACHE_IOCGETCACHE, buf);
    printf("%s\n", buf);

    snprintf(buf2, 1, "");
    read(fd0, buf2, 13);
    printf("%s\n\n", buf2);

    // memcache1
    snprintf(buf2, 13, "hi there fd1");
    write(fd1, buf2, 13);
    snprintf(buf2, 1, "");
    read(fd1, buf2, 13);
    printf("%s\n", buf2);

    ioctl(fd1, MEMCACHE_IOCSETCACHE, "my name is memcache1");
    ioctl(fd1, MEMCACHE_IOCGETCACHE, buf);
    printf("%s\n", buf);
    size = ioctl(fd1, MEMCACHE_IOCQBUFSIZE);
    printf("%d\n", size);
    ioctl(fd1, MEMCACHE_IOCTRUNC);
    ioctl(fd1, MEMCACHE_IOCGETCACHE, buf);
    printf("%s\n", buf);

    snprintf(buf2, 1, "");
    read(fd1, buf2, 13);
    printf("%s\n\n", buf2);

    // memcache2
    snprintf(buf2, 13, "hi there fd2");
    write(fd2, buf2, 13);
    snprintf(buf2, 1, "");
    read(fd2, buf2, 13);
    printf("%s\n", buf2);

    ioctl(fd2, MEMCACHE_IOCSETCACHE, "hello from memcache2");
    ioctl(fd2, MEMCACHE_IOCGETCACHE, buf);
    printf("%s\n", buf);
    size = ioctl(fd2, MEMCACHE_IOCQBUFSIZE);
    printf("%d\n", size);
    ioctl(fd2, MEMCACHE_IOCTRUNC);
    ioctl(fd2, MEMCACHE_IOCGETCACHE, buf);
    printf("%s\n", buf); 

    snprintf(buf2, 1, "");
    read(fd2, buf2, 13);
    printf("%s\n\n", buf2);

    // memcache3
    snprintf(buf2, 13, "hi there fd3");
    write(fd3, buf2, 13);
    snprintf(buf2, 1, "");
    read(fd3, buf2, 13);
    printf("%s\n", buf2);

    ioctl(fd3, MEMCACHE_IOCSETCACHE, "wtf man, im memcache3");
    ioctl(fd3, MEMCACHE_IOCGETCACHE, buf);
    printf("%s\n", buf);
    size = ioctl(fd3, MEMCACHE_IOCQBUFSIZE);
    printf("%d\n", size);
    ioctl(fd3, MEMCACHE_IOCTRUNC);
    ioctl(fd3, MEMCACHE_IOCGETCACHE, buf);
    printf("%s\n", buf);

    snprintf(buf2, 1, "");
    read(fd0, buf2, 13);
    printf("%s\n\n", buf2);

    ioctl(fd0, MEMCACHE_IOCRESET);    

    fd0 = open("/dev/memcache0", O_RDWR);
    fd1 = open("/dev/memcache1", O_RDWR);
    fd2 = open("/dev/memcache2", O_RDWR);
    fd3 = open("/dev/memcache3", O_RDWR);

    printf("%d %d %d %d\n", fd0, fd1, fd2, fd3);

    return 0;

}
