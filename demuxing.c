#include "avformat.h"
#include "avcodec.h"
#include <stdio.h>

//[inspired from this github repo](https://github.com/leandromoreira/ffmpeg-libav-tutorial)

static void save_gray_frame(unsigned char *buf, int linesize, int xsize, int ysize, char *filename);

int main() {
    //You'll first need to load your media file into a struct called AVFormatContext (the video container is also known as format).
    //It actually doesn't fully read the whole file it often only reads the header of the file
    AVFormatContext *avfctx = avformat_alloc_context();
    //if opened with non-zero return immediately
    //if AVFormatContext *avfctx is not allocated by avformat_alloc_context() and passed as NULL value to avformat_open_input()
    //it will be allocated automatically internally calling same avformat_alloc_context() function
    /*
     * AVFormatContext *avfctx = NULL;
     * here avfctx when passed to avformat_open_input will be automatically allocated
     * avformat_open_input(&avfctx,"file:mila.mp4",NULL,NULL);
     */
    if(avformat_open_input(&avfctx,"file:bunny.mp4",NULL,NULL) != 0){
        puts("failed at opening given file at avformat_open_input() call ");
        return -1;
    }
    //Once we loaded the minimal header of our container, we can access its streams (think of them as a rudimentary audio and video data).
    //Each stream will be available in a component called AVStream.
    //avformat_open_input() does not read underlying streams data so AVFormatContext will be initialized for limited field only
    //reading few bytes of streams and loading more actual data in AVFormatContext
    //TODO: use gdb to print *avfctx to see changes after avformat_find_stream_info() call to get the idea if where changes are happening in AVFormatContext
    if(avformat_find_stream_info(avfctx,NULL) < 0){
        puts("error in avformat_find_stream_info");
        return -1;
    };
    //TODO: find the fields which gets values after calling avformat_find_stream_info() only and not avformat_open_input()
    //Suppose our video has two streams: an audio encoded with AAC CODEC and a video encoded with H264 (AVC) CODEC.
    //From each stream we can extract pieces (slices) of data called packets that will be loaded into struct named AVPacket.
    //The data inside the packets are still coded (compressed) and in order to decode the packets, we need to pass them to a specific AVCodec.

    //The AVCodec will decode them into AVFrame and finally, this component gives us the uncompressed frame. Noticed that
    //the same terminology/process is used either by audio and video stream.
    //AVCodecParameters can give access to the AVCodec required to decode the stream
    //allocating a heap buffer to store info of AVCodecParameters for all streams available in the media
    AVCodecParameters **localCodecParameters = malloc(sizeof(AVCodecParameters*)*(avfctx->nb_streams));
    for(int i = 0; i < avfctx->nb_streams; i++){
        //with codec parameter we can get the codec required for this stream AVFormatContext.streams & AVStream.codecpar
        localCodecParameters[i] = avfctx->streams[i]->codecpar;
    }
    //with codec parameter find codec
    //getting the codec for the first video stream
    for(int i = 0; i < avfctx->nb_streams; i++) {
        //checking if the codec tyoe is of Video using AVMEDIA_TYPE_VIDEO
        if(localCodecParameters[i]->codec_type == AVMEDIA_TYPE_VIDEO){
            //AVCodec is now created using AVCodecParameters.codec_id
            AVCodec *codec = avcodec_find_decoder(localCodecParameters[0]->codec_id);
            //With the codec, we can allocate memory for the AVCodecContext, which will hold the context for our decode/encode process,
            // but then we need to fill this codec context with CODEC parameters; we do that with avcodec_parameters_to_context.
            //Once we filled the codec context, we need to open the codec. We call the function avcodec_open2 and then we can use it.
            //allocate memory for AVCodecContext
            AVCodecContext *cctx = avcodec_alloc_context3(codec);
            //fill the fields by passing AVCodecParameters
            avcodec_parameters_to_context(cctx,localCodecParameters[i]);
            //now open the codec
            avcodec_open2(cctx, codec, NULL);
            //Now we're going to read the packets from the stream and decode them into frames but first,
            // we need to allocate memory for both components, the AVPacket and AVFrame
            AVPacket *packet = av_packet_alloc();
            AVFrame *frame = av_frame_alloc();
            //read the packets from AVStream until AVStream is exhausted using av_read_frame
            while (av_read_frame(avfctx, packet) >= 0) {
                //send the packet to decoder
                int send_resp = avcodec_send_packet(cctx,packet);
                if(send_resp < 0){
                    puts("error in sending packets");
                }
                while (send_resp >= 0){
                    //receive frames when packet is processed by decoder
                    int receive_resp = avcodec_receive_frame(cctx,frame);
                    if (receive_resp == AVERROR(EAGAIN) || receive_resp == AVERROR_EOF) {
                        break;
                    } else if (receive_resp < 0) {
                        puts("Error while receiving a frame from the decoder");
                        return receive_resp;
                    }
                    if (receive_resp >= 0) {
                        //if frame received successfully
                        char frame_filename[1024];
                        snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", cctx->frame_number);
                        // save a grayscale frame into a .pgm file
                        save_gray_frame(frame->data[0], frame->linesize[0], frame->width, frame->height, frame_filename);

                    }
                }
            }
            //Let's send the raw data packet (compressed frame) to the decoder, through the codec context, using the function avcodec_send_packet.
            //break if first video is found
            break;
        }
        //free the heap allocated memory for codec parameters
        free(localCodecParameters);
    }

    //TODO: look for memory deallocations normally we are allocating through function calls need to be cleared
    //TODO:dealloc codec context AVCodecContext
    //TODO:close the opened decoder
    //TODO:free AVFrame and AVPacket

    //close the file opened previously
    avformat_close_input(&avfctx);
}
//save gray frame in pgm file
//[how pgm works](http://davis.lbl.gov/Manuals/NETPBM/doc/pgm.html)
//(pgm specifications)[http://netpbm.sourceforge.net/doc/pgm.html]
static void save_gray_frame(unsigned char *buf, int linesize, int xsize, int ysize, char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    /* writing the minimal required header for a pgm file format
     * P5
     * 1920 1080
     * 255
     */
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    /* writing line by line by a code a copied
     * we are writing data available at frame->data[0] pointer first written 1 byte size data of xsize=1920 here then in
     * next loop moved the pointer frame->data[0] + linesize[0] so ultimately we are writing a total of frame->data[0] + linesize[0]*ysize
     * since linesize[0] is same as xsize heck even frame->data[0] + xsize*ysize works
     * fwrite(buf,1,xsize * ysize,f);
     *    for (i = 0; i < ysize; i++)
     *        fwrite(buf + i * wrap, 1, xsize, f);
     */
    //but why not write all data in one go and this works instead of going in loop
    fwrite(buf,1,linesize * ysize,f);
    fclose(f);
}