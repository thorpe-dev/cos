all: setitimer-helper squish-pty squish-unix

CC = /usr/local/i386elfgcc/bin/i386-elf-gcc-4.7.2
CFLAGS = -Wall -W
LDFLAGS = -lm
setitimer-helper: setitimer-helper.o
squish-pty: squish-pty.o
squish-unix: squish-unix.o

clean:
	rm -f *.o setitimer-helper squish-pty squish-unix
