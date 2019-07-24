#include "linux/fs.h"
#include "linux/cdev.h"
#include "linux/module.h"
#include "linux/kernel.h"
#include "linux/device.h"
#include "asm/uaccess.h"
#include "asm/io.h"
#include "linux/slab.h"
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define DEVICE_NAME "leds"

#define LED_ON 1
#define LED_OFF 0

#define BASE_ADDR 0x01C20800
/* PA7 PA9*/
#define LED_GREEN 7
#define LED_RED 9
#define OFF_SET_COFIG_REG_PA_0_7 0x00

#define OFF_SET_CONFIG_PA7 28
#define DATA_REG 0x10
/*Button PA8 */
#define BUTTON 8
#define OFF_SET_COFIG_REG_PA_8_15 0x04
#define OFFSET_GPIOA_PULL 0x1C
#define OFFSET_EXTERNAL_INTERRUPT_GPIOA 0x200
#define SIZE_EXTERNAL_INTERRUPT_GPIOA 0x20
#define SIZE_GPIOA_PULL 0x08
#define SIZE_GPIOA 0x14

/*variable */
static struct timer_list exp_timer;
unsigned char irq_eint0;
unsigned int *GPIO_8_15;
unsigned int *ex_GPIO_8_15;
unsigned int *gpioa_pull_0_15;
/* entry point */
int leds_open(struct inode *inode, struct file *file);
ssize_t leds_read(struct file *file, char *buf, size_t count, loff_t *ppos);
ssize_t leds_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
int leds_release(struct inode *inode, struct file *file);

/* per led structure */
struct _vchar_drv{
	dev_t dev_num;
	struct class *dev_class;
	struct device *dev;
	/*Memory allocation*/
	struct cdev *vcdev;
	unsigned int open_cnt;
}vchar_drv;

/* file operations structure */
static struct file_operations leds_fops = {
    .owner = THIS_MODULE,
    .open = leds_open,
    .release = leds_release,
    .read = leds_read,
    .write = leds_write,
};

/* leds driver major number */
static void __iomem *io;
static unsigned long temp;

/* initialize led port - GPB */
void initLedPort(void)
{
    io = ioremap(BASE_ADDR, 0x100);
    if (io == NULL)
    {
        pr_alert("fail map base address\n");
        //return -1;
    }
    temp = ioread32(io + OFF_SET_COFIG_REG_PA_0_7);

    // set config mode output for pa7
    temp = temp & (~(0x7 << OFF_SET_CONFIG_PA7));

    // set mode output
    temp |= (0x01 << OFF_SET_CONFIG_PA7);

    iowrite32(temp, (io + OFF_SET_COFIG_REG_PA_0_7));
	temp = ioread32(io + DATA_REG);
	temp |= (1 << LED_GREEN);
	iowrite32(temp, (io + DATA_REG));

}

/* change led status */
void changeLedStatus(int status)
{
    temp = ioread32(io + DATA_REG);
    switch (status)
    {
    case LED_ON:
        temp |= (1 << LED_GREEN);
        break;

    case LED_OFF:
        temp &= ~(1 << LED_GREEN);
        break;
    }
    iowrite32(temp, (io + DATA_REG));
}
void toggle(int led_number)
{
    
    temp = ioread32(io + DATA_REG);
    if(temp&(1 << led_number))
    {
        temp &= ~(1 << led_number);
    }
    else{
        temp |= (1 << led_number);
    }
    iowrite32(temp, (io + DATA_REG));

}
/*interrupt handler */
irqreturn_t external_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	printk("before [0]=%x ---[4]=%x/n",&(GPIO_8_15[0]),&(GPIO_8_15[4])); 
    toggle(LED_GREEN);
	return IRQ_HANDLED;

}
/*timer */
static void timer_led(unsigned long);

static void timer_led(unsigned long data)
{
	unsigned int delay = 1 * HZ;
    toggle(LED_RED);
	exp_timer.expires = jiffies + delay;
	exp_timer.function = (void *)timer_led;
	add_timer(&exp_timer);

}
/* driver initialization */
int __init leds_init(void)
{
    int ret;

    /* request device major number */
    if ((ret = alloc_chrdev_region(&vchar_drv.dev_num, 0, 1, DEVICE_NAME) < 0))
    {
        printk(KERN_DEBUG "Error registering device!\n");
        return ret;
    }

    /* init leds GPIO port */
    initLedPort();
 

	printk("alocated device number (%d,%d)\n",MAJOR(vchar_drv.dev_num),MINOR(vchar_drv.dev_num));
    
    vchar_drv.dev_class = class_create(THIS_MODULE,"class_vchar_dev");
   	if(vchar_drv.dev_class == NULL)
   	{
   		printk("failed to create a device class\n");
   		goto failed_create_class;
   	}

   	vchar_drv.dev = device_create(vchar_drv.dev_class,NULL,vchar_drv.dev_num,NULL,"vchar_dev");
 	if(IS_ERR(vchar_drv.dev))
 	{
 		printk("failed to create a device\n");
 		goto failed_create_device;
 	}

	/*register entry point to kernel*/
	vchar_drv.vcdev = cdev_alloc();
	if(vchar_drv.vcdev == NULL)
	{
		printk("failed to allocate cdev structure\n");
	}
	cdev_init(vchar_drv.vcdev,&leds_fops);
	ret = cdev_add(vchar_drv.vcdev, vchar_drv.dev_num, 1);
	if(ret < 0)
	{
		printk("failed to add a char device to the system\n");
	}

    /*interrupt */
    GPIO_8_15 = ioremap(BASE_ADDR + OFF_SET_COFIG_REG_PA_8_15, SIZE_GPIOA);
	if(GPIO_8_15 == NULL)
	{
		printk("Mapping address gpioa fail\n");
		return -1;
	}

	//Select EINT0 mode GPIOA_8
	GPIO_8_15[0] &= ~(7 << 0);
	GPIO_8_15[0] |= (6 << 0);

    
	//Mapping EINT_GPIOA
	ex_GPIO_8_15 = ioremap(BASE_ADDR + OFFSET_EXTERNAL_INTERRUPT_GPIOA, SIZE_EXTERNAL_INTERRUPT_GPIOA);
	if(ex_GPIO_8_15 == NULL)
	{
		printk("Mapping ex_gpioa fail\n");
		return -1;
	}

	//EINT0 Positive Edge
	ex_GPIO_8_15[0] &= ~(15 << 0);
	ex_GPIO_8_15[0] |= (1 << 0);

	//Mapping gpioa pull
	gpioa_pull_0_15 = ioremap(BASE_ADDR + OFFSET_GPIOA_PULL, SIZE_GPIOA_PULL);
	if(gpioa_pull_0_15 == NULL)
	{
		printk("Mapping gpioa pull fail!\n");
		return -1;
	}
	//GPIOA8 PULL UP
	gpioa_pull_0_15[0] &= ~(3 << 0);
	gpioa_pull_0_15[0] |= (1 << 0);

	//Init interrupt.
	irq_eint0 = gpio_to_irq(BUTTON);
	printk("irq_eint0 = %d!\n",irq_eint0);
	ret = request_irq(irq_eint0, (irq_handler_t) external_interrupt_handler, IRQF_SHARED, "EINT0", (void *)external_interrupt_handler);
	if(ret)
		printk("Init EINT0 success\n");
    /*end interrupt */
    /*init timer */
    exp_timer.expires = jiffies + 1 * HZ;
	exp_timer.function = (void *)timer_led;

	//Select output mode GPIOA_9
   
	GPIO_8_15[0] &= ~(7 << 4);
	GPIO_8_15[0] |= (1 << 4);
	/*start timer */ 
	add_timer(&exp_timer);
    return 0;

    failed_allocate_structure:
    device_destroy(vchar_drv.dev_class,vchar_drv.dev_num);

    failed_create_device:
    class_destroy(vchar_drv.dev_class);

    failed_create_class:
    unregister_chrdev_region(vchar_drv.dev_num,1);
}

/* driver exit */
void __exit leds_exit(void)
{
	temp = ioread32(io + DATA_REG);
	temp &= ~( 1 << LED_GREEN);
	iowrite32(temp , (io +DATA_REG));
	/*destroy entry point*/
	cdev_del(vchar_drv.vcdev);
	/* remove device file*/
	device_destroy(vchar_drv.dev_class,vchar_drv.dev_num);
	class_destroy(vchar_drv.dev_class);
	/* free device number*/
	unregister_chrdev_region(vchar_drv.dev_num,1);
	printk("exit vchar driver ");
    iounmap(GPIO_8_15);
	iounmap(ex_GPIO_8_15);
	iounmap(gpioa_pull_0_15);
	free_irq(irq_eint0, (void *)external_interrupt_handler);
}

/* open led file */
int leds_open(struct inode *inode, struct file *file)
{
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
    char *kernel_buf = NULL;
    int num_bytes = count;
    printk("Handle read event start from %lld, %zu byte\n", ppos, count);
    kernel_buf = kzalloc(count, GFP_KERNEL);
    if (kernel_buf == NULL)
        return 0;
    memcpy((io + DATA_REG), kernel_buf, num_bytes);
    printk("read %d bytes from HW\n", num_bytes);
    if (num_bytes < 0)
        return -EFAULT;
    if (copy_to_user(buf, kernel_buf, num_bytes))
        return -EFAULT;
    *ppos += num_bytes;
    return num_bytes;
}
/*write led */
ssize_t leds_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{

    char *kernel_buf = NULL;
    int num_bytes = count;

    printk("Handle write event from %lld, %zu bytes\n", *ppos, count);
    kernel_buf = kzalloc(count, GFP_KERNEL);
    if (copy_from_user(kernel_buf, buf, count))
    {
         return -EFAULT;
    }
    
    printk("before kernel_buf == %ld ----%ld\n", kernel_buf[0],kernel_buf[1]);
    if(kernel_buf[0] == '0')
    {
        changeLedStatus(LED_ON);
    }
    else if(kernel_buf[0] == '1')
    {
        changeLedStatus(LED_OFF);
    }
    printk("writes %d bytes to HW\n", num_bytes);

    if (num_bytes < 0)
        return -EFAULT;
    *ppos += num_bytes;
    return num_bytes;
}

module_init(leds_init);
module_exit(leds_exit);
MODULE_AUTHOR("Nhom_AtoZ");
MODULE_LICENSE("GPL");
