#include <linux/module.h>

#include <linux/mm.h>		/* everything */
#include <linux/errno.h>	/* error codes */
#include <asm/pgtable.h>

#include "memcache.h"		/* local definitions */


/*
 * open and close: just keep track of how many times the device is
 * mapped, to avoid releasing it.
 */

void memcache_vma_open(struct vm_area_struct *vma)
{
	struct memcache_dev *dev = vma->vm_private_data;

	dev->vmas++;
}

void memcache_vma_close(struct vm_area_struct *vma)
{
	struct memcache_dev *dev = vma->vm_private_data;

	dev->vmas--;
}

/*
 * The nopage method: the core of the file. It retrieves the
 * page required from the memcache device and returns it to the
 * user. The count for the page must be incremented, because
 * it is automatically decremented at page unmap.
 *
 * For this reason, "order" must be zero. Otherwise, only the first
 * page has its count incremented, and the allocating module must
 * release it as a whole block. Therefore, it isn't possible to map
 * pages from a multipage block: when they are unmapped, their count
 * is individually decreased, and would drop to 0.
 */

int memcache_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	unsigned long offset;
	struct memcache_dev *ptr, *dev = vma->vm_private_data;
	struct page *page = NOPAGE_SIGBUS;
	void *pageptr = NULL; /* default to "missing" */

	mutex_lock(&dev->mutex);
	offset = (unsigned long)(vmf->virtual_address - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT);
	if (offset >= dev->size) goto out; /* out of range */

	/*
	 * Now retrieve the memcache device from the list,then the page.
	 * If the device has holes, the process receives a SIGBUS when
	 * accessing the hole.
	 */
	offset >>= PAGE_SHIFT; /* offset is a number of pages */
	for (ptr = dev; ptr && offset >= dev->qset;) {
		ptr = ptr->next;
		offset -= dev->qset;
	}
	if (ptr && ptr->data) pageptr = ptr->data[offset];
	if (!pageptr) goto out; /* hole or end-of-file */

	/* got it, now increment the count */
	page = virt_to_page(pageptr);
	get_page(page);
  out:
	mutex_unlock(&dev->mutex);
	if (!page)
		return VM_FAULT_SIGBUS;
	vmf->page = page;
	return 0;
}



struct vm_operations_struct memcache_vm_ops = {
	.open 	=	memcache_vma_open,
	.close 	=	memcache_vma_close,
	.fault 	=	memcache_vma_fault,
};


int memcache_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct inode *inode = filp->f_dentry->d_inode;

	/* refuse to map if order is not 0 */
	if (memcache_devices[iminor(inode)].order)
		return -ENODEV;

	/* don't do anything here: "nopage" will set up page table entries */
	vma->vm_ops = &memcache_vm_ops;
	vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);
	vma->vm_private_data = filp->private_data;
	memcache_vma_open(vma);
	return 0;
}

