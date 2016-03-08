#!/bin/sh

#./fio-fio-2.2.9/fio ./disk1.fio &
#./fio-fio-2.2.9/fio ./disk2.fio &


DISKS=`ls -1 /dev/sd?`
rm ./stat_disk_?.log

for d in $DISKS;
do
	disk_letter=`echo -n ${d} | tail -c 1`
	echo "generate fio for disk $disk_letter"
	
	if [ $disk_letter = "a" ]; then
		cat ./diskX.template | sed "s/DISKNAME/\/opt\/disk\/f/g" > ./disk_${disk_letter}.fio
	else
		cat ./diskX.template | sed "s/DISKNAME/\/home\/vagrant\/disk\/dev\/sd${disk_letter}\/f/g" > ./disk_${disk_letter}.fio
	fi

	./fio-fio-2.2.9/fio disk_${disk_letter}.fio >stat_disk_${disk_letter}.log &
done;






