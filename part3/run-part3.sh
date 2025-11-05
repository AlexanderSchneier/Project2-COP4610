make clean
bear -- make
sudo rmmod elevator
sudo insmod elevator.ko
watch -n 1 "cat /proc/elevator"
