#ifndef AUDIODATA_H
#define AUDIODATA_H

#include <QIODevice>
#include <QBuffer>
#include <QAudioFormat>

class AudioData : public QBuffer
{
public:
    AudioData(QObject* parent = nullptr);
    AudioData(const QString& filename, QObject* parent = nullptr);
    void setFile(const QString& filename);
    bool open();
    QAudioFormat format();


private:
    QString filename;
    QAudioFormat fmt;
    bool initPcmData();
};

#endif // AUDIODATA_H
