CFLAGS = -I/usr/include/libavcodec -I/usr/include/libavfilter -I/usr/include/libavformat -I/usr/include/libavutil
#CFLAGS = -I/usr/include -g
all:muxing
	./muxing && rm muxing
demuxing:demuxing.c -lavformat -lavcodec -lavfilter -lavutil
muxing:muxing.c -lavformat -lavcodec -lavfilter -lavutil
clean:
	rm *.pgm

.PHONY:all,clean