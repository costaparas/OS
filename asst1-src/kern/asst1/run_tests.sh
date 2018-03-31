#!/bin/sh

if test "$#" -ne 1
then
	echo "Usage: $0 (pc-prob|bar-prob)" >&2
	exit 1
fi

original_c=`mktemp -p /tmp`
original_h=`mktemp -p /tmp`
original_h2=`mktemp -p /tmp`

if test "$1" = pc-prob
then
	#run tests for producer-consumer problem
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
elif test "$1" = bar-prob
then
	#run tests for bar sync problem
	mv bar_driver.c $original_c
	cp bar_driver.h $original_h
	cp barglass.h $original_h2
	cp bar-tests/bar_driver.c .
	for cust in 1 3 10 32
	do
		for bart in 1 3 10 32
		do
			for drink in 1 3 5 8
			do
				for bottle in 1 3 10 16
				do
					sed -i "s/NCUSTOMERS [0-9]*/NCUSTOMERS $cust/" bar_driver.h
					sed -i "s/NBARTENDERS [0-9]*/NBARTENDERS $bart/" bar_driver.h
					sed -i "s/NBOTTLES [0-9]*/NBOTTLES $drink/" barglass.h
					sed -i "s/DRINK_COMPLEXITY [0-9]*/DRINK_COMPLEXITY $bottle/" barglass.h
					make && make run args='1d;q'
				done
			done
		done
	done
	mv $original_c bar_driver.c
	mv $original_h bar_driver.h
	mv $original_h2 barglass.h
else
	echo "$0: unknown test '$1'" >&2
	exit 1
fi
