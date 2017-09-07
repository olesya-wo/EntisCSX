#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include "csxparser.h"
#include <conio.h>

static const char int_div0[] = { 0x02, 0x00, 0x04 };
static const char int_div1[] = { 0x02, 0x01, 0x04 };
static const char str_div[] = { 0x02, 0x00, 0x06 };
static const char f_div[] = { 0x02, 0x04, 0x04 };

static const char hrPostfix[] = { 0x01, 0x02, 0x03, 0x04, 0x01, 0x00, 0x00 };
static const char hrSufffix[] = { 3, -1, 1, 2, 3, 4, 1, 0, 0, 0, 2, 0, 4, 30, 0, 0, 0, 14, 1, 2, 3, 4, 1, 0, 0, 0, 2, 0, 4, 20, 0, 0, 0, 14, 1, 12, 6, 7, 0, 70, 0, 0};
static const char chSuffix[] = { 1, 2, 0, 4, 0, 0, 0, 0, 9, 0, 6, 31, 0, 0, 0, 2, 3, 4, 1, 0, 0, 0, 2, 0, 4, 40, 0, 0, 0, 14, 1, 7, 0, 9, 0, 0, 0, 2, 0, 4, 0, 0, 0, 0, 9};
static const char chSuffix1[] = { 1, 2, 0, 4, 0, 0, 0, 0, 9, 0, 6, 31, 0, 0, 0, 2, 3, 4, 1, 0, 0, 0, 2, 0, 4, 40, 0, 0, 0, 14, 1, 7, 0, 9, 0, 0, 0, 2, 0, 4, 0, 0, 0, 0};

QString try_parse_string(char *data){
    int length=0;
    memcpy(&length,data,sizeof(length));
    if (length>250 || length<1) return "";
    char *pos=data;
    pos+=4;
    QString res;
    for (int n=0; n<length;++n)
    {
        short ch=0;
        memcpy(&ch,pos,sizeof(short));
        if (!QChar(ch).isPrint()) return "";
        res+=QChar(ch);
        pos+=2;
    }
    return res;
}

QString try_parse_asci(char *data){
    int length=0;
    memcpy(&length,data,sizeof(length));
    if (length>250 || length<1) return "";
    char *pos=data;
    pos+=4;
    QString res;
    for (int n=0; n<length;++n)
    {
        if (pos[0]<32 || pos[0]>127 || pos[1]!=0) return "";
        res+=QString(pos);
        pos+=2;
    }
    return res;
}
QString format_args(char *begin, char *end){
    if (end-begin==sizeof(hrPostfix) && memcmp(begin,hrPostfix,sizeof(hrPostfix))==0) return "<--hrp-->";
    if (end-begin==sizeof(hrSufffix) && memcmp(begin,hrSufffix,sizeof(hrSufffix))==0) return "<--hrs-->";
    if (end-begin==sizeof(chSuffix) && memcmp(begin,chSuffix,sizeof(chSuffix))==0) return "<--chs-->";
    if (end-begin==sizeof(chSuffix1) && memcmp(begin,chSuffix1,sizeof(chSuffix1))==0) return "<--chs1-->";
    QString res;
    char *pos=begin;
    while (pos<end) {
        QString s=try_parse_asci(pos);
        if (s.isEmpty())
        {
            res+=QString::number(pos[0])+" ";
            ++pos;
        }else{
            res+=s+" ";
            pos+=s.length()*2+4;
        }
    }
    return res;
}

bool str_to_file(QString s, QString file)
{
    QFile outf(file);
    if (!outf.open(QIODevice::WriteOnly)) return false;
    QTextStream out_s(&outf);
    out_s.setCodec("UTF-8");
    out_s << s;
    outf.close();
}
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    CSX csx;
    if (!csx.read_from_file("inp.csx")){
        char c=getch();
        return 0;
    }
    printf("Addr: %i\tFunctions: %i\n",csx.addr_offs.length(), csx.functions.count());

    QFile img_b("image.txt");
    img_b.open(QIODevice::WriteOnly);
    QTextStream img_out(&img_b);
    img_out.setCodec("UTF-8");
    char *pos=csx.image.content;
    char *last=csx.image.content;
    QString args;
    char call_type=0;
    char b1,b2;
    for (int i=0; i<csx.image.size-4;++i)
    {
        QString text=try_parse_asci(pos);
        if (text.isEmpty()){
            ++pos;
            continue;
        }
        call_type=0;
        if (csx.functions.contains(text) && *(pos-1)==4) call_type=1;
        b1=*(pos-6); b2=*(pos-5);
        if (b1==8 && b2==5) call_type=2;
        if (b1==8 && b2==2) call_type=3;
        if (call_type==0){++pos; continue;}
        if (call_type>1){
            // Парсинг аргументов
            int arg_cnt=0;
            memcpy(&arg_cnt,pos-4,4);
            if (arg_cnt<0 || arg_cnt>10)
            {
                printf("Error func arg - %i!\n",arg_cnt);
                break;
            }
            args = text + "( "; // Строка текущей сканируемой функции вместе с аргументами
            char *ar_pos=pos-8; // Временная позиция для сканирования влево
            int int_val;
            QString txt_val;
            bool err=false; // При считывании аргументов что-то пошло не так
            // Найти все аргументы
            for (int a=0; a<arg_cnt;++a){
                // Идём влево, пока не встретим метки аргументов
                while (memcmp(int_div0,ar_pos,3)!=0 && memcmp(int_div1,ar_pos,3)!=0 && memcmp(str_div,ar_pos,3)!=0 && memcmp(f_div,ar_pos,3)!=0)
                {
                    --ar_pos;
                    if (pos<last){
                        printf("Error func arg length: %s",text.toLatin1().constData());
                        a=arg_cnt;
                        err=true;
                        break;
                    }
                }
                // Найдено число
                if (memcmp(int_div0,ar_pos,3)==0){
                    memcpy(&int_val,ar_pos+3,4);
                    args+=QString::number(int_val)+" ";
                }
                // Найден адрес
                if (memcmp(int_div1,ar_pos,3)==0){
                    memcpy(&int_val,ar_pos+3,4);
                    args+="#"+QString::number(int_val)+" ";
                }
                // Найдена строка
                if (memcmp(str_div,ar_pos,3)==0){
                    txt_val=try_parse_string(ar_pos+3);
                    args+="\""+txt_val+"\" ";
                }
                // Найдено целое число
                if (memcmp(f_div,ar_pos,3)==0){
                    memcpy(&int_val,ar_pos+3,4);
                    args+="&"+QString::number(int_val)+" ";
                }
                --ar_pos; // Сместить ещё влево, чтобы поиск не зациклился
            }
            if (err){
                // Аргументы считать не удалось
                // Вывести RAW от last до pos, потом text
                img_out << format_args(last,pos);
                img_out << "\n"  << text + "(?)\n";
            }else{
                // Всё OK
                // pos сейчас стоит на начале имени функции, где мы её и нашли
                // ar_pos стоит в начале аргументов функции
                // Вывести RAW от last до ar_pos, потом text
                img_out << format_args(last,ar_pos);
                img_out << "\n" << args << ")"+QString::number(call_type)+"\n";
            }
        }else{
            // Даже кода вызова 8 5 найти не удалось
            // Вывести RAW от last до pos, потом text
            img_out << format_args(last,pos);
            img_out << "\n"  << text + "[?]\n";
        }
        pos+=text.length()*2+4;
        i+=text.length()*2+3;
        last=pos;
    }
    img_b.close();

    QString ftmp;
    foreach (QString fs, csx.functions) {
        ftmp+=fs+"\n";
    }
    str_to_file(ftmp,"Functions.txt");

    printf("Press any key to exit...");
    char c=getch();
    return 0;
}
