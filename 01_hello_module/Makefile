# 告訴 Kernel Build System 我們要編譯 hello.o
obj-m += pci_driver.o

# 指向我們之前編譯好的 Kernel 原始碼路徑
KDIR := ~/kernel_study/linux-6.6.14

all:
	# 呼叫 Kernel 的 Makefile，並告訴它：「模組原始碼在這裡 (PWD)」
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
