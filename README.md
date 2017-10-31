# Qt-FFmpeg-Demo
Use Qt's QAudioOutput to play pcm AudioData decoded by FFmpeg

## Minimum Demo
```C++
#include <QCoreApplication>
#include <QAudioOutput>
#include "audiodata.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc,argv);
    AudioData data("xxxx.mp3");
    data.open();
    QAudioOutput audio(data.format(), nullptr);
    audio.start(&data);

    return a.exec();
}
```
