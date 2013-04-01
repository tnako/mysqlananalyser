#include "worker.h"

#include <QCoreApplication>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Использование: %s [пути до лога/ов MySQL]\n", argv[0]);
//        printf("\t --stdout\t\tРезультат выдавать в stdout\n");
        fflush(stdout);
        return 1;
    } else {
        printf("MySQLAnalyser v1.0\n");
    }

    QCoreApplication a(argc, argv);
    quint16 fileCount = 1;

    QTextCodec::setCodecForCStrings( QTextCodec::codecForName("utf8") );

    worker Analyser;

    while (fileCount < argc) {
        QMetaObject::invokeMethod( &Analyser, "initFile", Qt::QueuedConnection, Q_ARG(QString, argv[fileCount++]));
    }

    QMetaObject::invokeMethod( &a, "quit", Qt::QueuedConnection);

    return a.exec();
}
