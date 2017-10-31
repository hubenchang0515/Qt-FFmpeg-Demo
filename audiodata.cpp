#include "audiodata.h"
#include <QDebug>

AudioData::AudioData(QObject* parent) :
    QBuffer(parent)
{

}

AudioData::AudioData(const QString& filename, QObject* parent) :
    QBuffer(parent)
{
    this->filename = filename;
}

void AudioData::setFile(const QString& filename)
{
    this->filename = filename;
}

bool AudioData::open()
{
    if(QBuffer::open(ReadWrite|Truncate)  && initPcmData())
    {
        QBuffer::seek(0);
        return true;
    }

    return false;
}

QAudioFormat AudioData::format()
{
    return fmt;
}








/********************/
#ifdef __cplusplus
    extern "C"{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
    }
#endif

bool AudioData::initPcmData()
{
    av_register_all();

#define AUDIO_FILE  (this->filename.toStdString().c_str())

    AVFormatContext* pFormatCtx = avformat_alloc_context();
    if(avformat_open_input(&pFormatCtx, AUDIO_FILE, NULL, NULL) != 0)
    {
        qWarning() << ("Couldn't open input stream.\n");
        return false;
    }

    if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        qWarning() << ("Couldn't find stream information.\n");
        return false;
    }

    av_dump_format(pFormatCtx, 0, AUDIO_FILE, 0);
#undef AUDIO_FILE

    int audioStream = -1;
    for(size_t i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioStream = i;
            break;
        }
    }

    if(audioStream == -1)
    {
        qWarning() << ("Did't find audio stream.\n");
        return false;
    }

    AVCodecContext* pCodecCtx = pFormatCtx->streams[audioStream]->codec;

    /* Set QAudioFormat */
    fmt.setCodec("audio/pcm");
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setSampleRate(pCodecCtx->sample_rate);
    fmt.setChannelCount(pCodecCtx->channels);
    AVSampleFormat out_sample_fmt; // used in decode
    switch(pCodecCtx->sample_fmt)
    {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
        out_sample_fmt = AV_SAMPLE_FMT_U8; // U8P is not supported.
        fmt.setSampleSize(8);
        fmt.setSampleType(QAudioFormat::UnSignedInt);
        break;

    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
        out_sample_fmt = AV_SAMPLE_FMT_S16;
        fmt.setSampleSize(16);
        fmt.setSampleType(QAudioFormat::SignedInt);
        break;

    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        out_sample_fmt = AV_SAMPLE_FMT_S32;
        fmt.setSampleSize(32);
        fmt.setSampleType(QAudioFormat::SignedInt);
        break;

    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
        out_sample_fmt = AV_SAMPLE_FMT_FLT;
        fmt.setSampleSize(32);
        fmt.setSampleType(QAudioFormat::Float);
        break;

    default:
        qWarning() << "AudioData : Unsupported Audio Format";
    }

    AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec == NULL)
    {
        qWarning() << ("Codec not found.\n");
        return false;
    }

    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        qWarning() << ("Couldn't open codec.\n");
        return false;
    }

    // set FFmpeg decode format
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    int out_nb_samples = pCodecCtx->frame_size;
    int out_sample_rate = pCodecCtx->sample_rate;
    int out_channels = pCodecCtx->channels;
    int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

    uint8_t* out_buffer = (uint8_t*)av_malloc(out_buffer_size);
    AVFrame* pFrame = av_frame_alloc();

    int64_t in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);

    struct SwrContext *au_convert_ctx = swr_alloc();
    au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout,
        out_sample_fmt, out_sample_rate, in_channel_layout, pCodecCtx->sample_fmt,
        pCodecCtx->sample_rate, 0, NULL);
    swr_init(au_convert_ctx);

    // decode
    int index = 0;
    AVPacket* packet = (AVPacket*) av_malloc(sizeof(AVPacket));
    av_init_packet(packet);
    while(av_read_frame(pFormatCtx, packet) >= 0)
    {
        if(packet->stream_index == audioStream)
        {
            int got_picture = 0;
            int ret = avcodec_decode_audio4(pCodecCtx, pFrame, &got_picture, packet);
            if(ret < 0)
            {
                // May decode a part of data successfully.
                // print warning but return TRUE
                qWarning() << ("Error in decoding audio frame.\n");
                return true;
            }

            if(got_picture > 0)
            {
                swr_convert(au_convert_ctx, &out_buffer, out_buffer_size,
                    (const uint8_t**)pFrame->data, pFrame->nb_samples);
                write((char*)out_buffer, out_buffer_size);
                index++;
            }
        }


    }

    av_free_packet(packet);
    swr_free(&au_convert_ctx);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return true;
}
