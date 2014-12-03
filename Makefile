PKGS=gtk+-3.0 glib-2.0 clutter-1.0 clutter-gtk-1.0
CFLAGS=-g3 `pkg-config --cflags $(PKGS)`
CC=gcc
LDLIBS=`pkg-config --libs $(PKGS)` -lm

CFILES=$(shell find . -iname "*.c")
OBJ=$(patsubst %.c, %.o, $(CFILES))

main: $(OBJ)
	$(CC) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(OBJ) main
