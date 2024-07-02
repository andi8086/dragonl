#include <asm/uaccess.h> /* put_user */
#include <linux/cdev.h> /* cdev_ */
#include <linux/fs.h>
#include <linux/pci.h>

#include "mydev.h"
#include "ops.h"

extern void __iomem *mmio;

long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rv = 0;
	printk(KERN_INFO "dev_ioctl() cmd: %x arg: %lx \n",cmd, arg);
	
	switch (cmd) {
	/*
	case 7:
		ip = (int *) arg;	
		*ip = ioread32((void*)(mmio + 0x30));
		printk(KERN_INFO "result: %x\n",*ip);
		break;
	case 10:
		set = (int) arg;
		iowrite32(set, mmio + 0x18);
		break;
	*/
	default:
		break;
	}
	return rv;
}


ssize_t read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	u32 kbuf;
        printk(KERN_INFO CLASS_NAME ": read() len: %lx offset: %llx\n",len, *off);
	if (*off % 4 || len == 0) {
		return 0;
	}
	kbuf = ioread32(mmio + *off);
	if (copy_to_user(buf, (void *)&kbuf, sizeof(kbuf))) {
		printk(KERN_ALERT "fault on copy_to_user\n");
		return -EFAULT;
	} 
	return sizeof(kbuf);
}


ssize_t write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	u32 kbuf;
        printk(KERN_INFO CLASS_NAME ": write()\n");

	if ((*off % 4) || len != sizeof(kbuf)) {
		return 0;
	}
	if (copy_from_user((void *)&kbuf, buf, 4)) {
		return -EFAULT;
	}
	iowrite32(kbuf, mmio + *off);
	return len;
}

loff_t llseek(struct file *filp, loff_t off, int whence)
{
	filp->f_pos = off;
	return off;
}
