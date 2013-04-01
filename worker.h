#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QTime>
#include <QMap>

class worker : public QObject
{
    Q_OBJECT
public:
    explicit worker(QObject *parent = 0);

private:
    void logger(const QString, const bool exit = 0);
    inline void lineProcess(const QByteArray);
    inline int getMagicType(const char *, char *);
    inline int preProcessLine(const unsigned char *, const quint32);
    inline void saveQueryResult();

    quint64 counter;
    QByteArray dataForProcessing;
    QTime lastTime;
    quint32 lastPid;
    QString query;
    QMap < quint32, QString > usersByPid;
    const QString *fileName;

    
signals:
    
public slots:
    void initFile(const QString);

};

#endif // WORKER_H
