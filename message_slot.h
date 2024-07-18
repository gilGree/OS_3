#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define BUF_LEN 128

typedef struct channelNode {
    struct channelNode *next;
    int len_message;
    unsigned int channel_id;
    char message[BUF_LEN];
} channelNode;

typedef struct channelList {
    channelNode *head;
} channelList;

#endif