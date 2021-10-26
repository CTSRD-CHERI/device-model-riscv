.NOTPARALLEL:

APP = device-model-riscv

OSDIR = mdepx
OBJDIR = obj

CMD = python3 -B ${OSDIR}/tools/emitter.py

all: release copy
debug: _debug copy
purecap: release_purecap copy
purecap_debug: debug_purecap copy

release:
	@${CMD} -j mdepx.conf

release_purecap:
	@${CMD} -j mdepx-purecap.conf

_debug:
	@${CMD} -d -j mdepx.conf

debug_purecap:
	@${CMD} -d -j mdepx-purecap.conf

objdump:
	${CROSS_COMPILE}objdump -S ${OBJDIR}/${APP}.elf | less

readelf:
	${CROSS_COMPILE}readelf -a ${OBJDIR}/${APP}.elf | less

copy:
	${CROSS_COMPILE}objcopy -O binary \
		${OBJDIR}/${APP}.elf ${OBJDIR}/${APP}.bin
	@echo /usr/sbin/bm -Rl /root/${APP}.bin
	@mkdir -p ${HOME}/cheri/extra-files/root/
	@cp obj/${APP}.bin ${HOME}/cheri/extra-files/root/

clean:
	@rm -rf obj/*

include ${OSDIR}/mk/user.mk
