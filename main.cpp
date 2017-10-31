#include <QCoreApplication>
#include <QAudioOutput>
#include <QDebug>
#include "audiodata.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc,argv);

    /* Create AudioData */
    AudioData data("xxxx.mp3");
    if(!data.open())
    {
        return 1;
    }

    /* Check if device suppoerted */
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(data.format()))
    {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        return 1;
    }

    /* Create QAudioOutput and play */
    QAudioOutput audio(data.format(), nullptr);
    QObject::connect(&audio, &QAudioOutput::stateChanged, QCoreApplication::quit);
    audio.start(&data);

    return a.exec();
}


