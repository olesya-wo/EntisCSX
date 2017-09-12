#ifndef CSXPARSER_H
#define CSXPARSER_H
#include <QString>
#include <QList>
#include <QMap>

static const char entisFT[] = { 'E', 'n', 't', 'i', 's', 0x1a, 0x00, 0x00 };
static const char cim[] = { 0x43, 0x6F, 0x74, 0x6F, 0x70, 0x68, 0x61, 0x20, 0x49, 0x6D, 0x61, 0x67, 0x65, 0x20, 0x66, 0x69, 0x6C, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const char image_h[] = { 'i', 'm', 'a', 'g', 'e', 0x20, 0x20, 0x20 };
static const char func_h[] = { 'f', 'u', 'n', 'c', 't', 'i', 'o', 'n' };
static const char glob_h[] = { 'g', 'l', 'o', 'b', 'a', 'l', 0x20, 0x20 };
static const char data_h[] = { 'd', 'a', 't', 'a', 0x20, 0x20, 0x20, 0x20 };
static const char const_h[] = { 'c', 'o', 'n', 's', 't', 's', 't', 'r' };
static const char link_h[] = { 'l', 'i', 'n', 'k', 'i', 'n', 'f', 0x20 };

static const char int_div0[] = { 0x02, 0x00, 0x04 };
static const char int_div1[] = { 0x02, 0x01, 0x04 };
static const char str_div[] = { 0x02, 0x00, 0x06 };
static const char f_div[] = { 0x02, 0x04, 0x04 };

static const char hrPostfix[] = { 1, 2, 3, 4, 1, 0, 0, 0 };
static const char zero[] = { 0, 0, 0, 0 };
static const char hrSufffix[] = { 3, -1, 1, 2, 3, 4, 1, 0, 0, 0, 2, 0, 4, 30, 0, 0, 0, 14, 1, 2, 3, 4, 1, 0, 0, 0, 2, 0, 4, 20, 0, 0, 0, 14, 1, 12, 6, 7, 0, 70, 0, 0, 0 };
static const char chSuffix[] = { 1, 2, 0, 4, 0, 0, 0, 0, 9, 0, 6, 31, 0, 0, 0, 2, 3, 4, 1, 0, 0, 0, 2, 0, 4, 40, 0, 0, 0, 14, 1, 7, 0, 9, 0, 0, 0, 2, 0, 4, 0, 0, 0, 0, 9, 0 };
static const char sksu[] = { 1, 2, 0, 4, 0, 0, 0, 0, 9, 0 };
static const char aft[] = { 0, 0, 0, 0, 9, 1 };

static const char rec_jmp_n[] = { 0x07, 0x00, 0x06, 0x00, 0x00, 0x00, 0x05, 0x06 };

struct Header {
    char fileType[8]; // entisFT
    uint32_t unknown1;// FF FF FF FF
    uint32_t zero;
    char imageType[40];
    uint32_t size;
    uint32_t unknown2; // 0
    Header(){memcpy(fileType,entisFT,sizeof(fileType)); unknown1=-1; zero=0; memcpy(imageType,cim,sizeof(cim)); size=0; unknown2=0;}
};

struct Section {
    char id[8];
    uint32_t size;
    uint32_t unknown;   // 0
    char *content;
    QByteArray out;
    Section(){size=0; unknown=0; content=0;}
    ~Section(){if (content) delete content;}
    int read(const char *data){
        const char *pos=data;
        memcpy(id,pos,sizeof(id));
        pos+=sizeof(id);
        memcpy(&size,pos,sizeof(size));
        pos+=sizeof(size)*2;
        if (size>50000000) return 1000000000;
        if (size>0){
            content = new char[size];
            mempcpy(content,pos,size);
            pos+=size;
        }else{
            content=0;
        }
        return pos-data;
    }
};

struct CSX{
    Header header;
    Section image, function, global, data, conststr, linkinf;
    QList<uint32_t> addr_offs;                  // Позиции @Initiasize блоков
    QMap<QString,uint32_t> named_offs;          // Позиции { } функций
    QStringList functions;                      // Просто перечисление всех функций из function блока входного csx
    QMap<QString,int> f_used;                   // Количество вызовов функций, для статистики
    QMap<QString,uint32_t> labels;              // Имя метки и её позиция в image
    QMap<uint32_t,QString> jumps;               // Адрес в image, где нужно заменить 0000 на адрес реальной метки
    int lbl_cnt=0;                              // Количество меток и их счётчик
    uint32_t i=0;                               // Либо позиция в image при декомпиляции, либо строка в файле при компиляции
    int jmp0c=0,jmp1c=0,gotoc=0,callc=0,fcnt=0; // Статистика
    QStringList data_list;                      // Список строк в файле для компиляции
    // Читает csx и заполняет заголовок и все секции
    bool read_from_file(QString file_name);
    // Считает корректной любую строку без непечатных символов
    static QString try_parse_string(char *data);
    // Считает корректной только asci строку
    static QString try_parse_asci(char *data, int size);
    // Ищет метку в районе +/-20 байт
    QString get_lbl(uint32_t addr);
    // Пытается выделить строки и известные паттерны в потоке байт, остальное вывести как есть
    QString format_args(char *begin, char *end, char *global_pos, bool simple=false);
    // Преобразует все содержащиеся в классе данные в читабельный текстовый файл
    QString decompile();
    // Читает текстовый файл и пытается заполнить все данные в классе
    bool compile(QString file_name);
    // Сохраняет все данные класса обратно в бинарный файл
    bool save_to_file(QString file_name);
    // Парсит raw-строку с байтами и прочим в массив
    QByteArray parse_raw(QString str);
    // QString to US
    QByteArray str_to_us(QString str);
    // Генерирует префикс с аргументами для вызова
    QByteArray parse_args(QString args);
};

#endif // CSXPARSER_H
