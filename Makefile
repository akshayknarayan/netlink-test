TARGET = test
test-objs := netlink.o

obj-m := $(TARGET).o

all: test.ko test_user

test.ko:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

test_user:
	gcc -o test_user netlink_user.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f test_user
