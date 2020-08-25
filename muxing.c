#include <avformat.h>
#include <avcodec.h>
#include <stdio.h>
#include <errno.h>

/* [muxing from ffmpeg tutorial](https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html)
 * 1. create a AVFormatContext which can be outputted to a file avformat_alloc_output_context2()
 * AVFormatContext.oformat is initialized by this and when we get AVFormatContext from avformat_open_input() AVFormatContext.iformat is having value
 * 2. create a new AVStream and attach to AVFormatContext using avformat_new_stream()
 * 3. create codec parameter AVCodecParameters or copy from existing AVFormatContext from where we are copying stream
 * avcodec_parameters_copy() = but copying from avcodec_parameters_copy is not recommended usually and should be created a fresh one but how ??
 * 4. open AVFormatContext for writing using avio_open2(), AVFormatContext.pb is given value by avio_open()
 * 5. write stream headers avformat_write_header()
 * 6. write the frame = av_write_frame()/av_interleaved_write_frame()
 * first we have to read packets/frames using av_read_frame() in loop and inside the loop use av_write_frame()/av_interleaved_write_frame()
 * 7. av_write_trailer()
*/
int main(){
    //opening the existing mp4 container file
    AVFormatContext *inputFormatContext = avformat_alloc_context();
    if(avformat_open_input(&inputFormatContext,"file:bunny.mp4",NULL,NULL) != 0){
        perror("unable to open the given file");
        return -1;
    };
    //allocated AVFormatContext which will be used to create new output file with different container
    AVFormatContext *outputFormatContext = avformat_alloc_context();
    //initialize this outputFormatContext of type AVFormatContext using vformat_alloc_output_context2 where we are telling output file would be file:new_bunny.mp4
    if (avformat_alloc_output_context2(&outputFormatContext,NULL,NULL,"file:new_bunny.ts") < 0){
        perror("avformat_alloc_output_context2() allocation error");
    }
    //loop over all the streams of type audio or video and this streams will be inserted in our new AVFormatContext *outputFormatContext

    for (int i = 0; i < inputFormatContext->nb_streams; ++i) {
        AVStream *outputStream;
        AVStream *inputStream = inputFormatContext->streams[i];
        //get the codec params for the ith stream
        AVCodecParameters *inputCodecParam = inputStream->codecpar;
        //check if the codec_type is audio or video
        if(inputCodecParam->codec_type == AVMEDIA_TYPE_VIDEO || inputCodecParam->codec_type == AVMEDIA_TYPE_AUDIO){
            //create a new stream in AVFormatContext *outputFormatContext
            outputStream = avformat_new_stream(outputFormatContext,NULL);
            //copy the codec parameters from previous existing AVFormatContext *formatContext
            avcodec_parameters_copy(outputFormatContext->streams[i]->codecpar,inputFormatContext->streams[i]->codecpar);
        }
    }

    //open the file video stream for modification here i have assumed video stream to be at index 0 as i know before hand from demuxing.c
    //but can be checked for video stream index and assigned accordingly
    avio_open2(&outputFormatContext->pb,"file:new_bunny.ts",AVIO_FLAG_WRITE,NULL,NULL);
    //write headers
    avformat_write_header(outputFormatContext,NULL);
    //allocate a packet
    AVPacket *pkt = av_packet_alloc();
    //read the packets in loop
    while(av_read_frame(inputFormatContext,pkt) == 0){
        AVStream *inputStream, *outputStream;
        inputStream = inputFormatContext->streams[pkt->stream_index];
        outputStream = outputFormatContext->streams[pkt->stream_index];
        // copy packet
        // for each packet we need to re-calculate the PTS and DTS to finally write it (av_interleaved_write_frame) to our output format context.
        pkt->pts = av_rescale_q_rnd(pkt->pts, inputStream->time_base, outputStream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt->dts = av_rescale_q_rnd(pkt->dts, inputStream->time_base, outputStream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt->duration = av_rescale_q(pkt->duration, inputStream->time_base, outputStream->time_base);
        // https://ffmpeg.org/doxygen/trunk/structAVPacket.html#ab5793d8195cf4789dfb3913b7a693903
        pkt->pos = -1;
        //write packet
        av_interleaved_write_frame(outputFormatContext,pkt);
    }
    //create output file
    av_write_trailer(outputFormatContext);
}

