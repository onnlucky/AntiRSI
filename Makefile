BASE=gnome

ifeq ($(shell uname), Linux)
BASE=gnome
endif

ifeq ($(shell uname), Darwin)
BASE=macosx
endif

all:
	cd $(BASE) && make all

run:
	cd $(BASE) && make run

install:
	cd $(BASE) && make install

uninstall:
	cd $(BASE) && make uninstall

clean:
	cd gnome && make clean
	cd macosx && make clean

.PHONY:all run clean install uninstall
