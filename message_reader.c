#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>

#include "message_slot.h"

#define MAJOR_NUM 235
#define BUF_LEN 128
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)

int main(int argc, char const *argv[]) {
    int file_desc, len_message;
    unsigned int channel_id;
    char buffer_for_message[BUF_LEN];

    if (argc != 3) {
        perror("nonvalid number of arguments");
        exit(1);
    }
    file_desc = open(argv[1], O_RDONLY);
    if (file_desc == -1) {
        perror("failed to open device file \n");
        exit(1);
    }
    channel_id = atoi(argv[2]);
    if (ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id) < 0){
        perror("failed to set the channel id \n");
        exit(1);
    }
    len_message = read(file_desc, buffer_for_message, BUF_LEN);
    if (len_message < 0) {
        perror("failed to read from message slot file \n");
        exit(1);
    }
    close(file_desc);
    if (write(1, buffer_for_message, len_message) != len_message) {
        perror("failed to print the message\n");
        exit(1);
    }
    exit(0);
}
