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

## Debugging

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

You can wrap these up into a script dm.gdb:

   set arch riscv:rv64
   target extended-remote localhost:1234
   file obj/device-model-riscv.elf
   thread 2
   set step on

and then `source dm.gdb` after you start gdb.

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

Symbols are available, eg to set a breakpoint at the main() function:

    (gdb) break *main

Function boundary tracing can be achieved by setting a breakpoint on each
function:

    (gdb) set logging on  # goes to gdb.txt
    (gdb) set confirm off
    (gdb) rbreak .        # set breakpoint on every symbol
    (gdb) c               # continue (Enter on a blank line repeats)

gdb will stop at every function entry and print the breakpoint number.  For
particularly regular breakpoints you can delete them so they don't stop
again: `disable 789`.

### Notes on virtual memory and bootup

The second core starts in a bootloader with the MMU off, at physical address
0x80xxxxxx.  Once the device model binary is loaded from CheriBSD, it is mapped at
physical address 0xf8000000.

The first code run is in
mdepx/arch/riscv/riscv/start-purecap.S  
This sets a 4-entry page table that maps the 4GB of physical
address space starting at 0x0 to virtual address 0xffffffd000000000.  The
supervisor trap handler is then set to the label `va` in virtual address
space, eg 0xffffffd0f80000ac.  The MMU is then enabled.  Since the next
PC=0xf80000ac, this virtual address is no longer mapped and the supervisor
trap handler is called.  Execution now proceeds in virtual address space.

Note that QEMU's gdb stub can't single step this switch (it can't insert
breakpoint instructions in memory which no longer exists), so you have to know that
execution has continued at `va`.  If you run:

    ~/cheri/output/sdk/bin/riscv64cheri-objdump -DSdt obj/device-model-riscv.elf > device-model-riscv.dump

It'll output the disassembly and symbol table, from where you can find the
virtual address of `va` and trace from there onwards.
