#if 1

/* general include */
#include <linux/init.h> /* __init, __exit, module_init(), module_exit() */
#include <linux/module.h> /* THIS_MODULE, MODULE_AUTHOR(), MODULE_LICENSE() */
#include <linux/platform_device.h> /* platform_driver_register(), platform_driver_unregister(), struct platform_driver */
#include <linux/printk.h> /* printk(), KERN_ALERT */
#include <linux/kernel.h> /* __FUNCTION__ */
#include <linux/fs.h> /* struct file_operations, struct inode, struct file */
#include <linux/types.h> /* size_t, loff_t */
#include <linux/compiler.h> /* __user */
/* specific device driver include */
#include <linux/miscdevice.h> /* misc_register(), misc_deregister(), struct miscdevice */



#define DEVICE_NAME    "HCL_BUTTON"
#define DRIVER_NAME    "HCL_BUTTON"



/* printk() information format definition */
#if 1
# define DPRINTK(x...)    printk(KERN_ALERT    DRIVER_NAME    ": "    x)
#else
# define DPRINTK(x...)
#endif



static int HCL_BUTTON_open(struct inode *inode, struct file *filp)
{
	DPRINTK("%s() ENTER\n", __FUNCTION__);

	DPRINTK("%s() LEAVE\n", __FUNCTION__);
	return 0;
}

static int HCL_BUTTON_release(struct inode *inode, struct file *filp)
{
	DPRINTK("%s() ENTER\n", __FUNCTION__);

	DPRINTK("%s() LEAVE\n", __FUNCTION__);
	return 0;
}

static ssize_t HCL_BUTTON_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	return 0;
}



static struct file_operations HCL_BUTTON_ops = {
	.owner          = THIS_MODULE,
	.open           = HCL_BUTTON_open,
	.release        = HCL_BUTTON_release,
	.read           = HCL_BUTTON_read
};

static struct miscdevice HCL_BUTTON_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &HCL_BUTTON_ops,
	.mode  = 0777
};

static int HCL_BUTTON_probe(struct platform_device *pdev)
{
	int ret;

	DPRINTK("%s() ENTER\n", __FUNCTION__);

	/* below should get resources needed */

	/* above should get resources needed */

	ret = misc_register(&HCL_BUTTON_dev);
	if (ret < 0) {
		/* below should release resources gotten in this function */

		/* above should release resources gotten in this function */
		goto out;
	}

out:
	DPRINTK("%s() LEAVE, ret = %d\n", __FUNCTION__, ret);
	return ret;
}

static int HCL_BUTTON_remove(struct platform_device *pdev)
{
	int ret;

	DPRINTK("%s() ENTER\n", __FUNCTION__);

	ret = misc_deregister(&itop4412_misc_dev);
	if (ret < 0) {
		DPRINTK("%s() misc_deregister() error! ret = %d\n", __FUNCTION__, ret);
		return ret;
	}

	/* below should release resources gotten in this function */

	/* above should release resources gotten in this function */

	DPRINTK("%s() LEAVE\n", __FUNCTION__);
	return 0;
}

static struct platform_driver HCL_BUTTON_driver = {
	.probe   = HCL_BUTTON_probe,
	.remove  = HCL_BUTTON_remove,
	.driver  = {
		.name  = DRIVER_NAME,
		.owner = THIS_MODULE
	}
};

static int __init HCL_BUTTON_init(void)
{
	int ret;

	DPRINTK("%s() ENTER\n", __FUNCTION__);

	ret = platform_driver_register(&HCL_BUTTON_driver);

	DPRINTK("%s() LEAVE, ret = %d\n", __FUNCTION__, ret);
	return ret;
}

static void __exit HCL_BUTTON_exit(void)
{
	DPRINTK("%s() ENTER\n", __FUNCTION__);

	platform_driver_unregister(&HCL_BUTTON_driver);

	DPRINTK("%s() LEAVE\n", __FUNCTION__);
}

module_init(HCL_BUTTON_init);
module_exit(HCL_BUTTON_exit);

MODULE_AUTHOR("He ChengLong");
MODULE_LICENSE("Dual BSD/GPL");

#else

/* general include */
#include <linux/init.h> /* __init, __exit, module_init(), module_exit() */
#include <linux/module.h> /* THIS_MODULE, MODULE_AUTHOR(), MODULE_LICENSE() */
#include <linux/moduleparam.h> /* module_param() */
#include <linux/stat.h> /* S_IRUGO */
#include <linux/kernel.h> /* container_of(), __FUNCTION__ */
#include <linux/printk.h> /* printk(), KERN_ALERT */
#include <linux/fs.h> /* struct inode, struct file, struct file_operations, register_chrdev_region(), alloc_chrdev_region(), unregister_chrdev_region() */
#include <linux/compiler.h> /* __user */
#include <linux/types.h> /* size_t, loff_t, ssize_t, dev_t */
#include <linux/slab.h> /* kzalloc(), kfree() */
#include <linux/gfp.h> /* GFP_KERNEL */
#include <linux/errno.h> /* ENOMEM */
#include <linux/uaccess.h> /* copy_to_user() */
/* specific device driver include */
#include <linux/cdev.h> /* struct cdev, cdev_init(), cdev_add(), cdev_del() */
#include <linux/kdev_t.h> /* MKDEV(), MAJOR() */
/* specific project include */
#include <linux/interrupt.h> /* request_irq(), free_irq() */
#include <linux/gpio.h> /* EXYNOS4_GPX1(0), gpio_request(), gpio_free(), gpio_get_value() */
#include <plat/gpio-cfg.h> /* s3c_gpio_setpull(), s3c_gpio_cfgpin() */
#include <linux/timer.h> /* struct timer_list, add_timer(), setup_timer(), del_timer() */
#include <linux/param.h> /* HZ */
#include <linux/jiffies.h> /* jiffies */
#include <linux/wait.h> /* wait_queue_head_t, init_waitqueue_head() */
#include <linux/sched.h> /* __set_current_state() */
/* uncertain include */
#include <linux/irq.h> /* request_irq(), IRQ_EINT(), IRQ_TYPE_EDGE_FALLING */



/* driver name(the name of the source file is normally used) */
#define DRIVER_NAME    "HCL_BUTTON"

/* printk() information format definition */
#if 1
# define DPRINTK(x...)    printk(KERN_ALERT    DRIVER_NAME    ": "    x)
#else
# define DPRINTK(x...)
#endif

/* major number(0: dynamically allocate; other value: static allocate) */
#define DRIVER_MAJOR    0

/* maximum number of device instances supported by the driver */
#define MAX_DEVICE_INSTANCE_AMOUNT    1



/* the millisecond delay macro definition for the timer */
#define TIMER_DELAY_MS_TIME(ms)    ((ms) * HZ / 1000)

/* button amount */
#define BUTTON_AMOUNT    1



/* button unique sequence number */
typedef enum
{
	BUTTON_UNIQUE_SEQUENCE_NUMBER_NONE,
	BUTTON_UNIQUE_SEQUENCE_NUMBER_0,
	BUTTON_UNIQUE_SEQUENCE_NUMBER_1
} BUTTON_UNIQUE_SEQUENCE_NUMBER;

/* identify the state of the button */
typedef enum
{
	BUTTON_PUSH_STATUS_UP,
	BUTTON_PUSH_STATUS_DOWN_JUST_NOW,
	BUTTON_PUSH_STATUS_DOWN
} BUTTON_PUSH_STATUS;

/* button information */
typedef struct
{
	BUTTON_UNIQUE_SEQUENCE_NUMBER button_unique_sequence_number;

	unsigned int  gpio_port;
	const char   *gpio_desc;

	int         irq_number;
	const char *irq_desc;

	BUTTON_PUSH_STATUS button_push_status;
	bool is_pushed;

	struct timer_list check_timer;
} BUTTON_INFO;

/* device struct */
struct HCL_BUTTON_dev
{
	struct cdev cdev;

	wait_queue_head_t read_wqh;

	BUTTON_INFO button_info[BUTTON_AMOUNT];
};



/* initial driver major number setting */
static int HCL_BUTTON_major = DRIVER_MAJOR;
module_param(HCL_BUTTON_major, int, S_IRUGO);

/* device struct pointer */
static struct HCL_BUTTON_dev *HCL_BUTTON_devp;



static int HCL_BUTTON_open(struct inode *inode, struct file *filp)
{
	struct HCL_BUTTON_dev *dev;

	/* using this method, multiple device instances are supported */
	dev = container_of(inode->i_cdev, struct HCL_BUTTON_dev, cdev);
	filp->private_data = dev;

	DPRINTK("%s() ENTER\n", __FUNCTION__);

	DPRINTK("%s() LEAVE\n", __FUNCTION__);
	return 0;
}

static int HCL_BUTTON_release(struct inode *inode, struct file *filp)
{
	//struct HCL_BUTTON_dev *dev = filp->private_data;

	DPRINTK("%s() ENTER\n", __FUNCTION__);

	DPRINTK("%s() LEAVE\n", __FUNCTION__);
	return 0;
}

static ssize_t HCL_BUTTON_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	int i;
	int err;
	bool is_retry = false;
	struct HCL_BUTTON_dev *dev = filp->private_data;
	DECLARE_WAITQUEUE(read_wq, current);

	DPRINTK("%s() ENTER\n", __FUNCTION__);

retry:
	if (is_retry) {
		add_wait_queue(&dev->read_wqh, &read_wq);

		__set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		set_current_state(TASK_RUNNING);

		if (signal_pending(current)) {
			err = -ERESTARTSYS;
			goto out;
		}
	}

	for (i = 0; i < BUTTON_AMOUNT; i++) {
		if (dev->button_info[i].is_pushed) {
			if (copy_to_user(buf, &dev->button_info[i].button_unique_sequence_number, sizeof(BUTTON_UNIQUE_SEQUENCE_NUMBER))) {
				err = -EFAULT;
				goto out;
			}
			dev->button_info[i].is_pushed = false;

			DPRINTK("%s() LEAVE, because button is pushed\n", __FUNCTION__);
			err = sizeof(BUTTON_UNIQUE_SEQUENCE_NUMBER);
			goto out;
		}
	}

	if (!(filp->f_flags & O_NONBLOCK)) {
		is_retry = true;
		goto retry;
	}

	DPRINTK("%s() LEAVE, because no button is pushed in O_NONBLOCK mode\n", __FUNCTION__);
	err = -EAGAIN;

out:
	/* even though no use of add_wait_queue() before, use it is still no problem */
	remove_wait_queue(&dev->read_wqh, &read_wq);

	return err;
}

static const struct file_operations HCL_BUTTON_fops = {
	.owner   = THIS_MODULE,
	.open    = HCL_BUTTON_open,
	.release = HCL_BUTTON_release,
	.read    = HCL_BUTTON_read,
};



static irqreturn_t HCL_BUTTON_irq_handler(int irq, void *dev)
{
	BUTTON_INFO *button_info = (BUTTON_INFO *)dev;

	DPRINTK("%s() ENTER: irq = %d, button_unique_sequence_number = %d, irq_number = %d\n", __FUNCTION__, irq, button_info->button_unique_sequence_number, button_info->irq_number);

	disable_irq_nosync(irq);

	button_info->button_push_status = BUTTON_PUSH_STATUS_DOWN_JUST_NOW;

	button_info->check_timer.expires = jiffies + TIMER_DELAY_MS_TIME(20);
	add_timer(&button_info->check_timer);

	DPRINTK("%s() LEAVE\n", __FUNCTION__);
	return IRQ_HANDLED;
}

static void HCL_BUTTON_timer_handler(unsigned long data)
{
	BUTTON_INFO *button_info = (BUTTON_INFO *)data;
	struct HCL_BUTTON_dev *dev = container_of(button_info, struct HCL_BUTTON_dev, button_info[button_info->button_unique_sequence_number - BUTTON_UNIQUE_SEQUENCE_NUMBER_0]);

	DPRINTK("%s() ENTER: button_unique_sequence_number = %d\n", __FUNCTION__, button_info->button_unique_sequence_number);

	if (gpio_get_value(button_info->gpio_port)) {
		DPRINTK("%s() now button is PUSH_UP\n", __FUNCTION__);

		button_info->button_push_status = BUTTON_PUSH_STATUS_UP;

		enable_irq(button_info->irq_number);
	} else {
		DPRINTK("%s() now button is PUSH_DOWN\n", __FUNCTION__);

		if (BUTTON_PUSH_STATUS_DOWN_JUST_NOW == button_info->button_push_status) {
			button_info->button_push_status = BUTTON_PUSH_STATUS_DOWN;
			button_info->is_pushed = true;
			wake_up_interruptible(&dev->read_wqh);
		}

		button_info->check_timer.expires = jiffies + TIMER_DELAY_MS_TIME(20);
		add_timer(&button_info->check_timer);
	}

	DPRINTK("%s() LEAVE\n", __FUNCTION__);
}



static int __init HCL_BUTTON_specific_setup(struct HCL_BUTTON_dev *dev)
{
	int i;
	int err;

	/* init for each button */
	dev->button_info[0].button_unique_sequence_number = BUTTON_UNIQUE_SEQUENCE_NUMBER_0;
	dev->button_info[0].gpio_port                     = EXYNOS4_GPX1(0);
	dev->button_info[0].gpio_desc                     = "HCL BUTTON 0 for GPIO";
	dev->button_info[0].irq_number                    = IRQ_EINT(8);
	dev->button_info[0].irq_desc                      = "HCL BUTTON 0 for IRQ";
	dev->button_info[0].button_push_status            = BUTTON_PUSH_STATUS_UP;
	dev->button_info[0].is_pushed                     = false;

	/* config each button */
	for (i = 0; i < BUTTON_AMOUNT; i++) {
		DPRINTK("%s() S1:: button_unique_sequence_number = %d, gpio_port = %d, irq_number = %d\n", __FUNCTION__, dev->button_info[i].button_unique_sequence_number, dev->button_info[i].gpio_port, dev->button_info[i].irq_number);

		err = gpio_request(dev->button_info[i].gpio_port, dev->button_info[i].gpio_desc);
		if (err) {
			DPRINTK("%s() S2:: gpio_request() FAIL!\n", __FUNCTION__);
			return err;
		}
		DPRINTK("%s() S2:: gpio_request() OK!\n", __FUNCTION__);

		err = s3c_gpio_setpull(dev->button_info[i].gpio_port, S3C_GPIO_PULL_UP);
		if (err) {
			gpio_free(dev->button_info[i].gpio_port);

			DPRINTK("%s() S3:: s3c_gpio_setpull() FAIL!\n", __FUNCTION__);
			return err;
		}
		DPRINTK("%s() S3:: s3c_gpio_setpull() OK!\n", __FUNCTION__);

		/* 0xF = WAKEUP_INTx[y] */
		err = s3c_gpio_cfgpin(dev->button_info[i].gpio_port, S3C_GPIO_SFN(0xF));
		if (err) {
			gpio_free(dev->button_info[i].gpio_port);

			DPRINTK("%s() S4:: s3c_gpio_cfgpin() FAIL!\n", __FUNCTION__);
			return err;
		}
		DPRINTK("%s() S4:: s3c_gpio_cfgpin() OK!\n", __FUNCTION__);

		gpio_free(dev->button_info[i].gpio_port);

		/* use it before request_irq(), because interrupt may happen */
		setup_timer(&dev->button_info[i].check_timer, HCL_BUTTON_timer_handler, (unsigned long)&dev->button_info[i]);

		err = request_irq(dev->button_info[i].irq_number,
			HCL_BUTTON_irq_handler,
			IRQ_TYPE_EDGE_FALLING,
			dev->button_info[i].irq_desc,
			&dev->button_info[i]);
		if (err) {
			DPRINTK("%s() S5:: request_irq() FAIL!\n", __FUNCTION__);

			(void)del_timer(&dev->button_info[i].check_timer);
			for (i = i - 1; i >= 0; i--) {
				free_irq(dev->button_info[i].irq_number, &dev->button_info[i]);
				(void)del_timer(&dev->button_info[i].check_timer);
			}

			return err;
		}
		DPRINTK("%s() S5:: request_irq() OK!\n", __FUNCTION__);
	}

	return err;
}

static int __init HCL_BUTTON_device_instance_setup(struct HCL_BUTTON_dev *dev, int index)
{
	int err;
	dev_t device_number = MKDEV(HCL_BUTTON_major, index);

	DPRINTK("%s() ENTER: device_number = 0x%x\n", __FUNCTION__, (unsigned int)device_number);

	cdev_init(&dev->cdev, &HCL_BUTTON_fops);
	dev->cdev.owner = THIS_MODULE;

	/* device specific init function could put below */
	init_waitqueue_head(&dev->read_wqh);

	err = HCL_BUTTON_specific_setup(dev);
	if (err) {
		return err;
	}
	/* device specific init function could put above */

	/*
			1. cdev_add() must be at the end in this function;
			2. it has handled error cases internally,
		so we don't need to handle errors externally.
	*/
	err = cdev_add(&dev->cdev, device_number, 1);
	DPRINTK("%s() LEAVE: cdev_add() err = %d\n", __FUNCTION__, err);

	return err;
}

static void __init HCL_BUTTON_device_instance_desetup(struct HCL_BUTTON_dev *dev, int index)
{
	DPRINTK("%s() ENTER\n", __FUNCTION__);

	cdev_del(&dev[index].cdev);

	DPRINTK("%s() LEAVE\n", __FUNCTION__);
}

static int __init HCL_BUTTON_init(void)
{
	int i;
	int result;
	dev_t device_number_start;

	DPRINTK("%s() S1:: major number of device = %d\n", __FUNCTION__, HCL_BUTTON_major);

	if (HCL_BUTTON_major) {
		device_number_start = MKDEV(HCL_BUTTON_major, 0);
		result = register_chrdev_region(device_number_start, MAX_DEVICE_INSTANCE_AMOUNT, DRIVER_NAME);
		DPRINTK("%s() S2:: register_chrdev_region() result = %d\n", __FUNCTION__, result);
	} else {
		result = alloc_chrdev_region(&device_number_start, 0, MAX_DEVICE_INSTANCE_AMOUNT, DRIVER_NAME);
		HCL_BUTTON_major = MAJOR(device_number_start);
		DPRINTK("%s() S2:: alloc_chrdev_region() result = %d\n", __FUNCTION__, result);
	}
	/*
			both register_chrdev_region() and alloc_chrdev_region(),
		they have handled error cases internally,
		so we don't need to handle errors externally.
	*/
	if (result) {
		DPRINTK("%s() S3:: major number of device = FAIL! so LEAVE!\n", __FUNCTION__);
		return result;
	}
	DPRINTK("%s() S3:: major number of device = %d\n", __FUNCTION__, HCL_BUTTON_major);

	HCL_BUTTON_devp = kzalloc(MAX_DEVICE_INSTANCE_AMOUNT * sizeof(struct HCL_BUTTON_dev), GFP_KERNEL);
	if (!HCL_BUTTON_devp) {
		DPRINTK("%s() S4:: HCL_BUTTON_devp = kzalloc() FAIL!\n", __FUNCTION__);
		result = -ENOMEM;
		goto fail;
	}
	DPRINTK("%s() S4:: HCL_BUTTON_devp = kzalloc() OK!\n", __FUNCTION__);

	for (i = 0; i < MAX_DEVICE_INSTANCE_AMOUNT; i++) {
		/*
				in a real project, if error happen,
			we should release resources in our user function,
		*/
		result = HCL_BUTTON_device_instance_setup(&HCL_BUTTON_devp[i], i);
		if (result) {
			for (i = i - 1; i >= 0; i--) {
				HCL_BUTTON_device_instance_desetup(&HCL_BUTTON_devp[i], i);
			}
			kfree(HCL_BUTTON_devp);
			goto fail;
		}
	}

	DPRINTK("%s() S5:: LEAVE because OK!\n", __FUNCTION__);
	return 0;

fail:
	unregister_chrdev_region(device_number_start, MAX_DEVICE_INSTANCE_AMOUNT);

	DPRINTK("%s() S5:: LEAVE because FAIL!\n", __FUNCTION__);
	return result;
}

static void __exit HCL_BUTTON_exit(void)
{
	int i;

	DPRINTK("%s() ENTER\n", __FUNCTION__);

	for (i = 0; i < MAX_DEVICE_INSTANCE_AMOUNT; i++) {
		cdev_del(&HCL_BUTTON_devp[i].cdev);
		free_irq(HCL_BUTTON_devp[i].button_info[i].irq_number, &HCL_BUTTON_devp[i].button_info[i]);
		(void)del_timer(&HCL_BUTTON_devp[i].button_info[i].check_timer);
	}
	kfree(HCL_BUTTON_devp);
	unregister_chrdev_region(MKDEV(HCL_BUTTON_major, 0), MAX_DEVICE_INSTANCE_AMOUNT);

	DPRINTK("%s() LEAVE\n", __FUNCTION__);
}

module_init(HCL_BUTTON_init);
module_exit(HCL_BUTTON_exit);

MODULE_AUTHOR("He ChengLong");
MODULE_LICENSE("Dual BSD/GPL");

#endif

