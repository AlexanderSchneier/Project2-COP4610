#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/ktime.h>

#define ENTRY_NAME "timer"
#define PERMS 0666
#define PARENT NULL

static struct proc_dir_entry* proc_entry;
static struct timespec64 last_time;
static int first_read = 1;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("COP4610 Group 9");
MODULE_DESCRIPTION("A kernel module that tracks elapsed time");

static ssize_t procfile_read(struct file* file, char __user* ubuf, size_t count, loff_t* ppos)
{
    char buf[256];
    int len;
    struct timespec64 current_time;
    long long elapsed_sec;
    long elapsed_nsec;
    
    printk(KERN_INFO "proc_read\n");
    
    // Only send data once per read
    if (*ppos > 0)
        return 0;
    
    // Get current time
    ktime_get_real_ts64(&current_time);
    
    if (first_read) {
        // First read: only print current time
        len = snprintf(buf, sizeof(buf), "current time: %lld.%09ld\n",
                      (long long)current_time.tv_sec, current_time.tv_nsec);
        first_read = 0;
    } else {
        // Calculate elapsed time
        elapsed_sec = current_time.tv_sec - last_time.tv_sec;
        elapsed_nsec = current_time.tv_nsec - last_time.tv_nsec;
        
        // Handle nanosecond underflow
        if (elapsed_nsec < 0) {
            elapsed_sec--;
            elapsed_nsec += 1000000000;
        }
        
        len = snprintf(buf, sizeof(buf), "current time: %lld.%09ld\nelapsed time: %lld.%09ld\n",
                      (long long)current_time.tv_sec, current_time.tv_nsec,
                      elapsed_sec, elapsed_nsec);
    }
    
    // Store current time as last time for next read
    last_time = current_time;
    
    if (count < len)
        return 0;
    
    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;
    
    *ppos = len;
    printk(KERN_INFO "gave to user current time\n");
    
    return len;
}

static const struct proc_ops procfile_fops = {
    .proc_read = procfile_read,
};

static int __init timer_init(void)
{
    printk(KERN_INFO "my_timer: initializing module\n");
    
    proc_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &procfile_fops);
    if (proc_entry == NULL) {
        printk(KERN_ALERT "my_timer: failed to create /proc/%s\n", ENTRY_NAME);
        return -ENOMEM;
    }
    
    first_read = 1;
    printk(KERN_INFO "my_timer: /proc/%s created\n", ENTRY_NAME);
    
    return 0;
}

static void __exit timer_exit(void)
{
    proc_remove(proc_entry);
    printk(KERN_INFO "my_timer: /proc/%s removed\n", ENTRY_NAME);
}

module_init(timer_init);
module_exit(timer_exit);