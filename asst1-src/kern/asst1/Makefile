make:
	cd ../compile/ASST1 && bmake && bmake install

run:
	cd ../../../root && sys161 kernel $(args)

run-gdb:
	cd ../../../root && sys161 -w kernel $(args)

gdb:
	cd ../../../root && os161-gdb kernel
