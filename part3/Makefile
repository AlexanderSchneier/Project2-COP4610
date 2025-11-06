obj-m += elevator.o
elevator-objs := src/elevator_main.o src/core.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean

.PHONY: all clean