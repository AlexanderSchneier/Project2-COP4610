make clean
bear -- make
sudo rmmod elevator.ko
sudo insmod elevator.ko
watch -n 1 "cat /proc/elevator"
