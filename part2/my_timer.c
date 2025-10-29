#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("COP4610 Project 2 Group 9");
MODULE_DESCRIPTION("my_timer kernel module");

#define ENTRY_NAME "timer"
#define PERMS 0444
#define PARENT NULL

#define BUF_LEN 256
static char msg[BUF_LEN];
static size_t msg_len;

static struct proc_dir_entry *proc_entry;
static struct timespec64 last_ts;
static bool last_valid = false;
static DEFINE_MUTEX(lock);

static ssize_t procfile_read(struct file *file, char __user *ubuf,
                             size_t count, loff_t *ppos)
{
    struct timespec64 now, delta;
    size_t len = 0;

    if (*ppos > 0){
        return 0;
    }
    mutex_lock(&lock);
    ktime_get_real_ts64(&now);
    len += scnprintf(msg + len, BUF_LEN - len, "current time: %lld.%09ld\n", (long long)now.tv_sec, (long)now.tv_nsec);
    if (last_valid) {
        // compute difference manually (delta = now - last_ts)
        delta.tv_sec = now.tv_sec - last_ts.tv_sec;
        delta.tv_nsec = now.tv_nsec - last_ts.tv_nsec;
        if (delta.tv_nsec < 0) {
            delta.tv_nsec += 1000000000L;
            delta.tv_sec -= 1;
        }

        len += scnprintf(msg + len, BUF_LEN - len,
                         "elapsed time: %lld.%09ld\n",
                         (long long)delta.tv_sec, (long)delta.tv_nsec);
    }
    last_ts = now;
    last_valid = true;
    msg_len = len;
    mutex_unlock(&lock);
    if (count < msg_len){
        return 0;
    }
    if (copy_to_user(ubuf, msg, msg_len)){
        return -EFAULT;
    }
    *ppos = msg_len;
    return msg_len;
}

static const struct proc_ops procfile_fops = {
    .proc_read = procfile_read,
};

//module init, creates proc entry
static int __init my_timer_init(void)
{
    proc_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &procfile_fops);
    if (!proc_entry){
        return -ENOMEM;
    }
    last_valid = false;
    return 0;
}
//module exit
static void __exit my_timer_exit(void)
{
    //removing proc entry
    if (proc_entry) {
        proc_remove(proc_entry);
        proc_entry = NULL;
    }
}

module_init(my_timer_init);
module_exit(my_timer_exit);
