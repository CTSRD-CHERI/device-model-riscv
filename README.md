# device-model-riscv

This is RISC-V version of [device-model](https://github.com/CTSRD-CHERI/device-model).

This app emulates AHCI block and Intel E1000 Ethernet devices using a secondary core of RISC-V CPU.

QEMU emulator is used in this project.

### Prerequisites

You need CHERI LLVM installed.  You can build that using [cheribuild](https://github.com/CTSRD-CHERI/cheribuild):

    $ ./cheribuild.py -d llvm

### To build this project

    $ env CROSS_COMPILE=$HOME/cheri/output/sdk/bin/riscv64-unknown-freebsd- CC=$HOME/cheri/output/sdk/bin/clang \
      make purecap

Alternatively, you can build using cheribuild:

    $ ./cheribuild.py device-model

You can now find the device-model binary in your ${HOME}/cheri/extra-files/root/ directory.

### Now, setup a tap device on the QEMU host machine:

    $ sudo tunctl -t tap0 -u `whoami`
    $ sudo ifconfig tap0 up

### Now, build and run CheriBSD on the 1st core of RISC-V CPU:
    $ ./cheribuild.py run-dm-riscv64-purecap "--run-dm-riscv64-purecap/extra-options=-smp 2 -serial mon:stdio -serial pty -accel tcg,thread=multi -device virtio-net-device,netdev=net0 -netdev tap,id=net0,ifname=tap0,script=no,downscript=no -D trace.log" -d --skip-world --cheribsd/git-revision=device-model-riscv --qemu/git-revision=concurrent_tags_rebased_dm --bbl/git-revision=cheri_purecap_dm

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

### Debugging

QEMU provides a GDB interface by which it's possible to attach to either or
both cores.  First, make sure device-model-riscv.elf is compiled with debug
symbols - add `-O0 -ggdb` to `set-build-flags` in mdepx-purecap.conf

Next, build the CHERI version of GDB:

    $ ./cheribuild.py -d gdb-native

Then run QEMU with the `-s` flag (eg adding it inside the
`--run-dm-riscv4-purecap/extra-options` list in the cheribuild command
above).  Separately, run GDB:

    $ ~/cheri/output/sdk/bin/gdb

Then connect to the GDB stub and configure GDB

    (gdb) set arch riscv:rv64
    (gdb) target extended-remote localhost:1234
    (gdb) info threads
          Id   Target Id                    Frame 
          * 1    Thread 1.1 (CPU#0 [halted ]) 0x000000008020425c in ?? ()
            2    Thread 1.2 (CPU#1 [halted ]) 0xffffffc0005bd6f6 in ?? ()
    (gdb) file device-model-riscv/obj/device-model-riscv.elf
    A program is being debugged already.
    Are you sure you want to change the file? (y or n) y
    Reading symbols from device-model-riscv/obj/device-model-riscv.elf...
    (No debugging symbols found in device-model-riscv/obj/device-model-riscv.elf)
    (gdb) info threads
      Id   Target Id                    Frame
    * 1    Thread 1.1 (CPU#0 [halted ]) 0xffffffc0005bd6f6 in ?? ()
      2    Thread 1.2 (CPU#1 [running]) 0xffffffd0f80059a4 in uart_16550_rxready ()

We can now use GDB to debug either thread (here 1 is FreeBSD, 2 is the
device model).  Note that it's not currently possible to pause one while
letting the other run - you have to pause both together.

    (gdb) thread 2
    [Switching to thread 2 (Thread 1.2)]
    #0  0xffffffd0f80059a4 in uart_16550_rxready ()
    (gdb) c
    Continuing.
    ^C
    Thread 1 received signal SIGINT, Interrupt.
    0xffffffd0f80059a4 in uart_16550_rxready (dev=0xffffffd0f8802960 [rwxRW,0xffffffd0f8802960-0xffffffd0f88029d0])                                                           at mdepx/dev/uart/uart_16550.c:82
    82              status = UART_READ(sc, REG_LSR);
    (gdb) n
    83              if (status & LSR_RXRDY)
    (gdb)
    86              return (false);
    (gdb)
    87      }
    (gdb)
    mdx_uart_rxready (dev=0xffffffd0f8802960 [rwxRW,0xffffffd0f8802960-0xffffffd0f88029d0]) at mdepx/dev/uart/uart.c:63                                                   63              return (ready);
