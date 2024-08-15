#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE
#define BUF_LEN 128
#define SUCCESS 0
#define DEVICE_NAME "message_slot"
#define MAJOR_NUM 235
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#include "message_slot.h"

static channelList message_slot_device_files[257];

static int __init init_message_slot(void){
    int rc = -1;
    int i;
    rc = register_chrdev( MAJOR_NUM, DEVICE_NAME, &Fops );
    if( rc < 0 ) {
        printk( KERN_ERR "%s registration failed for  %d\n",
                DEVICE_NAME, MAJOR_NUM );
        return rc;
    }
    for (i = 0; i < 257; i++){
        message_slot_device_files[i].head = NULL;
    }
    printk( "registration is a successful \n");
    return SUCCESS;
}

static void __exit cleanup_message_slot(void){
    channelNode *cur_channel_node, *temp_node;
    int i;
    for (i = 0; i < 257; i++){
        cur_channel_node = message_slot_device_files[i].head;
        while (cur_channel_node != NULL){
            temp_node = cur_channel_node;
            cur_channel_node = cur_channel_node->next;
            kfree(temp_node);
        }
    }
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

struct file_operations Fops = {
        .owner = THIS_MODULE,
        .read = device_read,
        .write = device_write,
        .open = device_open,
        .unlocked_ioctl = device_ioctl,
};

static int device_open(struct inode *inode,struct file *file) {
    return SUCCESS;
}

static ssize_t device_read( struct file *file,char __user *buffer,size_t length,loff_t *offset){
    channelNode* cur_channel;
    int i, minor_num;
    if (buffer == NULL){
        printk("the buffer pointer is null.\n");
        return -EINVAL;
    }
    cur_channel = (channelNode *)file->private_data;
    minor_num = iminor(file->f_inode);
    if (cur_channel == NULL){
        printk("no channel assigned to file descriptor for this minor %d\n", minor_num);
        return -EINVAL;
    }
    if (cur_channel->len_message == 0){
        printk("no message is in channel %d of minor %d\n", cur_channel->channel_id, minor_num);
        return -EWOULDBLOCK;
    }
    if (cur_channel->len_message > length){
        printk("buffer length too small for last message in channel %d of minor %d\n", cur_channel->channel_id, minor_num);
        return -ENOSPC;
    }
    for (i = 0; i < cur_channel->len_message; i++){
        if (put_user(cur_channel->message[i], &buffer[i]) != 0){
            printk("put_user failed\n");
            return -EFAULT;
        }
    }
    printk("read message from channel %d of minor %d\n", cur_channel->channel_id, minor_num);
    return i;
}

static ssize_t device_write( struct file *file,const char __user *buffer,size_t length,loff_t *offset){
    channelNode* cur_channel;
    int i, minor_num;
    char temp_message[BUF_LEN];
    if (buffer == NULL){
        printk("buffer pointer is NULL\n");
        return -EINVAL;
    }
    minor_num = iminor(file->f_inode);
    cur_channel = (channelNode *)file->private_data;
    if (cur_channel == NULL){
        printk("no channel set for this file descriptor of minor %d\n", minor_num);
        return -EINVAL;
    }
    if (length == 0 || length > BUF_LEN){
        printk("the message length passed is either 0 or exceeds 128.\n");
        return -EMSGSIZE;
    }
    for (i = 0; i < length; i++){
        if (get_user(temp_message[i], &buffer[i]) != 0){
            printk("get_user failed\n");
            return -EFAULT;
        }
    }
    cur_channel->len_message = i;
    for (i = 0; i < cur_channel->len_message; i++){
        cur_channel->message[i] = temp_message[i];
}
    printk("write the message to channel %d of minor %d\n", cur_channel->channel_id, minor_num);
    return i;
}

static long device_ioctl( struct file* file,unsigned int ioctl_command_id,unsigned long ioctl_param){
    channelNode *cur_channel_node, *last_node;
    int minor_num;
    if (ioctl_param == 0){
        printk("channel id shouldn't be zero\n");
        return -EINVAL;
    }
    if (ioctl_command_id != MSG_SLOT_CHANNEL){
        printk("the command in ioctl is not MSG_SLOT_CHANNEL\n");
        return -EINVAL;
    }
    if (ioctl_param > UINT_MAX){
        printk("channel id exceeded unsigned int max value\n");
        return -EINVAL;
    }
    minor_num = iminor(file->f_inode);
    cur_channel_node = message_slot_device_files[minor_num].head;
    last_node = NULL;
    while (cur_channel_node != NULL){
        last_node = cur_channel_node;
        if(cur_channel_node->channel_id == ioctl_param){
            break;
        }
        cur_channel_node = cur_channel_node->next;
    }
    if (cur_channel_node == NULL){
        cur_channel_node = (channelNode *)kmalloc(sizeof(channelNode), GFP_KERNEL);
        if (cur_channel_node == NULL){
            printk("failing to allocate memory\n");
            return -ENOMEM;
        }
        if (last_node == NULL){
            message_slot_device_files[minor_num].head = cur_channel_node;
        }
        else{
            last_node->next = cur_channel_node;
        }
        cur_channel_node->channel_id = ioctl_param;
        cur_channel_node->len_message = 0;
        cur_channel_node->next = NULL;
    }
    file->private_data = cur_channel_node;
    printk("ioctl of channel %d of minor %d\n", cur_channel_node->channel_id, minor_num);
    return SUCCESS;
}

module_init(init_message_slot);
module_exit(cleanup_message_slot);
