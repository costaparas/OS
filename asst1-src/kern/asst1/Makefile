make:
	cd ../compile/ASST? && bmake && bmake install

run:
	cd ../../../root && sys161 kernel "$(args)"

run-gdb:
	cd ../../../root && sys161 -w kernel "$(args)"

gdb:
	cd ../../../root && os161-gdb kernel

conf:
	cd ../../ && ./configure && cd kern/conf && ./config ASST? && \
	cd ../compile/ASST? && bmake depend && bmake && bmake install
