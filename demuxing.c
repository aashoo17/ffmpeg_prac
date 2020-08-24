#include "avformat.h"
#include "avcodec.h"
#include <stdio.h>
#include <errno.h>

int main(){
    //format context
    AVFormatContext *formatContext = avformat_alloc_context();
    if(avformat_open_input(&formatContext,"file:bunny.mp4",NULL,NULL) != 0){
        perror("unable to open the given file");
        return -1;
    };

    //get the video stream
    for(int i = 0; i < formatContext->nb_streams; i++){
        if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            AVCodec *codec = avcodec_find_decoder(formatContext->streams[i]->codecpar->codec_id);
            AVCodecContext *codecContext = avcodec_alloc_context3(codec);
            if(avcodec_parameters_to_context(codecContext,formatContext->streams[i]->codecpar) < 0)
                return -1;
            //open the codec
            if(avcodec_open2(codecContext,codec,NULL) != 0)
                return -1;
            AVPacket *pkt = av_packet_alloc();
            AVFrame *frame = av_frame_alloc();
            FILE *f;
            while(av_read_frame(formatContext,pkt) == 0){
                if(avcodec_send_packet(codecContext,pkt) == 0){
                    if(avcodec_receive_frame(codecContext,frame) == 0){
                        //save_frame
                        char filename[1024];
                        snprintf(filename, sizeof(filename), "%s-%d.pgm", "frame", codecContext->frame_number);
                        f = fopen(filename,"w");
                        fprintf(f, "P5\n%d %d\n%d\n", frame->width, frame->height, 255);
                        fwrite(frame->data[0],1,frame->linesize[0] * frame->height,f);
                    }
                }
            }
            fclose(f);
            //break the loop when first video is encountered
            break;
        }
    }
}
