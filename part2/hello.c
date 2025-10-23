#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define ENTRY_NAME "hello"
#define PERMS 0666
#define PARENT NULL
#define BUF_LEN 100

static struct proc_dir_entry* proc_entry;   // pointer to the /proc entry
static char msg[BUF_LEN] = "Hello from the kernel!\n";
static int procfs_buf_len;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("COP4610 Group 9");
MODULE_DESCRIPTION("A simple Linux kernel module");

static ssize_t procfile_read(struct file* file, char* ubuf, size_t count, loff_t* ppos)
{
    printk(KERN_INFO "proc_read\n");
    procfs_buf_len = strlen(msg);

    // Only send data once per read
    if (*ppos > 0 || count < procfs_buf_len)
        return 0;

    if (copy_to_user(ubuf, msg, procfs_buf_len))
        return -EFAULT;

    *ppos = procfs_buf_len;
    printk(KERN_INFO "gave to user %s\n", msg);
    return procfs_buf_len;
}

static const struct proc_ops procfile_fops = {
    .proc_read = procfile_read,
};

static int __init hello_init(void)
{
    proc_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &procfile_fops);
    if (proc_entry == NULL)
        return -ENOMEM;

    printk(KERN_INFO "/proc/%s created\n", ENTRY_NAME);
    return 0;
}

static void __exit hello_exit(void)
{
    proc_remove(proc_entry);
    printk(KERN_INFO "/proc/%s removed\n", ENTRY_NAME);
}

module_init(hello_init);
module_exit(hello_exit);
