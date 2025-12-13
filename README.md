# Embedded Linux Driver Journey

![Linux Kernel](https://img.shields.io/badge/Kernel-6.6.14-linux?logo=linux)
![QEMU](https://img.shields.io/badge/Emulator-QEMU%208.2.0-orange)
![Language](https://img.shields.io/badge/Language-C%20%7C%20Shell-blue)

This repository documents my hands-on journey into Linux Kernel development, from building a minimal bootable system to implementing a full software-hardware co-design simulation using QEMU.

## 🎯 Project Goals
The goal of this project is to bridge the gap between **Hardware (EE background)** and **System Software (CS concepts)** by:
- Building a custom **Linux Kernel (v6.6)** and **Rootfs (BusyBox)** from scratch.
- Developing various types of **Linux Drivers** (Character, Platform, PCI).
- Modifying **QEMU Source Code** to create a custom PCI hardware device.
- Implementing **Memory Mapped I/O (MMIO)** and **Interrupt Handling (IRQ)** mechanisms for efficient hardware-software communication.

## 🛠️ Tech Stack & Environment
- **Host System:** WSL2 (Ubuntu 24.04)
- **Target Kernel:** Linux 6.6.14 (x86_64)
- **User Space:** BusyBox 1.36.1 (Static linked)
- **Emulator:** QEMU 8.2.0 (Custom build with modified hardware model)
- **Languages:** C, Makefile, Shell Script

## 🚀 Key Milestones

### Level 0: System Bring-up
- Compiled Linux Kernel 6.6.14 from source.
- Built a minimal Root Filesystem using BusyBox.
- Solved the initial `Kernel panic - not syncing: VFS: Unable to mount root fs` by creating a custom `init` script and packaging it with `cpio`.

### Level 1: Kernel Module Basics
- Implemented a basic Loadable Kernel Module (LKM).
- Understood the lifecycle of a driver (`module_init`, `module_exit`) and Kernel Logging (`printk`, `dmesg`).

### Level 2: Character Device & IOCTL
- Developed a Character Device Driver (`simple_char`).
- Implemented `file_operations` (`open`, `read`, `write`, `release`).
- **Key Learning:** Used `copy_from_user` / `copy_to_user` to safely handle memory between User Space and Kernel Space.
- Implemented `ioctl` to control simulated hardware states (LED On/Off).

### Level 3: Linux Device Model (Platform Driver)
- Migrated from legacy driver registration to the modern **Platform Driver** architecture.
- Implemented `probe` and `remove` callbacks.
- Utilized `class_create` and `device_create` for automatic device node creation (`/dev/my_led`) via udev mechanisms.

### Level 4: Hardware Simulation (QEMU Modification)
- **"God Mode" enabled:** Modified QEMU source code to create a custom PCI Device (`my-pci-dev`).
- **Features:**
  - Defined a custom Vendor ID (0x1234) and Device ID (0x2222) to avoid conflict with default QEMU VGA devices.
  - Implemented **MMIO** logic using QEMU's `MemoryRegionOps`.
  - Configured **PCI Interrupt Pin (INTA)** to enable asynchronous hardware notification.
  - Simulates a hardware register that triggers a specific event when the magic value `0xCAFEBABE` is written.

### Level 5: PCI Driver & Interrupt Handling
- Developed a Linux PCI Driver to drive the custom QEMU hardware.
- **Key Implementation:**
  - Used `pci_register_driver` and `pci_device_id` table for device matching.
  - Retrieved BAR0 physical address and mapped it to virtual address using `ioremap`.
  - Implemented **Interrupt Service Routine (ISR)** to handle hardware IRQs asynchronously.
  - Successfully verified the full **Software-Hardware Co-design loop**: Driver Command -> Hardware Action -> IRQ Trigger -> ISR Handling.

## 📸 Screenshots / Logs

**1. MMIO Communication Verification**
<img width="1330" height="639" alt="MMIO Log" src="https://github.com/user-attachments/assets/9ef8dc14-4f69-41a3-9875-9ee8d641aa5f" />

**2. Demonstration of Interrupt Handling (IRQ)**
*Note: The log shows the full cycle of IRQ Trigger (Hardware) and ISR Acknowledgement (Driver).*
<img width="1040" height="554" alt="IRQ Log" src="https://github.com/user-attachments/assets/54c1f838-2461-40be-a148-68e3ba4991b0" />

## 🔮 Future Work
- Implement **DMA (Direct Memory Access)** for bulk data transfer.
- Upgrade the custom device to comply with standard **VirtIO** protocols.
