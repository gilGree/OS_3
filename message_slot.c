#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
// #include <linux/fs/filemap.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

#include "message_slot.h"

static message_channel *message_slots[MAX_MINOR_NUM];

// Helper linked list functions:

int free_list(message_channel *head)
{
    message_channel *curr = head;
    message_channel *next;

    while (curr != NULL)
    {
        next = curr->next;
        kfree(curr);
        curr = next;
    }

    head = NULL;
    return 0;
}
// Device Functions

static int device_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    int minor_num = iminor(inode);
    return free_list(message_slots[minor_num]);
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    message_channel *channel = (message_channel *)file->private_data;

    if (channel == NULL || channel->message == NULL)
        return -EWOULDBLOCK;

    if (channel->length > length)
        return -ENOSPC;

    return length - copy_to_user(buffer, channel->message, length);
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
    int bytes_written;
    message_channel *channel = (message_channel *)file->private_data;

    if (channel == NULL)
        return -EINVAL;

    if (length == 0 || length > BUF_SIZE)
        return -EMSGSIZE;

    bytes_written = length - copy_from_user(channel->message, buffer, length);
    channel->length = bytes_written;
    return bytes_written;
}

static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ioctl_param)
{
    int minor_num;
    message_channel *channel, *curr, *prev;
    if (ioctl_command_id != MSG_SLOT_CHANNEL)
    {
        printk("ERROR: ioctl command id is not valid \n");
        return -EINVAL;
    }

    if (ioctl_param == 0)
    {
        printk("ERROR: channel id is not valid \n");
        return -EINVAL;
    }

    minor_num = iminor(file_inode(file));

    curr = message_slots[minor_num];
    prev = curr;

    while (curr != NULL)
    {
        if (curr->channel_id == minor_num)
        {
            file->private_data = curr;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    channel = (message_channel *)kmalloc(sizeof(message_channel), GFP_KERNEL);
    channel->channel_id = minor_num;
    channel->next = NULL;

    if (prev == NULL)
    {
        prev = channel;
        file->private_data = prev;
        return 0;
    }

    prev->next = channel;
    file->private_data = channel;

    return 0;
}

// Device Setup

struct file_operations Fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .release = device_release,
};

static int __init simple_init(void)
{
    int rc = -1, i;

    // Register driver capabilities. Obtain major num
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);

    // Negative values signify an error
    if (rc < 0)
    {
        printk(KERN_ERR "%s registraion failed for  %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
        return rc;
    }

    printk("heyeyeeyeyyeyeyeyeyeyafaefaeh");
    for (i = 0; i < MAX_MINOR_NUM; i++)
    {
        message_slots[i] = NULL;
    }

    return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    int i;
    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
    for (i = 0; i < MAX_MINOR_NUM; i++)
    {
        free_list(message_slots[i]);
    }
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);