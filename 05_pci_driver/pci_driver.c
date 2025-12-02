#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>      // [NEW] PCI 驅動的核心標頭檔
#include <linux/io.h>       // [NEW] 定義 ioremap, readl, writel

#define DRIVER_NAME "my_pci_driver"
#define MY_VENDOR_ID 0x1234
#define MY_DEVICE_ID 0x1111

// 定義我們要對應的硬體 ID 表
static struct pci_device_id my_pci_tbl[] = {
    { PCI_DEVICE(MY_VENDOR_ID, MY_DEVICE_ID) },
    { 0, } // 結尾必須是 0
};
// 讓 Kernel 工具知道這個模組支援哪些硬體 (Hot-plug 用)
MODULE_DEVICE_TABLE(pci, my_pci_tbl);

// 這是一個指向裝置記憶體的指標 (Virtual Address)
static void __iomem *mmio_base;

// --- Probe 函式：當 Kernel 發現硬體 ID 符合時會呼叫 ---
static int my_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    int ret;
    u32 magic_value;
    resource_size_t mmio_start, mmio_len;

    printk(KERN_INFO "MyPCI: Found device! Bus: %s\n", pci_name(pdev));

    // 1. 啟用 PCI 裝置 (喚醒硬體)
    ret = pci_enable_device(pdev);
    if (ret) return ret;

    // 2. 讀取 BAR0 的實體地址與長度
    // (這就是你在 cat /proc/bus/pci/devices 看到的 febf1000)
    mmio_start = pci_resource_start(pdev, 0);
    mmio_len = pci_resource_len(pdev, 0);

    printk(KERN_INFO "MyPCI: BAR0 Physical Address: %pa, Length: %llu\n", &mmio_start, mmio_len);

    // 3. [面試必考] 將實體地址映射到虛擬地址 (ioremap)
    // Linux Kernel 不能直接存取實體地址，必須透過 Page Table 映射
    mmio_base = ioremap(mmio_start, mmio_len);
    if (!mmio_base) {
        printk(KERN_ERR "MyPCI: ioremap failed\n");
        return -EIO;
    }

    // 4. 讀取硬體暫存器 (驗證 Magic Number)
    // QEMU 裡我們寫了：if (addr == 0) return 0x12345678;
    magic_value = readl(mmio_base + 0); 
    printk(KERN_INFO "MyPCI: Read Magic Number from HW: 0x%X\n", magic_value);

    if (magic_value == 0x12345678) {
        printk(KERN_INFO "MyPCI: Hardware Verification SUCCESS!\n");
        
        // 5. 寫入密技指令，觸發 QEMU 的 printf
        printk(KERN_INFO "MyPCI: Sending Secret Command to QEMU...\n");
        writel(0xCAFEBABE, mmio_base + 4); // 寫入 Offset 4
    } else {
        printk(KERN_WARNING "MyPCI: Magic Number mismatch! Is this the VGA card?\n");
        // 因為 QEMU 有兩個 1234:1111，其中一個是 VGA，它沒有我們的 Magic Number
    }

    return 0;
}

// --- Remove 函式：卸載 Driver 時呼叫 ---
static void my_pci_remove(struct pci_dev *pdev)
{
    printk(KERN_INFO "MyPCI: Removing driver\n");

    if (mmio_base) {
        iounmap(mmio_base); // 解除映射
    }
    pci_disable_device(pdev); // 關閉裝置
}

// 定義 PCI Driver 結構
static struct pci_driver my_driver = {
    .name = DRIVER_NAME,
    .id_table = my_pci_tbl,
    .probe = my_pci_probe,
    .remove = my_pci_remove,
};

// 註冊 Driver
static int __init my_init(void) {
    return pci_register_driver(&my_driver);
}

static void __exit my_exit(void) {
    pci_unregister_driver(&my_driver);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
