sudo mount /dev/sdb1 /opt/disk

DISKS=`ls -1 /dev/sd?`

for d in $DISKS;
do
	echo "mounting $d"
	mkdir -p ~/disk/$d
	sudo mount -oumask=0000 $d ~/disk/$d
	sudo touch      ~/disk/$d/f
done;
