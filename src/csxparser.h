#ifndef CSXPARSER_H
#define CSXPARSER_H
#include <QString>
#include <QList>
#include <QMap>

const char entisFT[] = { 'E', 'n', 't', 'i', 's', 0x1a, 0x00, 0x00 };
struct Header {
    char fileType[8]; // entisFT
    uint32_t unknown1;
    uint32_t zero;
    char imageType[40];
    uint32_t size;
    uint32_t unknown2;
};

struct Section {
    Section(){size=0; unknown=0; content=0;}
    ~Section(){if (content) delete content;}
    int read(const char *data); // Возвращает количество прочитанных байт
    char id[8];
    uint32_t size;
    uint32_t unknown;
    char *content;
};

struct CSX{
    Header header;
    Section image, function, global, data, conststr, linkinf;
    QList<uint32_t> addr_offs;
    QMap<QString,uint32_t> named_offs;
    QStringList functions;
    bool read_from_file(QString file_name); // Читает и заполняет заголовок и все секции
};

#endif // CSXPARSER_H
