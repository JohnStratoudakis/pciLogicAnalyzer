/*
 * DragonPCI.c - template Linux driver for the KNJN Dragon board configured
 * with PCI_PnP. Tested w/ SUSE Linux 10.1, kernel version  2.6.16.13-4.
 *
 * Author: Mike Larson <michael.a.larson@verizon.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Build/use notes (you'll probably need to be superuser to run most of
 * the steps below):
 *
 * 1) How to build the driver:
 *
 *      $ make -C /usr/src/linux M=`pwd` modules
 *
 * 2) How to install the driver:
 *
 *      $ insmod DragonPCI.ko
 *
 * 3) If pci_pnp_init_major (below) is 0, then obtain the major number:
 *
 *      $ cat /proc/devices
 *
 * Look for the line that contains the string "DragonPCI". The major
 * number is to the left.
 *
 * 4) Make the DragonPCI device special file. Substitute the major
 * number obtained above for <major_number>.
 *
 *      $ mknod /dev/DragonPCI c <major_number> 0
 *
 * 5) The test program DragonPCITest does some basic PCI_PnP reads
 * and writes.
 *
 * 6) How to remove the driver:
 *
 *      $ rmmod DragonPCI
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/cdev.h>

#include <asm/byteorder.h>              /* PCI is little endian */
#include <asm/uaccess.h>                /* copy to/from user */

#define PCI_VENDOR_ID_PCI_PNP 0x0100
#define PCI_DEVICE_ID_PCI_PNP 0x0000

#define PCI_PNP_RAM_SIZE 64             /* RAM size (bytes) */

#define PCI_PNP_DRIVER_NAME "DragonPCI" /* driver name */

/*
 * PCI device IDs supported by this driver. The PCI_DEVICE macro sets
 * the vendor and device fields to its input arguments, sets subvendor
 * and subdevice to PCI_ANY_ID. It does not set the class fields.
 */
static struct pci_device_id dragonPci_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_PCI_PNP, PCI_DEVICE_ID_PCI_PNP), },
	{ 0, }
};

/*
 * For completeness, tell the module loading and hotplug systems
 * what PCI devices this module supports.
 */
MODULE_DEVICE_TABLE(pci, dragonPci_ids);

/*
 * pci_register_driver parameter.
 */
static int pci_pnp_probe(struct pci_dev *, const struct pci_device_id *);
static void pci_pnp_remove(struct pci_dev *);

static struct pci_driver pci_pnp_driver = {
	.name = PCI_PNP_DRIVER_NAME,
	.id_table = dragonPci_ids,
	.probe = pci_pnp_probe,
	.remove = pci_pnp_remove,
};

/*
 * File operations i/f.
 */
int pci_pnp_open(struct inode *, struct file *);
int pci_pnp_release(struct inode *, struct file *);
ssize_t pci_pnp_read(struct file *, char __user *, size_t, loff_t *);
ssize_t pci_pnp_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations pci_pnp_fops = {
    read: pci_pnp_read,
    write: pci_pnp_write,
    open: pci_pnp_open,
    release: pci_pnp_release
};

/*
 * Module declarations.
 */
static int __init pci_pnp_init(void);
static void __exit pci_pnp_exit(void);

/*
 * Driver major number. 0 = allocate dynamically.
 */
static int pci_pnp_init_major = 0;
static int pci_pnp_major;

/*
 * Per-device structure.
 */
static struct pci_pnp_dev {
    struct cdev cdev;               /* Char device structure        */
    struct pci_dev *pcidev;         /* PCI device pointer           */
    unsigned iobase;                /* I/O base port address        */
    unsigned ioend;                 /* I/O end  port address        */
    size_t iosize;                  /* I/O region size              */
} *pci_pnp_devices;

#if 0
/*
 * pci_pnp_print_pci_dev - prints the input pci_dev contents.
 */
static void pci_pnp_print_pci_dev(struct pci_dev *pcidev)
{
    int i;

    if(pcidev->driver == NULL)
        printk("Resource information for unknown PCI DEV\n");
    else
        printk("Resource information for PCI DEV %s\n", pcidev->driver->name);
    for(i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
        printk("r[%d] start = 0x%lx, end = 0x%lx, flags = 0x%lx\n", i,
                pcidev->resource[i].start, pcidev->resource[i].end,
                pcidev->resource[i].flags);
    }
}
#endif

/*
 * pci_pnp_probe - pci_driver probe function. Just enable the PCI device.
 * Could also check various configuration registers, find a specific PCI
 * device, request a specific region, etc.
 */
static int pci_pnp_probe(struct pci_dev *pcidev, const struct pci_device_id *id)
{
    struct pci_pnp_dev *dev;

    printk("pci_pnp_probe called ...\n");

    if(pcidev == NULL) {
        printk(KERN_NOTICE "pci_pnp_probe: PCI DEV is NULL\n");
        return -EINVAL;
    }

#if 0
    pci_pnp_print_pci_dev(pcidev);
#endif

    dev = pci_pnp_devices;                  /* only one device for now */
    if(dev == NULL) {
        printk("pci_pnp_probe: device structure not allocated\n");
    } else {
        pci_enable_device(pcidev);
        dev->pcidev = pcidev;
    }

	return 0;
}

/*
 * pci_pnp_remove - pci_driver remove function. Release allocated resources,
 * etc.
 */
static void __devexit pci_pnp_remove(struct pci_dev *dev)
{
    printk("pci_pnp_remove called ...\n");
}

/*
 * pci_pnp_init - module init function. By convention, the function is
 * declared static, even though it is not exported to the rest of the
 * kernel unless explicitly requested via the EXPORT_SYMBOL macro. The
 * __init qualifier tells the loader that the function is only used at
 * module initialization time.
 */
static int __init pci_pnp_init(void)
{
    struct pci_pnp_dev *dev;
    dev_t devno;
    int result, bar;

    printk("pci_pnp_init called ...\n");

    /*
     * Allocate the per-device structure(s).
     */
    pci_pnp_devices = kmalloc(sizeof(struct pci_pnp_dev), GFP_KERNEL);
    if(pci_pnp_devices == NULL) {
        result = -ENOMEM;
        goto fail;
    }
    
    /*
     * Get a range of minor numbers to work with, asking for a dynamic
     * major unless directed otherwise at load time.
     */
    if(pci_pnp_init_major) {
        pci_pnp_major = pci_pnp_init_major;
        devno = MKDEV(pci_pnp_major, 0);
        result = register_chrdev_region(devno, 1, PCI_PNP_DRIVER_NAME);
    } else {
        result = alloc_chrdev_region(&devno, 0, 1, PCI_PNP_DRIVER_NAME);
        pci_pnp_major = MAJOR(devno);
    }
    if(result < 0) {
        printk(KERN_NOTICE "pci_pnp: can't get major %d\n", pci_pnp_major);
        goto fail;
    }

    dev = pci_pnp_devices;                  /* only one device for now */

    /*
     * Initialize and add this device's character device table entry.
     */
    dev->pcidev = NULL; 
    cdev_init(&dev->cdev, &pci_pnp_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &pci_pnp_fops;
    result = cdev_add(&dev->cdev, devno, 1);
    if(result) {
        printk(KERN_NOTICE "Error %d adding %s device", result,
                    PCI_PNP_DRIVER_NAME);
        goto fail;
    }

	if((result = pci_register_driver(&pci_pnp_driver)) != 0) {
        printk(KERN_NOTICE "Error %d registering %s PCI device",
                    result, PCI_PNP_DRIVER_NAME);
        goto fail;
    }

    if(dev->pcidev == NULL) {
        printk(KERN_NOTICE "PCI DEV is NULL, probe failed?\n");
        goto fail;
    }

    /*
     * Look for the I/O space address range.
     */
    for(bar = 0; bar < 6; bar++) {
        unsigned long flags;

        flags = pci_resource_flags(dev->pcidev, bar);
        if(flags & IORESOURCE_IO) {
            dev->iobase = pci_resource_start(dev->pcidev, bar);
            dev->ioend  = pci_resource_start(dev->pcidev, bar);
            dev->iosize = dev->ioend - dev->iobase;
            break;
        }
    }

    if(bar >= 6) {
        printk(KERN_NOTICE "Could not find PCI I/O range\n");
        goto fail;
    }

#if 0
    /*
     * Make sure no other driver is using the region.
     * Already reserved by PCI subsystem?
     */
    if(!request_region(dev->iobase, dev->iosize, PCI_PNP_DRIVER_NAME)) {
        printk(KERN_NOTICE "Can't reserve I/O @ 0x%x\n", dev->iobase);
        goto fail;
    }
#endif

	return 0;

fail: 
    pci_pnp_exit(); 
    return result;
}

/*
 * pci_pnp_exit - module exit function. Release resources allocated
 * by pci_pnp_init.
 */
static void __exit pci_pnp_exit(void)
{
    printk("pci_pnp_exit called ...\n");

    if(pci_pnp_devices) {
        struct pci_pnp_dev *dev;

        dev = &pci_pnp_devices[0];

        cdev_del(&dev->cdev);

#if 0
        release_region(dev->iobase, dev->iosize);
#endif

        kfree(pci_pnp_devices);
        pci_pnp_devices = NULL;
    }

    unregister_chrdev_region(MKDEV(pci_pnp_major, 0), 1);
    pci_pnp_major = 0;

	pci_unregister_driver(&pci_pnp_driver);
}

/*
 * pci_pnp_open - open file processing.
 */
int pci_pnp_open(struct inode *inode, struct file *filep)
{
    struct pci_pnp_dev *dev;

    dev = container_of(inode->i_cdev, struct pci_pnp_dev, cdev);
    filep->private_data = dev;           /* used by read, write, etc */

    /* Success */
    return 0;
}

/*
 * pci_pnp_release - close processing.
 */
int pci_pnp_release(struct inode *inode, struct file *filep)
{
    /* Success */
    return 0;
}

/*
 * pci_pnp_read - read processing.
 */
ssize_t pci_pnp_read(struct file *filep, char __user *buf,
                    size_t count, loff_t *f_pos)
{
    struct pci_pnp_dev *dev;
    unsigned port, rcnt, i;
    unsigned char kbuf[PCI_PNP_RAM_SIZE];

    /*
     * Check alignment.
     */
    if(((*f_pos % sizeof(long)) != 0) || ((count % sizeof(long)) != 0))
        return -EINVAL;

    if(*f_pos >= PCI_PNP_RAM_SIZE)              /* EOF? */
        return 0;

    /*
     * Get read count, trim if necessary.
     */
    if((*f_pos + count) > PCI_PNP_RAM_SIZE)
        rcnt = PCI_PNP_RAM_SIZE - *f_pos;
    else
        rcnt = count;

    /*
     * Calculate the starting I/O address.
     */
    dev = filep->private_data;
    port = dev->iobase + *f_pos;

    /*
     * Transfer data from the Dragon.
     */
    for(i = 0; i < rcnt; i += sizeof(long)) {
        *((unsigned long *)(kbuf + i)) = inl(port + i);
#if 0
        printk("read 0x%lx from port 0x%x\n", *((unsigned long *)(kbuf + i)),
                    port + i);
#endif
    }

    /*
     * Transfer data to user space
     */ 
    if(copy_to_user(buf, kbuf, rcnt)) {
        return -EFAULT;
    }

    /*
     * Update the file position and return the number of bytes read.
     */
    *f_pos += rcnt;

    return rcnt;
}

/*
 * pci_pnp_write - write processing.
 */
ssize_t pci_pnp_write(struct file *filep, const char __user *buf,
                    size_t count, loff_t *f_pos)
{
    struct pci_pnp_dev *dev;
    unsigned port, wcnt, i;
    unsigned char kbuf[PCI_PNP_RAM_SIZE];

    /*
     * Check alignment.
     */
    if(((*f_pos % sizeof(long)) != 0) || ((count % sizeof(long)) != 0))
        return -EINVAL;

    if(*f_pos >= PCI_PNP_RAM_SIZE)
        return -ENOSPC;

    /*
     * Get write count, trim if necessary.
     */
    if((*f_pos + count) > PCI_PNP_RAM_SIZE)
        wcnt = PCI_PNP_RAM_SIZE - *f_pos;
    else
        wcnt = count;

    /*
     * Calculate the starting I/O address.
     */
    dev = filep->private_data;
    port = dev->iobase + *f_pos;

    /*
     * Transfer data from user space
     */ 
    if(copy_from_user(kbuf, buf, wcnt)) {
        return -EFAULT;
    }

    /*
     * Transfer data to the Dragon.
     */
    for(i = 0; i < wcnt; i += sizeof(long)) {
#if 0
        printk("writing 0x%lx to port 0x%x\n", *((unsigned long *)(kbuf + i)),
                    port + i);
#endif
        outl(*((unsigned long *)(kbuf + i)), port + i);
    }

    /*
     * Update the file position and return the number of bytes read.
     */
    *f_pos += wcnt;

    return wcnt;
}

MODULE_LICENSE("Dual BSD/GPL");

module_init(pci_pnp_init);
module_exit(pci_pnp_exit);

