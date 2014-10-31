#include "worker.h"
#include "zlib.h"
#include <magic.h>

#include <QFile>
#include <QCoreApplication>
#include <QList>

#define CHUNK 65535
#define magickSize 1024
#define windowBits 15
#define ENABLE_ZLIB_GZIP 32

worker::worker(QObject *parent) :
    QObject(parent)
{
    fileName = NULL;
}

void worker::logger(const QString message, const bool exit)
{
    char dateTime[20] = "none"; // sizeof("2013-03-21 11:41:59")
    time_t rawtime;
    struct tm timeinfo;

    memset(&timeinfo, 0, sizeof(struct tm));
    time(&rawtime);
    localtime_r(&rawtime, &timeinfo);

    strftime(dateTime, sizeof(dateTime) , "%Y-%m-%d %H:%M:%S", &timeinfo);

    printf("%s | %s\n", dateTime, message.toLocal8Bit().constData());

    if (exit) {
        fflush(stdout);
        QCoreApplication::exit(1);
    }
}

void worker::initFile(const QString filePath)
{
    QFile logFile(filePath);
    char fileType[magickSize];

    fileName = &filePath;

    counter = 0;
    dataForProcessing.clear();
    lastPid = 0;
    query.clear();
    usersByPid.clear();

    logger("\t-> Запуск обработки файла '"+filePath+"'\n");

    if (!logFile.exists()) {
        logger("Файла '"+ filePath +"' не существует!", true);
        return;
    }

    if (!logFile.open(QIODevice::ReadOnly)) {
        logger("Ошибка открытия файла '"+ filePath +"' - "+ logFile.errorString(), true);
        return;
    }

    if (getMagicType(filePath.toLocal8Bit().constData(), fileType)) {
        // ToDo: использовать magic_errno
        return;
    }

    if (strcmp(fileType, "text/plain; charset=us-ascii") == 0 ||
            strcmp(fileType, "text/plain; charset=utf-8") == 0) {
        // Обработка в обычном текстовом виду
        quint16 countBytes = 1;
        while (countBytes != 0) {
            unsigned char tmp[CHUNK];
            countBytes = logFile.read((char*)tmp, CHUNK);

            if (preProcessLine(tmp, countBytes)) {
                logger("нет переносов строк");
            }
        }
    } else if(strcmp(fileType, "application/x-gzip; charset=binary") == 0 ||
              strcmp(fileType, "application/x-gzip; charset=unknown") == 0) {
        // Обработка в архиве gzip

        int fileHandle = logFile.handle();
        if (fileHandle == -1) {
            logger("Файл не открыт, обработка невозможна", true);
            return;
        }
        FILE* fh = fdopen(fileHandle, "rb");

        int ret;
        unsigned have;
        z_stream strm;
        unsigned char in[CHUNK];
        unsigned char out[CHUNK];

        // Подготовка inflate состояния
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;

        // Инициализация с уточнением, что это GZIP
        ret = inflateInit2(&strm, windowBits | ENABLE_ZLIB_GZIP);
        if (ret != Z_OK) {
            logger("Невозможно подготовить ZLIB!", true);
            return;
        }

        // Распаковка, пока deflate поток окончится
        while (ret != Z_STREAM_END) {
            strm.avail_in = fread(in, 1, CHUNK, fh);
            if (ferror(fh)) {
                (void)inflateEnd(&strm);
                logger("Невозможно прочитать архив, код: "+ QString::number(Z_ERRNO) +"!", true);
                return;
            }

            if (strm.avail_in == 0) {
                break;
            }

            strm.next_in = in;

            // гоним inflate() над данными, пока буффер есть
            while (strm.avail_in != 0) {
                strm.avail_out = CHUNK;
                strm.next_out = out;
                ret = inflate(&strm, Z_NO_FLUSH);
                if(ret == Z_STREAM_ERROR) {
                    logger("Ошибка потока данных!", true);
                    return;
                }

                switch (ret) {
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    logger("Невозможно распаковать архив, код: "+ QString::number(ret) +"!", true);
                    return;
                }

                have = CHUNK - strm.avail_out;

                if (preProcessLine(out, have)) {
                    logger("нет переносов строк");
                }
            }
        }

        // Чистка за собой
        (void)inflateEnd(&strm);
        if (ret != Z_STREAM_END) {
            logger("\t<- При обработке архива возникли ошибки!");
        }
    } else {
        logger("Неизвестный тип файла '"+ QByteArray::fromRawData(fileType, strlen(fileType)) +"'", true);
        return;
    }

    if (!query.isEmpty()) {
        saveQueryResult();
    }
    printf("\n");
    logger("\t<- Обработка файла '"+filePath+"' окончена, строк: "+ QString::number(counter) +" шт.");
    fflush(stdout);
    logFile.close();
}

void worker::lineProcess(const QByteArray fullLine)
{
    if (fullLine.isEmpty() || fullLine.size() < 6) { // sizeof("1 Quit")
        return;
    } else {
        ++counter;
    }

    QList<QByteArray> line = fullLine.simplified().split(' ');

    if (line.size() >= 2) {
        qint32 startAt = 0;
        qint8 type = 0;

        if (QTime::fromString(line.at(1), "h:mm:ss").isValid()) {
            startAt = 3;
            // Последнее время события
            lastTime = QTime::fromString(line.at(1), "h:mm:ss");
            if (line.size() <= 4) {
                return; // Испорчен log файл
            }
        } else {
            startAt = 1;
        }

        if (line.at(startAt) == "Query" || line.at(startAt) == "Execute" || line.at(startAt) == "Prepare") {
            type = 1;
        } else if (line.at(startAt) == "Connect") {
            type = 2;
        } else if (line.at(startAt) == "Quit") {
            type = 3;
        } else if (line.at(startAt) == "Init") {
            type = 4;
        }

        if (type > 0) {
            // Новые данные
            saveQueryResult();
            lastPid = line.at(startAt-1).toUInt();
            query.clear();
        }

        switch (type) {
        case 4:
            ++startAt;
            query = "Change DB to";
        case 1:
            for (qint32 i = startAt+1; i < line.size(); ++i) {
                if (!query.isEmpty()) {
                    query += " ";
                }
                query += line.at(i);
            }
            break;
        case 2:
            usersByPid.insert(line.at(startAt-1).toUInt(), line.at(startAt+1));
            query = "Connected";
            break;
        case 3:
            query = "Quit";
            break;
        }

        if (type > 0) {
            return;
        }
    }

    // Продолжение прошлого запроса
    if (!query.isEmpty()) {
        query += " "+fullLine.simplified();
    }
}

int worker::getMagicType(const char *filePath, char *fileType)
{
    magic_t myt = magic_open(MAGIC_ERROR|MAGIC_MIME|MAGIC_SYMLINK);
    if (magic_load(myt,NULL)) {
        logger("Ошибка при подгрузке 'default magic database'", true);
        return 1;
    }

    strncpy(fileType, magic_file(myt, filePath), magickSize);
    if (strlen(fileType) == 0) {
        logger("Неизвестный тип файла", true);
        return 2;
    }
    magic_close(myt);

    return 0;
}

int worker::preProcessLine(const unsigned char * input, const quint32 inputSize)
{

    dataForProcessing += QByteArray::fromRawData((char*)input, inputSize);
    const QList< QByteArray > lines = dataForProcessing.split('\n');
    quint32 linesSize = lines.size();
    if (linesSize > 0) {
        // Последний элемент разрезан, обработаем его отдельно
        for(quint32 i = 0; i < linesSize-1; ++i) {
            lineProcess(lines.at(i).trimmed());
        }
        dataForProcessing = lines.at(linesSize-1);
        return 0; // Успех
    }
    return 1; // Нет данных
}

void worker::saveQueryResult()
{
    printf("%s | %8s | %d | %s | %s\n",
           ((fileName) ? fileName->toLocal8Bit().constData() : ""),
           lastTime.toString("h:mm:ss").toLocal8Bit().constData(),
           lastPid,
           usersByPid.value(lastPid).toLocal8Bit().constData(),
           query.toLocal8Bit().constData());

}
