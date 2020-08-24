CFLAGS = -I/usr/include/ -g
all:demuxing
	./demuxing && rm demuxing
demuxing:demuxing.c -lavformat -lavcodec -lavfilter -lavutil
clean:
	rm *.pgm

.PHONY:all,clean