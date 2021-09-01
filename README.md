# device-model-riscv

This is RISC-V version of [device-model](https://github.com/CTSRD-CHERI/device-model).

This app emulates AHCI block and Intel E1000 Ethernet devices using a secondary core of RISC-V CPU.

CheriBSD branch is [device-model-riscv](https://github.com/CTSRD-CHERI/cheribsd/tree/device-model-riscv).

### CheriBSD startup on the 1st core
    $ ./cheribuild.py run-riscv64-purecap				\
	"--run-riscv64-purecap/extra-options=-smp 2 -serial mon:stdio	\
	-serial pty							\
	-accel tcg,thread=multi						\
	-device virtio-net-device,netdev=net0				\
	-netdev tap,id=net0,ifname=tap0,script=no,downscript=no		\
	-bios ${HOME}/cheri/build/bbl-baremetal-riscv64-purecap-build/bbl" -d

### Device-model-riscv (this project) startup on the 2nd core

First, build it:

    $ make purecap

From the CheriBSD that is running on 1st core:

    $ /usr/sbin/bm -Rl device-model-riscv.bin

### Console access to device-model.
    Replace 1 with your pts from QEMU output message on char device redirection
	$ cu -l /dev/pts/1

### Setup tap device (on the QEMU host):

    $ sudo tunctl -t tap0 -u `whoami`
    $ sudo ifconfig tap0 up

### Start PCI emulation (on the CheriBSD):

This includes PCI itself, E1000 and AHCI controllers:

    $ devctl disable pci0
    $ devctl enable pci0
