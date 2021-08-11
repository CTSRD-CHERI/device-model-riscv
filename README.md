# device-model-riscv

This is work in progress RISC-V version of [device-model](https://github.com/CTSRD-CHERI/device-model)

CheriBSD branch is [device-model-riscv](https://github.com/CTSRD-CHERI/cheribsd/tree/device-model-riscv).

### CheriBSD startup on the 1st core
        $ ./cheribuild.py run-riscv64 "--run-riscv64/extra-options=-smp 2 -serial mon:stdio -serial pty -accel tcg,thread=multi -bios /path/to/opensbi/lp64/sifive/fu540/firmware/fw_jump.elf -device virtio-net-device,netdev=net0 -netdev tap,id=net0,ifname=tap0,script=no,downscript=no" -d

### Device-model-riscv (this project) startup on the 2nd core

From the CheriBSD that is running on 1st core:

	$ /usr/sbin/bm -Rl qemu-riscv64.bin

### Console access to device-model.
    Replace 1 with your pts from QEMU output message on char device redirection
	$ cu -l /dev/pts/1

### Start PCI emulation

This includes PCI itself, E1000 and AHCI controllers:
    $ devctl disable pci0
    $ devctl enable pci0
