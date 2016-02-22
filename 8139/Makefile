obj-m += 8139-jack.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean 

load: 
	sudo insmod 8139-jack.ko

unload:
	sudo rmmod 8139-jack
