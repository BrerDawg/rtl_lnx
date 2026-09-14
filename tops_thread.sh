#!/bin/sh

while :
do
	top -n 1 -b -H -p `pidof rtl_lnx` > tops_thread.txt
	echo "running top cmd for rtl_lnx, see results in 'tops_thread.txt' ..."
	sleep 3
done


