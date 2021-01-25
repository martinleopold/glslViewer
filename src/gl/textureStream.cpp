#include "textureStream.h"

#include <iostream>
#include <fstream>

#ifdef SUPPORT_FOR_LIBAV 

extern "C" {
#include <libavutil/log.h>
#include <libavutil/avutil.h>
#include <sys/time.h>
#include <libswresample/swresample.h>
}

TextureStream::TextureStream() {

    // initialize libav
    av_register_all();
    avformat_network_init();
    
    //https://gist.github.com/shakkhar/619fd90ccbd17734089b
    avdevice_register_all();

    av_format_ctx = NULL;
    stream_idx = -1;
    av_video_stream = NULL;
    av_codec_ctx = NULL;
    av_decoder = NULL;
    av_frame = NULL;
    av_packet = NULL;
    frame_data = NULL;
    conv_ctx = NULL;
}

TextureStream::~TextureStream() {
    clear();
}

// av_err2str returns a temporary array. This doesn't work in gcc.
// This function can be used as a replacement for av_err2str.
static const char* av_make_error(int errnum) {
    static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

static AVPixelFormat correct_for_deprecated_pixel_format(AVPixelFormat pix_fmt) {
    // Fix swscaler deprecated pixel format warning
    // (YUVJ has been deprecated, change pixel format to regular YUV)
    switch (pix_fmt) {
        case AV_PIX_FMT_YUVJ420P: return AV_PIX_FMT_YUV420P;
        case AV_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUV422P;
        case AV_PIX_FMT_YUVJ444P: return AV_PIX_FMT_YUV444P;
        case AV_PIX_FMT_YUVJ440P: return AV_PIX_FMT_YUV440P;
        default:                  return pix_fmt;
    }
}

bool TextureStream::load(const std::string& _filepath, bool _vFlip) {

    // https://github.com/bartjoyce/video-app/blob/master/src/video_reader.cpp#L35-L40
    // Open the file using libavformat
    av_format_ctx = avformat_alloc_context();
    if (!av_format_ctx) {
        std::cout << "Couldn't created AVFormatContext" << std::endl;
        clear();
        return false;
    }

    av_log_set_level(AV_LOG_QUIET);

    // https://gist.github.com/rcolinray/7552384#file-gl_ffmpeg-cpp-L229
    // open video
    if (avformat_open_input(&av_format_ctx, _filepath.c_str(), NULL, NULL) < 0) {
        std::cout << "failed to open input" << std::endl;
        clear();
        return false;
    }
        
    // find stream info
    if (avformat_find_stream_info(av_format_ctx, NULL) < 0) {
        std::cout << "failed to get stream info" << std::endl;
        clear();
        return false;
    }

    // dump debug info
    av_dump_format(av_format_ctx, 0, _filepath.c_str(), 0);
        
    stream_idx = -1;
    AVCodecParameters* av_codec_params;
    AVCodec* av_codec;

    // find the video stream
    for (unsigned int i = 0; i < av_format_ctx->nb_streams; ++i) {
        av_codec_params = av_format_ctx->streams[i]->codecpar;
        av_codec = avcodec_find_decoder(av_codec_params->codec_id);
        if (!av_codec) {
            continue;
        }
        if (av_codec_params->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_width = av_codec_params->width;
            m_height = av_codec_params->height;
            time_base = av_format_ctx->streams[i]->time_base;
            stream_idx = i;
            break;
        }
    }

    if (stream_idx == -1) {
        std::cout << "failed to find video stream" << std::endl;
        clear();
        return false;
    }

    // Set up a codec context for the decoder
    av_codec_ctx = avcodec_alloc_context3(av_codec);
    if (!av_codec_ctx) {
        printf("Couldn't create AVCodecContext\n");
        return false;
    }
    if (avcodec_parameters_to_context(av_codec_ctx, av_codec_params) < 0) {
        printf("Couldn't initialize AVCodecContext\n");
        return false;
    }

    // open the decoder
    if (avcodec_open2(av_codec_ctx, av_decoder, NULL) < 0) {
        std::cout << "failed to open codec" << std::endl;
        clear();
        return false;
    }

    // allocate the video frames
    av_frame = av_frame_alloc();
    if (!av_frame) {
        std::cout << "Couldn't allocate AVFrame" << std::endl;
        return false;
    }

    av_packet = av_packet_alloc();
    if (!av_packet) {
        std::cout << "Couldn't allocate AVPacket" << std::endl;
        return false;
    }

    constexpr int ALIGNMENT = 128;
    if (posix_memalign((void**)&frame_data, ALIGNMENT, m_width * m_height * 4) != 0) {
        printf("Couldn't allocate frame buffer\n");
        return 1;
    }

    // Generate an OpenGL texture ID for this texturez
    glEnable(GL_TEXTURE_2D);
    if (m_id == 0)
        glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    return true;
}

bool TextureStream::update() {

     // Decode one frame
    int response;
    while (av_read_frame(av_format_ctx, av_packet) >= 0) {
        if (av_packet->stream_index != stream_idx) {
            av_packet_unref(av_packet);
            continue;
        }

        response = avcodec_send_packet(av_codec_ctx, av_packet);
        if (response < 0) {
            printf("Failed to decode packet: %s\n", av_make_error(response));
            return false;
        }

        response = avcodec_receive_frame(av_codec_ctx, av_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            av_packet_unref(av_packet);
            continue;
        } else if (response < 0) {
            printf("Failed to decode packet: %s\n", av_make_error(response));
            return false;
        }

        av_packet_unref(av_packet);
        break;
    }
    
    // Set up sws scaler
    if (!conv_ctx) {
        AVPixelFormat source_pix_fmt = correct_for_deprecated_pixel_format(av_codec_ctx->pix_fmt);
        conv_ctx = sws_getContext(  av_codec_ctx->width, av_codec_ctx->height, source_pix_fmt, 
                                    av_codec_ctx->width, av_codec_ctx->height, AV_PIX_FMT_RGB0,
                                    SWS_BILINEAR, NULL, NULL, NULL);
        
    }
    if (!conv_ctx) {
        printf("Couldn't initialize sw scaler\n");
        return false;
    }

    uint8_t* dest[4] = { frame_data, NULL, NULL, NULL };
    int dest_linesize[4] = { av_frame->width * 4, 0, 0, 0 };
    sws_scale(conv_ctx, av_frame->data, av_frame->linesize, 0, av_frame->height, dest, dest_linesize);
    // sws_scale(  conv_ctx, av_frame->data, av_frame->linesize, 0, av_codec_ctx->height, gl_frame->data, gl_frame->linesize);
    glBindTexture(GL_TEXTURE_2D, m_id);
    // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, av_codec_ctx->width, av_codec_ctx->height, GL_RGB, GL_UNSIGNED_BYTE, gl_frame->data[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame_data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void  TextureStream::clear() {
    if (conv_ctx)
        sws_freeContext(conv_ctx);

    if (av_frame) 
        av_free(av_frame);

    if (av_packet)
        av_free(av_packet);
        
    if (av_codec_ctx)
        avcodec_close(av_codec_ctx);
        
    if (av_format_ctx) {
        avformat_free_context(av_format_ctx);
        // avformat_close_input(&av_format_ctx);
    }

    if (frame_data) 
        delete frame_data;

    if (m_id != 0)
        glDeleteTextures(1, &m_id);

    m_id = 0;
}

#endif