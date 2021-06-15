# device-model-riscv

This is work in progress RISC-V version of [device-model](https://github.com/CTSRD-CHERI/device-model)

### CheriBSD startup on the 1st core
	$ ./cheribuild.py run-riscv64-hybrid "--run-riscv64-hybrid/extra-options=-smp 2 -serial mon:stdio -serial pty"

### Device-model-riscv (this project) startup
	$ /usr/sbin/bm -Rl qemu-riscv64.bin

### Console access to device-model. Replace 1 with your pts from QEMU output message on char device redirection
	$ cu -l /dev/pts/1
