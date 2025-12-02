#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_device.h"
#include "qom/object.h"

/* 定義這個裝置的名稱與 ID */
#define TYPE_MY_PCI_DEV "my-pci-dev"
#define MY_PCI_DEV(obj) OBJECT_CHECK(MyPCIDevState, (obj), TYPE_MY_PCI_DEV)

/* 這是我們裝置的 "狀態" (硬體裡的 Register 都在這) */
typedef struct {
    PCIDevice parent_obj; // 繼承自 PCI 裝置
    MemoryRegion mmio;    // 定義一塊 MMIO 記憶體區域
} MyPCIDevState;

/* * 當 CPU 讀取我們的記憶體時，會執行這個函式
 * addr: 相對位址 (Offset)
 */
static uint64_t my_pci_read(void *opaque, hwaddr addr, unsigned size)
{
    printf("QEMU [Hardware]: Driver reads from addr 0x%lx\n", addr);
    
    // 如果讀取 Offset 0，回傳我們的 Magic Number
    if (addr == 0) {
        return 0x12345678;
    }
    return 0;
}

/* * 當 CPU 寫入我們的記憶體時，會執行這個函式
 */
static void my_pci_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    printf("QEMU [Hardware]: Driver writes value 0x%lx to addr 0x%lx\n", val, addr);
    
    // 我們可以根據寫入的值做不同的事 (例如點亮 LED)
    if (val == 0xCAFEBABE) {
        printf("QEMU [Hardware]: >>> SECRET COMMAND RECEIVED! <<<\n");
    }
}

/* 定義讀寫的操作介面 */
static const MemoryRegionOps my_pci_ops = {
    .read = my_pci_read,
    .write = my_pci_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

/* * 裝置初始化函式 (像是硬體的 Power On)
 */
static void my_pci_realize(PCIDevice *pdev, Error **errp)
{
    MyPCIDevState *s = MY_PCI_DEV(pdev);

    // 1. 初始化一塊 4KB 的 MMIO 記憶體空間
    memory_region_init_io(&s->mmio, OBJECT(s), &my_pci_ops, s, "my-pci-mmio", 4096);

    // 2. 將這塊記憶體掛載到 PCI BAR 0 (Base Address Register 0)
    // 這樣 Driver 就可以透過讀取 BAR0 找到這塊記憶體
    pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);
    
    printf("QEMU [Hardware]: my-pci-dev is plugged in!\n");
}

static void my_pci_class_init(ObjectClass *class, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(class);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

    // 設定初始化函式
    k->realize = my_pci_realize;
    
    // 設定 PCI Vendor ID 和 Device ID (這很重要，Driver 靠這個認人)
    k->vendor_id = PCI_VENDOR_ID_QEMU; // 使用 QEMU 預設廠商 ID
    k->device_id = 0x1111;             // 我們自定義的 Device ID
    k->class_id = PCI_CLASS_OTHERS;
    
    // 設定分類
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static const TypeInfo my_pci_info = {
    .name          = TYPE_MY_PCI_DEV,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(MyPCIDevState),
    .class_init    = my_pci_class_init,
    .interfaces = (InterfaceInfo[]) {
        { INTERFACE_PCIE_DEVICE },
        { }
    },
};

static void my_pci_register_types(void)
{
    type_register_static(&my_pci_info);
}

type_init(my_pci_register_types)
