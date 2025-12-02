#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>       // 定義 file_operations
#include <linux/uaccess.h>  // 定義 copy_to_user, copy_from_user

#define DEVICE_NAME "simple_char"
#define BUF_LEN 1024

// [NEW] 定義我們的指令 (Magic Number)
// 這是為了避免跟其他 Driver 的指令衝突
#define MY_IOC_MAGIC 'k'
#define IOCTL_SET_LED _IOW(MY_IOC_MAGIC, 1, int) // 定義一個 "設定 LED" 的指令

// 全域變數
static int major_num;           // 系統分配的主設備號 (Major Number)
static char msg_buffer[BUF_LEN]; // 核心裡的資料緩衝區
static char *msg_ptr;
static int Device_Open = 0;     // 防止多個程式同時開啟 Driver
static int led_status = 0; // [NEW] 模擬硬體 LED 的狀態 (0:滅, 1:亮)

// 當 User 呼叫 open("/dev/simple_char") 時執行
static int device_open(struct inode *inode, struct file *file) {
    if (Device_Open) return -EBUSY; // 如果已經有人開了，就拒絕
    Device_Open++;
    
    // 讓 msg_ptr 指向 buffer 開頭，準備讀寫
    msg_ptr = msg_buffer;
    try_module_get(THIS_MODULE);
    return 0;
}

// 當 User 呼叫 close() 時執行
static int device_release(struct inode *inode, struct file *file) {
    Device_Open--;
    module_put(THIS_MODULE);
    return 0;
}

// 當 User 呼叫 read() / cat 時執行
// (面試必問：為什麼不能直接 return msg_buffer？)
static ssize_t device_read(struct file *filp, char __user *buffer, 
                           size_t length, loff_t *offset) {
    int bytes_read = 0;

    // 如果已經讀到結尾，就回傳 0
    if (*msg_ptr == 0) return 0;

    // 把資料從 Kernel Space 搬運到 User Space
    while (length && *msg_ptr) {
        // (面試重點) put_user 把一個 byte 搬給使用者
        put_user(*(msg_ptr++), buffer++);
        length--;
        bytes_read++;
    }
    return bytes_read;
}

// 當 User 呼叫 write() / echo 時執行
static ssize_t device_write(struct file *filp, const char __user *buffer, 
                            size_t length, loff_t *offset) {
    // int i;
    
    // 預防 Buffer Overflow (這也是資安重點)
    if (length > BUF_LEN - 1) length = BUF_LEN - 1;

    // (面試重點) 這裡必須用 copy_from_user，不能用 memcpy！
    // 因為 User 傳進來的指標可能是壞的，或者指向虛擬記憶體中沒 mapped 的區域
    if (copy_from_user(msg_buffer, buffer, length)) {
        return -EFAULT;
    }

    msg_buffer[length] = '\0'; // 補上字串結尾
    printk(KERN_INFO "simple_char: User wrote %zu bytes: %s\n", length, msg_buffer);
    
    return length;
}

// [NEW] 這是 ioctl 的實作
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case IOCTL_SET_LED:
            // arg 是使用者傳進來的參數 (0 或 1)
            if (arg == 1) {
                led_status = 1;
                printk(KERN_INFO "simple_char: [HARDWARE] LED is turned ON!\n");
                // 在真實世界，這裡會去寫入 GPIO Register
            } else {
                led_status = 0;
                printk(KERN_INFO "simple_char: [HARDWARE] LED is turned OFF!\n");
            }
            break;
        default:
            return -ENOTTY; // 指令錯誤
    }
    return 0;
}

// 定義這個 Driver 支援哪些操作
// 這就是 Linux "Everything is a file" 的靈魂
static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl, // [NEW] 註冊 ioctl
};

static int __init simple_char_init(void) {
    // 註冊一個字元裝置，參數 0 代表讓系統自動分配 Major Number
    major_num = register_chrdev(0, DEVICE_NAME, &fops);

    if (major_num < 0) {
        printk(KERN_ALERT "Registering char device failed with %d\n", major_num);
        return major_num;
    }

    printk(KERN_INFO "I was assigned major number %d. To talk to me:\n", major_num);
    printk(KERN_INFO "mknod /dev/%s c %d 0\n", DEVICE_NAME, major_num);
    return 0;
}

static void __exit simple_char_exit(void) {
    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "Goodbye from simple_char!\n");
}

module_init(simple_char_init);
module_exit(simple_char_exit);

MODULE_LICENSE("GPL");
