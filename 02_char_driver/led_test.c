#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // open
#include <unistd.h>     // close
#include <sys/ioctl.h>  // ioctl

// 必須跟 Driver 定義一樣的 Magic Number
#define MY_IOC_MAGIC 'k'
#define IOCTL_SET_LED _IOW(MY_IOC_MAGIC, 1, int)

int main(int argc, char *argv[]) {
    int fd;
    int ret;
    int cmd;

    if (argc < 2) {
        printf("Usage: %s <0|1>\n", argv[0]);
        printf("  1: Turn LED ON\n");
        printf("  0: Turn LED OFF\n");
        return -1;
    }

    // 1. 開啟裝置檔案
    fd = open("/dev/my_led", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    // 2. 讀取使用者輸入 (0 或 1)
    cmd = atoi(argv[1]);

    printf("Commanding Driver to set LED: %d...\n", cmd);

    // 3. 發送 ioctl 指令給 Driver
    ret = ioctl(fd, IOCTL_SET_LED, cmd);
    
    if (ret < 0) {
        perror("ioctl failed");
        close(fd);
        return -1;
    }

    printf("Success!\n");
    close(fd);
    return 0;
}
