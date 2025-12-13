#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>      // [NEW] PCI 驅動的核心標頭檔
#include <linux/io.h>       // [NEW] 定義 ioremap, readl, writel

#define DRIVER_NAME "my_pci_driver"
#define MY_VENDOR_ID 0x1234
#define MY_DEVICE_ID 0x2222

// 定義我們要對應的硬體 ID 表
static struct pci_device_id my_pci_tbl[] = {
    { PCI_DEVICE(MY_VENDOR_ID, MY_DEVICE_ID) },
    { 0, } // 結尾必須是 0
};
// 讓 Kernel 工具知道這個模組支援哪些硬體 (Hot-plug 用)
MODULE_DEVICE_TABLE(pci, my_pci_tbl);

// 這是一個指向裝置記憶體的指標 (Virtual Address)
static void __iomem *mmio_base;

// --- [NEW] 中斷處理函式 (ISR) ---
// 這就是 "Top Half" - 必須執行得非常快
static irqreturn_t my_pci_irq_handler(int irq, void *dev_id)
{
    // 在真實 Driver 中，我們會讀取 Hardware Status Register 確認是不是我們的卡
    // 這裡我們假設就是我們觸發的
    printk(KERN_INFO "MyPCI: [ISR] >>> INTERRUPT RECEIVED! (IRQ %d) <<<\n", irq);

    // [關鍵動作] 告訴硬體：「我收到了，你可以閉嘴了」
    // 如果不寫這行，硬體的中斷線會一直拉高，造成 "Interrupt Storm" (系統會當機)
    writel(0x1, mmio_base + 8); 

    return IRQ_HANDLED; // 告訴 Kernel：這是我處理的，沒問題
}

// --- Probe 函式：當 Kernel 發現硬體 ID 符合時會呼叫 ---
static int my_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    int ret;
    resource_size_t mmio_start, mmio_len;

    printk(KERN_INFO "MyPCI: Found device! Bus: %s\n", pci_name(pdev));

    // 1. 啟用 PCI 裝置 (喚醒硬體)
    ret = pci_enable_device(pdev);
    if (ret) return ret;

    // [NEW] 啟用 Bus Master (讓硬體有權限發送訊號)
    pci_set_master(pdev);

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

    // [NEW] 註冊中斷
    // IRQF_SHARED: PCI 裝置通常會共用中斷線，所以要設這個 flag
    ret = request_irq(pdev->irq, my_pci_irq_handler, IRQF_SHARED, DRIVER_NAME, pdev);
    if (ret) {
        printk(KERN_ERR "MyPCI: Failed to request IRQ %d\n", pdev->irq);
        iounmap(mmio_base);
        pci_disable_device(pdev);
        return ret;
    }
    
    printk(KERN_INFO "MyPCI: IRQ %d requested successfully.\n", pdev->irq);

    // --- 測試觸發 ---
    // 寫入 0xCAFEBABE，硬體應該要立刻回覆我們一個中斷
    printk(KERN_INFO "MyPCI: Writing command to trigger IRQ...\n");
    writel(0xCAFEBABE, mmio_base + 4);

    return 0;
}

// --- Remove 函式：卸載 Driver 時呼叫 ---
static void my_pci_remove(struct pci_dev *pdev)
{
    printk(KERN_INFO "MyPCI: Removing driver\n");

    // [NEW] 卸載時一定要釋放中斷，不然下次載入會炸掉
    free_irq(pdev->irq, pdev);

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
