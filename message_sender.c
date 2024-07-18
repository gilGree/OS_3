#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message_slot.h"

#define MAJOR_NUM 235
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)

int main(int argc, char const *argv[]) {
    int file_desc, len_message;
    unsigned int channel_id;
    
    if (argc != 4) {
        perror("nonvalid number of arguments");
        exit(1);
    }
    file_desc = open(argv[1], O_WRONLY);
    if (file_desc == -1) {
        perror("failed to open device file \n");
        exit(1);
    }
    channel_id = atoi(argv[2]);
    if (ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id) < 0){
        perror("failed to set the channel id \n");
        exit(1);
    }
    len_message = strlen(argv[3]);
    if (write(file_desc, argv[3], len_message) != len_message) {
        perror("failed to write the message to the slot file \n");
        exit(1);
    }
    close(file_desc);
    exit(0);
}
