#include <linux/init.h>      // for module init/exit macros
#include <linux/module.h>    // for all kernel module utilities
#include <linux/proc_fs.h>   // for proc file operations
#include <linux/uaccess.h>   // for copy_to_user and copy_from_user

MODULE_LICENSE("GPL");
MODULE_AUTHOR("COP4610 Group9");
MODULE_DESCRIPTION("A simple /proc hello world module");


static struct proc_dir_entry* proc_entry;

static int __init hello_init(void)
{
    proc_entry = proc_create("hello", 0666, NULL, &procfile_fops);
    return 0;
}

static void __exit hello_exit(void)
{
    proc_remove(proc_entry);
}

module_init(hello_init);
module_exit(hello_exit);


static ssize_t procfile_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char msg[] = "Hello from the kernel!\n";
    int len = strlen(msg);

    if (*ppos > 0 || count < len)
        return 0;

    if (copy_to_user(ubuf, msg, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

