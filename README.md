# device-model-riscv

This is RISC-V version of [device-model](https://github.com/CTSRD-CHERI/device-model).

This app emulates AHCI block and Intel E1000 Ethernet devices using a secondary core of RISC-V CPU.

QEMU emulator is used in this project.

### First, build this project
    $ make purecap

Alternatively, you can build using [cheribuild](https://github.com/CTSRD-CHERI/cheribuild):

    $ ./cheribuild.py device-model

You can now find the device-model binary in your ${HOME}/cheri/extra-files/root/ directory.

### Now, setup a tap device on the QEMU host machine:

    $ sudo tunctl -t tap0 -u `whoami`
    $ sudo ifconfig tap0 up

### Now, build and run CheriBSD on the 1st core of RISC-V CPU:
```
    $ ./cheribuild.py run-dm-riscv64-purecap "--run-dm-riscv64-purecap/extra-options=-smp 2 -serial mon:stdio -serial pty -accel tcg,thread=multi -device virtio-net-device,netdev=net0 -netdev tap,id=net0,ifname=tap0,script=no,downscript=no -D trace.log" -d --skip-world --cheribsd/git-revision=device-model-riscv --qemu/git-revision=concurrent_tags_rebased_dm --bbl/git-revision=cheri_purecap_dm
```

Optionally add --skip-world, --skip-kernel to the above command line to skip world or kernel builds.

### Device-model-riscv (this project) startup on the 2nd core

First, get the console access to device-model. Replace XX with your pts from QEMU output message on char device redirection

    $ cu -l /dev/pts/XX

Now, from the CheriBSD that is running on 1st core, load the device-model binary to a designated location in DRAM:

    $ /usr/sbin/bm -Rl /root/device-model-riscv.bin

### Start PCI emulation (from the CheriBSD terminal):

This includes PCI itself, E1000 and AHCI controllers:

    $ sleep 5 # ensure device-model is fully started
    $ devctl disable pci0
    $ devctl enable pci0

### Test

Once devices initialized you can setup an IP address on em0 device in CheriBSD, and a filesystem on /dev/ada0.
