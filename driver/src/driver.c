#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include "mydev.h"

#include "ops.h"

// #define IO_IRQ_ACK 0x64
// #define IO_IRQ_STATUS 0x24

MODULE_LICENSE("GPL");

static struct pci_device_id pci_ids[] = {
        {PCI_DEVICE(MY_VENDOR_ID, MY_DEVICE_ID)},
        {0}
};

MODULE_DEVICE_TABLE(pci, pci_ids);

static int            major;
static struct pci_dev *pdev;
void __iomem          *mmio;
dev_t                 devno;
struct class          *devclass;

static struct file_operations fops = {
        .owner          = THIS_MODULE,
        .llseek         = llseek,
        .read           = read,
        .unlocked_ioctl = dev_ioctl,
        .write          = write
};

/*
irqreturn_t irq_handler(int irq, void *dev)
{
        int devi;
        irqreturn_t ret;
        u32 irq_status;

        devi = *(int *)dev;
        if (devi == major) {
                irq_status = ioread32(mmio + IO_IRQ_STATUS);
                pr_info("interrupt irq = %d dev = %d irq_status = %llx\n",
                                irq, devi, (unsigned long long)irq_status);
                // Must do this ACK, or else the interrupts just keeps firing.
                iowrite32(irq_status, mmio + IO_IRQ_ACK);
                ret = IRQ_HANDLED;
        } else {
                ret = IRQ_NONE;
        }
        return ret;
}
*/

static int pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
        /* this function is only called for every matching PCIe card */
        u8 val;

        pdev = dev;
        if (pci_enable_device(dev) < 0) {
                dev_err(&(pdev->dev), "pci_enable_device\n");
                return 1;
        }

        if (pci_request_region(dev, BAR, "myregion0")) {
                dev_err(&(pdev->dev), "pci_request_region\n");
                return 1;
        }
        mmio = pci_ioremap_bar(pdev, BAR);
        printk(KERN_INFO "BAR0 mapped to %lX\n", (uintptr_t)mmio);
        /* IRQ setup. */
        // pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &val);
        /*pci_irq = pdev->irq;
        if (request_irq(pci_irq, irq_handler, IRQF_SHARED,
                        "pci_irq_handler0", &major) < 0) {
                dev_err(&(dev->dev), "request_irq\n");
                return 1;
        }

        printk(KERN_INFO "Registered IRQ handler for IRQ %d\n", pci_irq);
        */
        /* Optional sanity checks. The PCI is ready now, all of this could also
         * be called from fops. */
        if (1) {
                unsigned i;
		resource_size_t start, end;
                /* Check that we are using MEM instead of IO.
                 *
                 * In QEMU, the type is defiened by either:
                 *
                 * - PCI_BASE_ADDRESS_SPACE_IO
                 * - PCI_BASE_ADDRESS_SPACE_MEMORY
                 */
                if ((pci_resource_flags(dev, BAR) & IORESOURCE_MEM) != IORESOURCE_MEM) {
                        dev_err(&(dev->dev), "pci_resource_flags\n");
                        return 1;
                }

                start = pci_resource_start(pdev, BAR);
                end   = pci_resource_end(pdev, BAR);
                pr_info("length %llx\n", (unsigned long long)(end + 1 - start));

                /* The PCI standardized 64 bytes of the configuration space, see LDD3. */
                for (i = 0; i < 64u; ++i) {
                        pci_read_config_byte(pdev, i, &val);
                        pr_info("config %x %x\n", i, val);
                }
        }
        return 0;
}


static void pci_remove(struct pci_dev *dev)
{
        pr_info("pci_remove\n");
        // free_irq(pci_irq, &major);
        pci_release_region(dev, BAR);
        
}


static struct pci_driver pci_driver = {
        .name     = "pcie-led",
        .id_table = pci_ids,
        .probe    = pci_probe,
        .remove   = pci_remove,
};


static int myinit(void)
{
        struct device *sysdev;

        if (pci_register_driver(&pci_driver) < 0) {
                printk(KERN_INFO "pcie-led error registering the PCIe-LED driver\n");
                return -1;
        }
        printk(KERN_INFO "pcie-led success on registering the PCIe-LED driver\n");

        major = register_chrdev(0, MY_DEV_NAME, &fops);
        if (major < 0) {
                printk(KERN_WARNING "Could not register chrdev\n");
                dev_err(&(pdev->dev), "char dev registration failed\n");
                return -1;
        }
        printk(KERN_INFO "Major ID = %x\n", major);

        devno = MKDEV(major, 0);
        /* Later kernels don't need first parameter */
        devclass = class_create(THIS_MODULE, CLASS_NAME);
        if (IS_ERR(devclass)) {
                printk(KERN_WARNING "can't create class\n");
                unregister_chrdev_region(devno, 1);
                return -1;
        }

	sysdev = device_create(devclass, NULL, devno, NULL, CLASS_NAME);
        if (IS_ERR(sysdev)) {
                printk(KERN_WARNING "can't create /dev/" CLASS_NAME "\n");
                class_destroy(devclass);
                unregister_chrdev_region(devno, 1);
                return -1;
        }
        return 0;
}


static void myexit(void)
{
        device_destroy(devclass, devno);
        class_destroy(devclass);
        unregister_chrdev(major, MY_DEV_NAME);
        pci_unregister_driver(&pci_driver);
}


module_init(myinit);
module_exit(myexit);
