#include <linux/cdev.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/printk.h>
#include <linux/string.h> /* memset()??? */
                          /* EINVAL??? */
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/uaccess.h> /* copy_to_user()??? */
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/gfp.h>

#include <linux/ioctl.h>

#include <linux/semaphore.h>

#include <linux/wait.h>

#include <linux/thread_info.h> /* current */

#include <linux/sched.h>

#include <linux/poll.h>

#include <linux/signal.h>

#include <linux/aio.h>
#include <linux/uio.h>
#include <linux/workqueue.h>
#include <linux/errno.h>



#if 0
/* infer to ioctl-number.txt */
#define GLOBALFIFO_MAGIC    'x'
/* ioctl() cmd */
#define GLOBALFIFO_IOCTL_BUF_CLEAR    _IO(GLOBALFIFO_MAGIC, 0)
#else
#define GLOBALFIFO_IOCTL_BUF_CLEAR    0x01
#endif

/* buffer size */
#define GLOBALFIFO_BUF_SIZE    (60)

/* major number */
#define GLOBALFIFO_MAJOR    0

/* supported max devices amount of the driver */
#define GLOBALFIFO_MAX_DEVICES_AMOUNT    2



/* used for aio_read and aio_write */
struct aio_work
{
	struct kiocb       *iocb;
	int                 result;
	struct work_struct  work;
};

/* device struct */
struct globalfifo_dev
{
	struct cdev    cdev;

	unsigned int     read_position;
	unsigned int     write_position;
	unsigned char    mem[GLOBALFIFO_BUF_SIZE];

	struct semaphore    sem;

	wait_queue_head_t    w_wait;
	wait_queue_head_t    r_wait;

	struct fasync_struct    *fasync_queue;
};



static int globalfifo_major = GLOBALFIFO_MAJOR;
module_param(globalfifo_major, int, S_IRUGO);

/* device struct pointer */
static struct globalfifo_dev *globalfifo_devp;



static long globalfifo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct globalfifo_dev *dev = filp->private_data;

	printk(KERN_ALERT "HCL: globalfifo_ioctl() S1:: ENTER\n");

	switch (cmd) {
	case GLOBALFIFO_IOCTL_BUF_CLEAR:
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;

		dev->write_position = 0;
		dev->read_position = 0;

		up(&dev->sem);

		printk(KERN_ALERT "HCL: globalfifo_ioctl() S2:: globalfifo resets OK!\n");
		break;
	default:
		return -EINVAL;
	}

	printk(KERN_ALERT "HCL: globalfifo_ioctl() S3:: LEAVE\n");

	return 0;
}

/* HCL report problem:
        if (down_interruptible(&dev->sem)) {
            goto out;
        }
        if (true), then the code will make select() error and system error,
    I do not know how to deal with it.
*/
static unsigned int globalfifo_poll(struct file *filp, struct poll_table_struct *poll_table)
{
	unsigned int           mask = 0;
	struct globalfifo_dev *dev = filp->private_data;

	printk(KERN_ALERT "HCL: globalfifo_poll() S1:: ENTER\n");
	printk(KERN_ALERT "HCL: globalfifo_poll() S2:: filp->f_mode = 0x%x, filp->f_flags = 0x%x\n", filp->f_mode, filp->f_flags);

	if (down_interruptible(&dev->sem)) {
		goto out;
	}

	poll_wait(filp, &dev->r_wait, poll_table);
	poll_wait(filp, &dev->w_wait, poll_table);

	/* FIFO have data to be read */
	if (dev->read_position != dev->write_position) {
		mask |= POLLIN | POLLRDNORM;
	}
	/* FIFO have space to be written */
	if (!((dev->write_position + 1 == dev->read_position) || ((dev->read_position == 0) && (dev->write_position == GLOBALFIFO_BUF_SIZE - 1)))) {
		mask |= POLLOUT | POLLWRNORM;
	}

	up(&dev->sem);

out:
	printk(KERN_ALERT "HCL: globalfifo_poll() S3:: LEAVE\n");

	return mask;
}

static int globalfifo_fasync(int fd, struct file *filp, int mode)
{
	struct globalfifo_dev *dev = filp->private_data;

	return fasync_helper(fd, filp, mode, &dev->fasync_queue);
}

static int globalfifo_open(struct inode *inode, struct file *filp)
{
	struct globalfifo_dev *dev;

	printk(KERN_ALERT "HCL: globalfifo_open() S1:: ENTER\n");
	printk(KERN_ALERT "HCL: globalfifo_open() S2:: filp->f_mode = 0x%x, filp->f_flags = 0x%x\n", filp->f_mode, filp->f_flags);

	/* using this method, multiple devices are supported */
	dev = container_of(inode->i_cdev, struct globalfifo_dev, cdev);
	filp->private_data = dev;

	printk(KERN_ALERT "HCL: globalfifo_open() S3:: LEAVE\n");

	return 0;
}

static int globalfifo_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "HCL: globalfifo_release() S1:: ENTER\n");

	globalfifo_fasync(-1, filp, 0);

	printk(KERN_ALERT "HCL: globalfifo_release() S2:: LEAVE\n");

	return 0;
}

static ssize_t globalfifo_write(struct file       *filp,
                                const char __user *buf,
                                size_t             size,
                                loff_t            *ppos)
{
	int                    ret;
	size_t                 wait_write_normal_size;
	size_t                 wait_write_wrapped_size;
	struct globalfifo_dev *dev = filp->private_data;
	DECLARE_WAITQUEUE(write_wait, current);

	printk(KERN_ALERT "HCL: globalfifo_write() S1:: ENTER\n");

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	add_wait_queue(&dev->w_wait, &write_wait);

restart:
	printk(KERN_ALERT "HCL: globalfifo_write() S2:: dev->write_position = %d, dev->read_position = %d\n", dev->write_position, dev->read_position);

	/* maybe need twice write */
	if (dev->write_position >= dev->read_position) {
		wait_write_normal_size = (size_t)(GLOBALFIFO_BUF_SIZE - dev->write_position);
		/* need once write: case 1 */
		if (wait_write_normal_size > size) {
			wait_write_normal_size = size;

			wait_write_wrapped_size = 0;
		/* need once write: case 2 */
		} else if (wait_write_normal_size == size) {
			if (!dev->read_position)
				wait_write_normal_size -= 1;

			wait_write_wrapped_size = 0;
		/* maybe need twice write */
		} else {
			/* need once write */
			if (!dev->read_position) {
				wait_write_normal_size -= 1;
				
				wait_write_wrapped_size = 0;
			/* need twice write */
			} else {
				wait_write_wrapped_size = size - wait_write_normal_size;
				if (wait_write_wrapped_size >= dev->read_position)
					wait_write_wrapped_size = (size_t)(dev->read_position - 1);
			}
		}
	/* need once write */
	} else {
		wait_write_normal_size = (size_t)(dev->read_position - dev->write_position);
		if (wait_write_normal_size > size)
			wait_write_normal_size = size;
		else
			wait_write_normal_size -= 1;

		wait_write_wrapped_size = 0;
	}
	printk(KERN_ALERT "HCL: globalfifo_write() S3:: aim to write %d bytes, actually, wait_write_normal_size = %d, wait_write_wrapped_size = %d\n", (int)size, (int)wait_write_normal_size, (int)wait_write_wrapped_size);

	/* no enough fifo space */
	if (size != (wait_write_normal_size + wait_write_wrapped_size)) {
		if (O_NONBLOCK & filp->f_flags) {
			ret = -EAGAIN;
			goto out1;
		}

		__set_current_state(TASK_INTERRUPTIBLE);

		up(&dev->sem);

		schedule();

		set_current_state(TASK_RUNNING);

		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto out2;
		}

		if (down_interruptible(&dev->sem)) {
			ret = -ERESTARTSYS;
			goto out2;
		}

		goto restart;
	}
	
	if (copy_from_user(dev->mem + dev->write_position, buf, wait_write_normal_size)) {
		ret = -EFAULT;
		goto out1;
	} else {
		dev->write_position += (unsigned int)wait_write_normal_size;
		if (GLOBALFIFO_BUF_SIZE == dev->write_position)
			dev->write_position = 0;
		ret = (int)wait_write_normal_size;
		printk(KERN_ALERT "HCL: globalfifo_write()->copy_from_user() S4-1:: copy wait_write_normal_size(%d) bytes OK! dev->write_position = %d\n", (int)wait_write_normal_size, dev->write_position);

		wake_up_interruptible(&dev->r_wait);

		if (dev->fasync_queue)
			kill_fasync(&dev->fasync_queue, SIGIO, POLL_IN);

		if (wait_write_wrapped_size) {
			if (copy_from_user(dev->mem, buf + wait_write_normal_size, wait_write_wrapped_size)) {
				goto out1;
			} else {
				dev->write_position = (unsigned int)wait_write_wrapped_size;
				ret = (int)(wait_write_normal_size + wait_write_wrapped_size);
				printk(KERN_ALERT "HCL: globalfifo_write()->copy_from_user() S4-2:: copy wait_write_wrapped_size(%d) bytes OK! dev->write_position = %d\n", (int)wait_write_wrapped_size, dev->write_position);
			}
		}
	}

out1:
	up(&dev->sem);

out2:
	remove_wait_queue(&dev->w_wait, &write_wait);

	printk(KERN_ALERT "HCL: globalfifo_write() S5:: LEAVE\n");

	return ret;
}

static ssize_t globalfifo_read(struct file *filp,
                               char __user *buf,
                               size_t       size,
                               loff_t      *ppos)
{
	int                    ret;
	size_t                 wait_read_normal_size;
	size_t                 wait_read_wrapped_size;
	struct globalfifo_dev *dev = filp->private_data;
	DECLARE_WAITQUEUE(read_wait, current);

	printk(KERN_ALERT "HCL: globalfifo_read() S1:: ENTER\n");

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	add_wait_queue(&dev->r_wait, &read_wait);

restart:
	printk(KERN_ALERT "HCL: globalfifo_read() S2:: dev->write_position = %d, dev->read_position = %d\n", dev->write_position, dev->read_position);

	/* no content to read */
	if (dev->read_position == dev->write_position) {
		if (O_NONBLOCK & filp->f_flags) {
			ret = 0;
			goto out1;
		}

		__set_current_state(TASK_INTERRUPTIBLE);

		up(&dev->sem);

		schedule();

		set_current_state(TASK_RUNNING);

		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto out2;
		}

		if (down_interruptible(&dev->sem)) {
			ret = -ERESTARTSYS;
			goto out2;
		}

		goto restart;
	/* need once read */
	} else if (dev->write_position > dev->read_position) {
		wait_read_normal_size = (size_t)(dev->write_position - dev->read_position);
		if (wait_read_normal_size > size)
			wait_read_normal_size = size;

		wait_read_wrapped_size = 0;
	/* maybe need twice read */
	} else {
		wait_read_normal_size = (size_t)(GLOBALFIFO_BUF_SIZE - dev->read_position);
		/* still need once read */
		if (wait_read_normal_size >= size) {
			wait_read_normal_size = size;

			wait_read_wrapped_size = 0;
		/* need twice read */
		} else {
			wait_read_wrapped_size = size - wait_read_normal_size;
			if (wait_read_wrapped_size > (size_t)dev->write_position)
				wait_read_wrapped_size = (size_t)dev->write_position;
		}
	}
	printk(KERN_ALERT "HCL: globalfifo_read() S3:: aim to read %d bytes, actually, wait_read_normal_size = %d, wait_read_wrapped_size = %d\n", (int)size, (int)wait_read_normal_size, (int)wait_read_wrapped_size);

	if (copy_to_user(buf, (void *)(dev->mem + dev->read_position), wait_read_normal_size)) {
		ret = -EFAULT;
		goto out1;
	} else {
		dev->read_position += (unsigned int)wait_read_normal_size;
		if (GLOBALFIFO_BUF_SIZE == dev->read_position)
			dev->read_position = 0;
		ret = (int)wait_read_normal_size;
		printk(KERN_ALERT "HCL: globalfifo_read()->copy_to_user() S4-1:: copy wait_read_normal_size(%d) bytes OK! dev->read_position = %d\n", (int)wait_read_normal_size, dev->read_position);

		wake_up_interruptible(&dev->w_wait);

		if (wait_read_wrapped_size) {
			if (copy_to_user(buf + wait_read_normal_size, (void *)dev->mem, wait_read_wrapped_size)) {
				goto out1;
			} else {
				dev->read_position = (unsigned int)wait_read_wrapped_size;
				ret = (int)(wait_read_normal_size + wait_read_wrapped_size);
				printk(KERN_ALERT "HCL: globalfifo_read()->copy_to_user() S4-2:: copy wait_read_wrapped_size(%d) bytes OK! dev->read_position = %d\n", (int)wait_read_wrapped_size, dev->read_position);
			}
		}
	}

out1:
	up(&dev->sem);

out2:
	remove_wait_queue(&dev->w_wait, &read_wait);

	printk(KERN_ALERT "HCL: globalfifo_read() S5:: LEAVE\n");

	return ret;
}

#if 1 /* aio_read and aio_write is waiting for further study */
static void globalfifo_do_deferred_op(struct work_struct *p)
{
	struct aio_work *aio_wk = container_of(p, struct aio_work, work);;

	aio_complete(aio_wk->iocb, aio_wk->result, 0);
	kfree(aio_wk);
}

static int globalfifo_defer_op(int           write,
                               struct kiocb *iocb,
                               char         *buf,
                               size_t        count,
                               loff_t        pos)
{
	struct aio_work *aio_wk;
	int              result;

	if (write)
		result = globalfifo_write(iocb->ki_filp, buf, count, &pos);
	else
		result = globalfifo_read(iocb->ki_filp, buf, count, &pos);

	if (is_sync_kiocb(iocb))
		return result;

	aio_wk = kmalloc(sizeof(*aio_wk), GFP_KERNEL);
	if (!aio_wk)
		return result;

	aio_wk->iocb = iocb;
	aio_wk->result = result;
	INIT_WORK(&aio_wk->work, globalfifo_do_deferred_op);
	schedule_work(&aio_wk->work);

	return -EIOCBQUEUED;
}

static ssize_t globalfifo_aio_read(struct kiocb       *iocb,
                                   const struct iovec *iov,
                                   unsigned long       nr_segs,
                                   loff_t              pos)
{
	int result;

	printk(KERN_ALERT "HCL: globalfifo_aio_read() S1:: ENTER\n");

	/* now only deal with iov[0] */
	result = globalfifo_defer_op(0, iocb, iov[0].iov_base, iov[0].iov_len, pos);

	printk(KERN_ALERT "HCL: globalfifo_aio_read() S2:: LEAVE\n");

	return (ssize_t)result;
}

static ssize_t globalfifo_aio_write(struct kiocb       *iocb,
                                    const struct iovec *iov,
                                    unsigned long       nr_segs,
                                    loff_t              pos)
{
	int result;

	printk(KERN_ALERT "HCL: globalfifo_aio_write() S1:: ENTER\n");

	/* now only deal with iov[0] */
	result = globalfifo_defer_op(1, iocb, iov[0].iov_base, iov[0].iov_len, pos);

	printk(KERN_ALERT "HCL: globalfifo_aio_write() S2:: LEAVE\n");

	return (ssize_t)result;
}
#endif

static const struct file_operations globalfifo_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = globalfifo_ioctl,
	.poll           = globalfifo_poll,
	.fasync         = globalfifo_fasync,
	.open           = globalfifo_open,
	.release        = globalfifo_release,
	.write          = globalfifo_write,
	.read           = globalfifo_read,
#if 1
	.aio_read       = globalfifo_aio_read,
	.aio_write      = globalfifo_aio_write,
#endif
};

static int __init globalfifo_setup_cdev(struct globalfifo_dev *dev, int index)
{
	int   err;
	dev_t device_number;

	device_number = MKDEV(globalfifo_major, index);
	printk(KERN_ALERT "HCL: globalfifo_init()->globalfifo_setup_cdev() S1:: device_number = 0x%x\n", (unsigned int)device_number);

	cdev_init(&dev->cdev, &globalfifo_fops);
	dev->cdev.owner = THIS_MODULE;

	sema_init(&dev->sem, 1);

	init_waitqueue_head(&dev->w_wait);
	init_waitqueue_head(&dev->r_wait);

	/* cdev_add() must be last */
	err = cdev_add(&dev->cdev, device_number, 1);
	printk(KERN_ALERT "HCL: globalfifo_init()->globalfifo_setup_cdev() S2:: cdev_add() err = %d\n", err);
	
	return err;
}

/* HCL report problem:
        if globalfifo_major = 0, in globalfifo_init() MUST use "globalfifo_major = 0;",
    or it will be a uncertain large number, I do not know why.
        if do not do above, you can also use "insmod globalfifo.ko globalfifo_major=0" in command line.
        I guess the proble is because of "insmod" problem......just a guss, waiting to know future.
*/
static int __init globalfifo_init(void)
{
	int   i;
	int   result;
	dev_t device_start_number;

	printk(KERN_ALERT "HCL: globalfifo_init() S1:: globalfifo_major = %d\n", globalfifo_major);
	//globalfifo_major = 0; //??????????????????

	if (globalfifo_major) {
		device_start_number = MKDEV(globalfifo_major, 0);
		result = register_chrdev_region(device_start_number, GLOBALFIFO_MAX_DEVICES_AMOUNT, "globalfifo");
		printk(KERN_ALERT "HCL: globalfifo_init() S2:: register_chrdev_region() result = %d\n", result);
	} else {
		result = alloc_chrdev_region(&device_start_number, 0, GLOBALFIFO_MAX_DEVICES_AMOUNT, "globalfifo");
		globalfifo_major = MAJOR(device_start_number);
		printk(KERN_ALERT "HCL: globalfifo_init() S2:: alloc_chrdev_region() result = %d\n", result);
	}
	if (result < 0)
		return result;
	printk(KERN_ALERT "HCL: globalfifo_init() S3:: globalfifo_major = %d\n", globalfifo_major);

	globalfifo_devp = kzalloc(GLOBALFIFO_MAX_DEVICES_AMOUNT * sizeof(struct globalfifo_dev), GFP_KERNEL);
	if (!globalfifo_devp) {
		result = -ENOMEM;
		goto fail1;
	}
	printk(KERN_ALERT "HCL: globalfifo_init() S4:: globalfifo_devp = kzalloc() OK!\n");

	for (i = 0; i < GLOBALFIFO_MAX_DEVICES_AMOUNT; i++) {
		result = globalfifo_setup_cdev(&globalfifo_devp[i], i);
		/* HCL report attention:
				if error happen, in a real project,
			we should release resources in XXX_setup_cdev(),
			but here is only a sample, so DO NOT deal with error case.
		*/
		if (result)
			goto fail1;
	}

	printk(KERN_ALERT "HCL: globalfifo_init() S5:: LEAVE\n");
	return 0;

fail1:
	unregister_chrdev_region(device_start_number, GLOBALFIFO_MAX_DEVICES_AMOUNT);

	printk(KERN_ALERT "HCL: globalfifo_init() SERROR:: LEAVE\n");

	return result;
}

static void __exit globalfifo_exit(void)
{
	int i;

	printk(KERN_ALERT "HCL: globalfifo_exit() S1:: ENTER\n");

	for (i = 0; i < GLOBALFIFO_MAX_DEVICES_AMOUNT; i++)
		cdev_del(&globalfifo_devp[i].cdev);
	kfree(globalfifo_devp);
	unregister_chrdev_region(MKDEV(globalfifo_major, 0), GLOBALFIFO_MAX_DEVICES_AMOUNT);

	printk(KERN_ALERT "HCL: globalfifo_exit() S2:: LEAVE\n");
}

module_init(globalfifo_init);
module_exit(globalfifo_exit);

MODULE_AUTHOR("He ChengLong");
MODULE_LICENSE("Dual BSD/GPL");
