#include<linux/module.h>/* dinh nghia cac marco cua module_init va module_exit*/
#include<linux/fs.h> /*thu vien nay dinh nghia cap phat va giai phong cac device drive*/

#define DRIVER_AUTHOR "Nam Bang"
#define DRIVER_DESC "A simple decive drive"
#define DRIVER_VERSION "0.1"
struct _vchar_drv{
	dev_t dev_num;
}vchar_drv;

static int __init vchar_driver_init(void)
{
	int ret = 0;

	/* cap phat dong  device number*/

	vchar_drv.dev_num = 0;
	ret = alloc_chrdev_region(&vchar_drv.dev_num,0,1,"vchar device1");
	if(ret <0)
	{
		printk("fail to register divece number statically");
		goto failed_register_devnum;
	}

	printk("alocated device number (%d,%d)\n",MAJOR(vchar_drv.dev_num),MINOR(vchar_drv.dev_num));
    
    printk("initailize vchar driver successfully");
    return 0;
    failed_register_devnum:
    return ret;
}

static void __exit vchar_driver_exit(void)
{
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