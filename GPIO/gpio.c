#include "linux/fs.h"
#include "linux/cdev.h"
#include "linux/module.h"
#include "linux/kernel.h"
#include "linux/device.h"
#include "asm/uaccess.h"
#include "asm/io.h"
#include "linux/slab.h"

#define DEVICE_NAME     "leds"

#define NUM_LEDS        4

#define LED_ON          1
#define LED_OFF         0

#define GPB_BASE    0xFB000010
#define GPBCON      GPB_BASE
#define GPBDAT      GPB_BASE + 4

#define CLEAR_PORT_MASK     0xFFFC03FF
#define SET_WRITE_PORT_MASK 0x00015400

/* prototypes */
int leds_open(struct inode *inode, struct file *file);
ssize_t leds_read(struct file *file, char *buf, size_t count, loff_t *ppos);
ssize_t leds_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
int leds_release(struct inode *inode, struct file *file);

/* per led structure */
struct leds_device {
    int number;    
    int status;
    struct cdev cdev;
} leds_dev[NUM_LEDS];

/* file operations structure */
static struct file_operations leds_fops = {
    .owner      = THIS_MODULE,
    .open       = leds_open,
    .release    = leds_release,
    .read       = leds_read,
    .write      = leds_write,
};

/* leds driver major number */
static dev_t leds_dev_number;

/* initialize led port - GPB */
void initLedPort(void)
{
    void __iomem *base = (void __iomem *)GPBCON;
    
    u32 port = __raw_readl(base);
        
    port &= CLEAR_PORT_MASK;
    port |= SET_WRITE_PORT_MASK;
    
    __raw_writel(port, base);
}

/* change led status */
void changeLedStatus(int led_num, int status)
{
    void __iomem *base = (void __iomem *)GPBDAT;
    u32 mask, data;
    
    data = __raw_readl(base);
    
    mask = 0x01 << (4 + led_num);
    
    switch (status) {
        
        case LED_ON:
            mask = ~mask;
            data &= mask;
            break;
            
        case LED_OFF:
            data |= mask;
            break;
    }
    
    __raw_writel(data, base);
}


/* driver initialization */
int __init leds_init(void)
{
    int ret, i;
        
    /* request device major number */
    if ((ret = alloc_chrdev_region(&leds_dev_number, 0, NUM_LEDS, DEVICE_NAME) < 0)) {
        printk(KERN_DEBUG "Error registering device!\n");
        return ret;
    }
    
    /* init leds GPIO port */
    initLedPort();
    
    /* init each led device */
    for (i = 0; i < NUM_LEDS; i++) {
        
        /* init led status */
        leds_dev[i].number = i + 1;
        leds_dev[i].status = LED_OFF;
    
        /* connect file operations to this device */
        cdev_init(&leds_dev[i].cdev, &leds_fops);
        leds_dev[i].cdev.owner = THIS_MODULE;
    
        /* connect major/minor numbers */
        if ((ret = cdev_add(&leds_dev[i].cdev, (leds_dev_number + i), 1))) {
            printk(KERN_DEBUG "Error adding device!\n");
            return ret;
        }
        
        /* init led status */
        changeLedStatus(leds_dev[i].number, LED_OFF);
    }
    
    printk("Leds driver initialized.\n");
    
    return 0;        
}

/* driver exit */
void __exit leds_exit(void)
{
    /* release major number */
    unregister_chrdev_region(leds_dev_number, NUM_LEDS);
    
    printk("Exiting leds driver.\n");
}

/* open led file */
int leds_open(struct inode *inode, struct file *file)
{
    struct leds_device *leds_devp;
    
    /* get cdev struct */
    leds_devp = container_of(inode->i_cdev, struct leds_device, cdev);
    
    /* save cdev pointer */
    file->private_data = leds_devp;
    
    /* return OK */
    return 0;
}

/* close led file */
int leds_release(struct inode *inode, struct file *file)
{
    /* return OK */
    return 0;
}

/* read led status */
ssize_t leds_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    struct leds_device *leds_devp = file->private_data;
    
    if (leds_devp->status == LED_ON) {
        if (copy_to_user(buf, "1", 1))
            return -EIO;
    }
    else {
        if (copy_to_user(buf, "0", 1))
            return -EIO;
    }
    
    return 1;
}

ssize_t leds_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    struct leds_device *leds_devp = file->private_data;
    char kbuf = 0;
    
    if (copy_from_user(&kbuf, buf, 1)) {
        return -EFAULT;
    }
    
    if (kbuf == '1') {
        changeLedStatus(leds_devp->number, LED_ON);
        leds_devp->status = LED_ON;
    }
    else if (kbuf == '0') {
        changeLedStatus(leds_devp->number, LED_OFF);
        leds_devp->status = LED_OFF;
    }
        
    return count;
}

module_init(leds_init);
module_exit(leds_exit);
MODULE_AUTHOR("hethongnhung.com");
MODULE_LICENSE("GPL");