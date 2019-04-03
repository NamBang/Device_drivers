#include<linux/module.h>/* dinh nghia cac marco cua module_init va module_exit*/
#include<linux/fs.h> /*thu vien nay dinh nghia cap phat va giai phong cac device drive*/
#include <linux/device.h>
#include<linux/slab.h>
#include"vchar_driver.h"

#define DRIVER_AUTHOR "Nam Bang"
#define DRIVER_DESC "A simple decive drive"
#define DRIVER_VERSION "0.1"

typedef struct vchar_dev{
	unsigned char *control_regs;
	unsigned char *status_regs;
	unsigned char *data_regs;
}vchar_dev_t;

struct _vchar_drv{
	dev_t dev_num;
	struct class *dev_class;
	struct device *dev;
	/*cap phat bo nho*/
	vchar_dev_t *vchar_hw;
}vchar_drv;

int vchar_hw_init(vchar_dev_t *hw)
{
	char *buf;
	buf = kzalloc(NUM_DEV_REGS *REG_SIZE,GFP_KERNEL);
	if(!buf)
	{
		return -ENOMEM;
	}

	hw->control_regs =buf;
	hw->status_regs = hw->control_regs + NUM_CTRL_REGS;
	hw->data_regs = hw->status_regs + NUM_STS_REGS;

	/* khoi tao gia tri ban dau*/

	hw->control_regs[CONTROL_ACCESS_REG]=0x03;
	hw->status_regs[DEVICE_STATUS_REG] =0x03;

	return 0;

}

void vchar_hw_exit(vchar_dev_t *hw)
{
	kfree(hw->control_regs);
}


static int __init vchar_driver_init(void)
{
	int ret = 0;
	int ret1=0;

	/* cap phat dong  device number*/

	vchar_drv.dev_num = 0;
	ret = alloc_chrdev_region(&vchar_drv.dev_num,0,1,"vchar_device");
	if(ret <0)
	{
		printk("fail to register divece number statically");
		goto failed_register_devnum;
	}

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


 	/*cap phat bo nho cho cau truc du lieu cua driver va khoi tao*/
 	vchar_drv.vchar_hw = kzalloc(sizeof(vchar_dev_t),GFP_KERNEL);
 	if(!vchar_drv.vchar_hw){
 		printk("failed to allocate data structure of the driver\n");
 		ret = -ENOMEM;
 		goto failed_allocate_structure;

 	}
 	/*khoi tao thiet bi vat ly*/
 	ret1 = vchar_hw_init(vchar_drv.vchar_hw);
 	if(ret1 <0){
 		printk("failed to initailize a virtual character device\n");
 		goto failed_init_hw;
 	}


    printk("initailize vchar driver successfully");
    return 0;

    failed_init_hw:
    kfree(vchar_drv.vchar_hw);

    failed_allocate_structure:
    device_destroy(vchar_drv.dev_class,vchar_drv.dev_num);

    failed_register_devnum:
    return ret;

    failed_create_device:
    class_destroy(vchar_drv.dev_class);

    failed_create_class:
    unregister_chrdev_region(vchar_drv.dev_num,1);
}	

static void __exit vchar_driver_exit(void)
{
	/*gia phong thiet bi vat ly*/
	vchar_hw_exit(vchar_drv.vchar_hw);

	/*giai phong bo nho da cap phat cau truc du lieu cua driver*/
	kfree(vchar_drv.vchar_hw);
	/* xoa bo device file*/
	device_destroy(vchar_drv.dev_class,vchar_drv.dev_num);
	class_destroy(vchar_drv.dev_class);
	/* giai phong device number*/
	unregister_chrdev_region(vchar_drv.dev_num,1);
	printk("exit vchar driver ");
}





module_init(vchar_driver_init);
module_exit(vchar_driver_exit);

MODULE_LICENSE("GPL"); /* giay phep su dung cua module */
MODULE_AUTHOR(DRIVER_AUTHOR); /* tac gia cua module */
MODULE_DESCRIPTION(DRIVER_DESC); /* mo ta chuc nang cua module */
MODULE_VERSION(DRIVER_VERSION); /* mo ta phien ban cuar module */
MODULE_SUPPORTED_DEVICE("testdevice"); /* kieu device ma module ho tro */