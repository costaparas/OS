#!/bin/sh

original_c=`mktemp -p /tmp`
original_h=`mktemp -p /tmp`
mv producerconsumer_driver.c $original_c
mv producerconsumer_driver.h $original_h
for h in producer-consumer-tests/*.h
do
	cp "$h" producerconsumer_driver.h
	for c in producer-consumer-tests/*.c
	do
		cp "$c" producerconsumer_driver.c
		make && make run args='1c;q'
	done
done
mv $original_c producerconsumer_driver.c
mv $original_h producerconsumer_driver.h
