#include <QCoreApplication>
#include <QTextCodec>

#include "worker.h"

#include <getopt.h>

#define PACKAGE "MySQLAnalyser"
#define VERSION "1.2"
#define PACKAGE_STRING PACKAGE " v" VERSION


// Короткия параметры для getopt()
static const char *optString = "Vh?";

// Длинные параметры для getopt()
static const struct option longOpts[] = {
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'V' },
    { NULL, no_argument, NULL, 0 }
};

static void display_usage()
{
    fprintf(stdout, "Использование: ./%s [КЛЮЧ] [ПУТЬ ...]\n", PACKAGE);
    fprintf(stdout, "Программа изменяет стандартный лог MySQL до вида: время, pid, пользователь, запрос.\n\n");
    fprintf(stdout, "Аргументы, обязательные для длинных ключей, обязательны и для коротких.\n");
    fprintf(stdout, "\t-V, --version\t\tнапечатать информацию о версии и закончить работу.\n\n");
    fprintf(stdout, "\t-h, --help\t\tпоказать эту справку и закончить работу.\n");
    fprintf(stdout, "\nПример использования:\n");
    fprintf(stdout, "\t./%s /var/log/mysql/mysql.log\n", PACKAGE);
    fprintf(stdout, "\tНачнёт обрабатывать файл '/var/log/mysql/mysql.log' и выдавать результат в стандартный вывод.\n");
    fprintf(stdout, "\nОб ошибках сообщайте по адресу <chado@bingo-boom.ru>\n");

    exit(EXIT_SUCCESS);
}

static void display_version()
{
    fprintf(stdout, "%s\n", PACKAGE_STRING);
    fprintf(stdout, "Нет НИКАКИХ ГАРАНТИЙ до степени, разрешённой законом.\n");
    fprintf(stdout, "Автор программ - C.H.A.D.o\n");

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        display_usage();
    }

    int longIndex = 0;
    int opt = 0;

    // Проверяем входящии параметры/аргументы
    while( (opt = getopt_long(argc, argv, optString, longOpts, &longIndex)) != -1 ) {
        switch( opt ) {
        case 'V':   // Отображение версии
            display_version();
            break;
        case 'h':   // Отображение справки по программе и выход
        case '?':
            display_usage();
            break;
        }
    }

    if (optind >= argc) {
        display_usage();
    }

    QCoreApplication a(argc, argv);

    QTextCodec::setCodecForCStrings( QTextCodec::codecForName("utf8") );

    worker Analyser;

    // Оставшиеся аргументы считаем путями
    while (optind < argc) {
        QMetaObject::invokeMethod( &Analyser, "initFile", Qt::QueuedConnection, Q_ARG(QString, argv[optind++]));
    }

    QMetaObject::invokeMethod( &a, "quit", Qt::QueuedConnection);

    return a.exec();
}
