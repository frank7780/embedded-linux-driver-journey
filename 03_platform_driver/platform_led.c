#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h> // [NEW] Platform Driver 核心
#include <linux/device.h>          // [NEW] 自動建立 /dev 節點用
#include <linux/ioctl.h>
#include <linux/version.h>  // <--- 請加入這一行！ (FIX HERE)

#define DRIVER_NAME "my_led_driver"
#define DEVICE_NODE_NAME "my_led" // 將來在 /dev/my_led 看到的名稱

// --- IOCTL 定義 (跟之前一樣) ---
#define MY_IOC_MAGIC 'k'
#define IOCTL_SET_LED _IOW(MY_IOC_MAGIC, 1, int)

// --- 全域變數 ---
static int major_num;
static struct class *my_class;    // [NEW] 用來自動產生 /dev 節點的類別
static struct device *my_device;  // [NEW] 指向自動產生的裝置

// --- File Operations (跟之前邏輯一樣) ---
static int led_status = 0;

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case IOCTL_SET_LED:
            if (arg == 1) {
                led_status = 1;
                printk(KERN_INFO "PlatformLED: LED ON!\n");
            } else {
                led_status = 0;
                printk(KERN_INFO "PlatformLED: LED OFF!\n");
            }
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

static int my_open(struct inode *inode, struct file *file) {
    return 0;
}

static int my_close(struct inode *inode, struct file *file) {
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .unlocked_ioctl = my_ioctl,
};

// --- [NEW] Platform Driver 的核心：Probe 函式 ---
// 當 Kernel 發現有裝置名稱叫做 "my_led_driver" 時，會呼叫這個函式
static int my_probe(struct platform_device *pdev) {
    printk(KERN_INFO "PlatformLED: Device found! Probing...\n");

    // 1. 註冊字元裝置 (取得 Major Number)
    major_num = register_chrdev(0, DRIVER_NAME, &fops);
    if (major_num < 0) {
        printk(KERN_ALERT "PlatformLED: Failed to register char device\n");
        return major_num;
    }

    // 2. [NEW] 自動建立 /dev/my_led 節點 (不用再手動 mknod 了！)
    // 建立一個 Class
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    my_class = class_create("my_led_class"); // Linux 6.4+ 不需要 THIS_MODULE
#else
    my_class = class_create(THIS_MODULE, "my_led_class");
#endif

    if (IS_ERR(my_class)) {
        unregister_chrdev(major_num, DRIVER_NAME);
        return PTR_ERR(my_class);
    }

    // 在這個 Class 下建立裝置 (這步會讓 udev 自動在 /dev 產生檔案)
    my_device = device_create(my_class, NULL, MKDEV(major_num, 0), NULL, DEVICE_NODE_NAME);
    if (IS_ERR(my_device)) {
        class_destroy(my_class);
        unregister_chrdev(major_num, DRIVER_NAME);
        return PTR_ERR(my_device);
    }

    printk(KERN_INFO "PlatformLED: Probe success! /dev/%s created.\n", DEVICE_NODE_NAME);
    return 0;
}

// --- [NEW] Remove 函式 ---
// 當 Driver 被卸載，或裝置被移除時呼叫
static int my_remove(struct platform_device *pdev) {
    printk(KERN_INFO "PlatformLED: Removing driver...\n");

    // 依序銷毀：Device -> Class -> CharDev
    device_destroy(my_class, MKDEV(major_num, 0));
    class_destroy(my_class);
    unregister_chrdev(major_num, DRIVER_NAME);
    
    return 0;
}

// 定義 Driver 結構
static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = DRIVER_NAME, // [重點] 這個名字必須跟 Device 的名字一樣
        .owner = THIS_MODULE,
    },
};

// --- 模擬硬體的部分 (通常這會寫在 Device Tree 或另一個模組，這裡為了方便寫在一起) ---
static struct platform_device *my_pdev;

static int __init my_init(void) {
    int ret;
    printk(KERN_INFO "PlatformLED: Module loading...\n");

    // 1. 先註冊 Driver
    ret = platform_driver_register(&my_driver);
    if (ret) return ret;

    // 2. [模擬] 註冊一個 Platform Device
    // 讓 Kernel 以為真的有一個硬體插上去了，這樣才會觸發 probe
    my_pdev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
    if (IS_ERR(my_pdev)) {
        platform_driver_unregister(&my_driver);
        return PTR_ERR(my_pdev);
    }

    return 0;
}

static void __exit my_exit(void) {
    // 卸載時，先移除 Device，再移除 Driver
    platform_device_unregister(my_pdev);
    platform_driver_unregister(&my_driver);
    printk(KERN_INFO "PlatformLED: Module unloaded.\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Frank");
