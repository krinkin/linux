#!/bin/sh

#./fio-fio-2.2.9/fio ./disk1.fio &
#./fio-fio-2.2.9/fio ./disk2.fio &


DISKS=`ls -1 /dev/sd?`

for d in $DISKS;
do
	disk_letter=`echo -n ${d} | tail -c 1`
	
	if [ $disk_letter = "a" ]; then
		echo "Skipping $d"
	elif [ $disk_letter = "b" ]; then
		echo "Skipping $d"
	else
		echo "generate fio for disk $disk_letter"
		cat ./diskX.template | sed "s/DISKNAME/${disk_letter}/g" > ./disk_${disk_letter}.fio
		./fio-fio-2.2.9/fio disk_${disk_letter}.fio >stat_disk_${disk_letter}.log&
	fi
done;


./fio-fio-2.2.9/fio ./disk1.fio >stat_disk_a.log &
./fio-fio-2.2.9/fio ./disk2.fio >stat_disk_b.log &




