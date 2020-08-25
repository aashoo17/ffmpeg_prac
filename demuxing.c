#include <avformat.h>
#include <avcodec.h>
#include <stdio.h>
#include <errno.h>

/*[demuxing from ffmpeg website](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html)
 * 1. opening the media file with avformat_open_input()
 * local files are given url with file:bunny.mp4 like, network files/other protocols are also supported.
 * this avformat_open_input() call only reads the header and populate the AVFormatContext.
 * 2. avformat_find_stream_info() call can read few extra bytes from stream AVStream and populate extra info in AVFormatContext if required
 * 3. loop over all the streams, AVFormatContext.nb_streams will contain total no of streams available in AVFormatContext/media file
 * 4. AVStream.coecpar has codec related parameters from which we can check if codec_type is for video/audio etc.
 * 5. given codec id from codec parameter we can create the codec AVCodec using avcodec_find_decoder()
 * 6. given AVCodec we can create AVCodecContext using avcodec_alloc_context3(). AVCodecContext is used for encode/decode process
 * 7. avcodec_parameters_to_context() fills the AVCodecContext details using codec
 * 8. we can start reading packets from streams using av_read_frame()
 * 9. when packet recieved pass it to the decoder avcodec_send_packet() for decoding
 * 10. receive raw decoded frames from codec using avcodec_receive_frame()
 * 11. whatever we want we can do with the frame now AVFrame.data[0] and AVFrame.linesize[0] is important property
 * 12. here we have used the frame to create pgm image files
 * 13. close the input file
 */
int main(){
    //format context allocation
    AVFormatContext *formatContext = avformat_alloc_context();
    //open te file bunny.mp4 and which in turn initialises the various fields of AVFormatContext
    if(avformat_open_input(&formatContext,"file:bunny.mp4",NULL,NULL) != 0){
        perror("unable to open the given file");
        return -1;
    };

    //loop over all the streams available
    for(int i = 0; i < formatContext->nb_streams; i++){
        //check if the stream is of type video
        if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            //find the decoder with codec_id of AVStream available in AVFormatContext
            AVCodec *codec = avcodec_find_decoder(formatContext->streams[i]->codecpar->codec_id);
            //allocated the AVCodecContext
            AVCodecContext *codecContext = avcodec_alloc_context3(codec);
            //fill the properties of allocated AVCodecContext using AVCodecParameters
            if(avcodec_parameters_to_context(codecContext,formatContext->streams[i]->codecpar) < 0)
                return -1;
            //open the codec
            if(avcodec_open2(codecContext,codec,NULL) != 0)
                return -1;
            //allocate packet and frame
            AVPacket *pkt = av_packet_alloc();
            AVFrame *frame = av_frame_alloc();
            FILE *f;
            //consume the packets from AVStream available in AVFormatContext
            while(av_read_frame(formatContext,pkt) == 0){
                //send the packets to decoder
                if(avcodec_send_packet(codecContext,pkt) == 0){
                    //receive decoded frames from decoder
                    if(avcodec_receive_frame(codecContext,frame) == 0){
                        //save_frame
                        char filename[1024];
                        //create different file names for saving the frame
                        snprintf(filename, sizeof(filename), "%s-%d.pgm", "frame", codecContext->frame_number);
                        f = fopen(filename,"w");
                        /*
                         * minimal PGM header
                         * P5
                         * 1920 1080
                         * 255
                         */
                        fprintf(f, "P5\n%d %d\n%d\n", frame->width, frame->height, 255);
                        //write the data to PGM file
                        fwrite(frame->data[0],1,frame->linesize[0] * frame->height,f);
                    }
                }
            }
            fclose(f);
            //break the loop when first video is encountered
            break;
        }
    }
    //close the input
    avformat_close_input(&formatContext);
}
