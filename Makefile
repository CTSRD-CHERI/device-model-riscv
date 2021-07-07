APP = qemu-riscv64

OSDIR = mdepx
DM_BASE = 0

export CFLAGS = -march=rv64gc -mabi=lp64 -mcmodel=medany		\
	-nostdinc -fno-builtin-printf -ffreestanding -Wall		\
	-Wredundant-decls -Wnested-externs -Wstrict-prototypes		\
	-Wmissing-prototypes -Wpointer-arith -Winline -Wcast-qual	\
	-Wundef -Wmissing-include-dirs -Werror -DWITHOUT_CAPSICUM=1	\
	-DDM_BASE=${DM_BASE}

export AFLAGS = ${CFLAGS}

CMD = python3 -B ${OSDIR}/tools/emitter.py

all:
	@${CMD} -j mdepx.conf
	${CROSS_COMPILE}objcopy -O binary ${OBJDIR}/${APP}.elf	\
		${OBJDIR}/${APP}.bin
	@echo /usr/sbin/bm -Rl /root/qemu-riscv64.bin
	@mkdir -p ${HOME}/cheri/extra-files/root/
	@cp obj/qemu-riscv64.bin ${HOME}/cheri/extra-files/root/

clean:
	@rm -rf obj/*

include ${OSDIR}/mk/user.mk
