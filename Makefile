CFLAGS = -I/usr/include/
all:main
	./demuxing && rm demuxing
demuxing:demuxing.c -lavformat -lavcodec -lavfilter -lavutil
clean:
	rm *.pgm

.PHONY:all,clean