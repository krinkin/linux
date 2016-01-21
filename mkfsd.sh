
DISKS='/dev/sdc  /dev/sde  /dev/sdg  /dev/sdi  /dev/sdk  /dev/sdm  /dev/sdo  /dev/sdq /dev/sdd  /dev/sdf  /dev/sdh  /dev/sdj  /dev/sdl  /dev/sdn  /dev/sdp  /dev/sdr'

for d in $DISKS;
do
	echo "formatting as fat $d"
	sudo mkfs.fat -I $d
done;
