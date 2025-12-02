#include <linux/init.h>   // 必須：定義 module_init/exit
#include <linux/module.h> // 必須：定義 module 相關巨集
#include <linux/kernel.h> // 必須：定義 printk

// 這是 Driver 載入時會執行的函式 (Constructor)
static int __init hello_init(void) {
    printk(KERN_INFO "Hello, World! This is my first Linux Driver.\n");
    return 0; // 回傳 0 代表載入成功
}

// 這是 Driver 被移除時會執行的函式 (Destructor)
static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye, World! Driver removed.\n");
}

// 告訴 Kernel 哪裡是起點，哪裡是終點
module_init(hello_init);
module_exit(hello_exit);

// 必要的版權聲明，沒寫這行有些 Kernel API 會不讓你用
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Frank");
MODULE_DESCRIPTION("A Simple Hello World Module");
