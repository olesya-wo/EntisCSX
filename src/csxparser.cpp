#include <stdio.h>
#include <QFile>
#include <QTextStream>
#include <math.h>
#include "csxparser.h"

bool CSX::read_from_file(QString file_name)
{
    if (file_name.isEmpty()){
        printf("Filename is empty!\n");
        return false;
    }
    QFile inp(file_name);
    if (!inp.open(QIODevice::ReadOnly)){
        printf("Can't open file!\n");
        return false;
    }
    QByteArray buf=inp.readAll();
    inp.close();
    printf("Memory\t\tOK\n");
    memcpy(&header,buf.constData(),sizeof(Header));

    if (memcmp(header.fileType, entisFT, sizeof(entisFT)) != 0){
        printf("Wrong file type!\n");
        return false;
    }
    if (header.zero != 0) {
        printf("Corrupted header data!\n");
        return false;
    }
    if (strncmp(header.imageType, "Cotopha Image file", 18) != 0) {
        printf("Wrong image type!\n");
        return false;
    }

    printf("Reading sections...\n");
    // Reading of sections raw data and size checks

    int pos=sizeof(Header);
    pos+=image.read(buf.constData()+pos);
    if (pos>buf.length()){
        printf("OoB in content size!\n");
        return false;
    }
    printf("'Image'\t\tOK\n");
    pos+=function.read(buf.constData()+pos);
    if (pos>buf.length()){
        printf("OoB in content size!\n");
        return false;
    }
    printf("'Function'\tOK\n");
    pos+=global.read(buf.constData()+pos);
    if (pos>buf.length()){
        printf("OoB in content size!\n");
        return false;
    }
    printf("'Global'\tOK\n");
    pos+=data.read(buf.constData()+pos);
    if (pos>buf.length()){
        printf("OoB in content size!\n");
        return false;
    }
    printf("'Data'\t\tOK\n");
    pos+=conststr.read(buf.constData()+pos);
    if (pos>buf.length()){
        printf("OoB in content size!\n");
        return false;
    }
    pos+=linkinf.read(buf.constData()+pos);
    printf("'Linkinf'\tOK\n");

    if (header.size != 16 * 6 + image.size + function.size + global.size + data.size + conststr.size + linkinf.size){
        printf("Wrong content size!\n");
        return false;
    }

    // Парсинг первого блока функций
    if (function.content){
        int funct_rec_cnt=0;
        char *offs=function.content;
        memcpy(&funct_rec_cnt,offs,sizeof(funct_rec_cnt));
        offs+=sizeof(funct_rec_cnt);
        for (int i=0; i<funct_rec_cnt;++i){
            memcpy(&pos,offs,sizeof(pos));
            addr_offs.append(pos);
            offs+=4;
        }
        offs+=sizeof(funct_rec_cnt);
        // Парсинг второго блока функций
        memcpy(&funct_rec_cnt,offs,sizeof(funct_rec_cnt));
        offs+=sizeof(funct_rec_cnt);
        for (int i=0; i<funct_rec_cnt;++i)
        {
            int length=0;
            int addr=0;
            memcpy(&addr,offs,sizeof(addr));
            offs+=sizeof(addr);
            memcpy(&length,offs,sizeof(length));
            offs+=sizeof(length);
            QString f;
            for (int n=0; n<length;++n)
            {
                f+=QString(offs);
                offs+=2;
            }
            named_offs[f]=addr;
            functions.append(f);
        }
    }
    return true;
}
QString CSX::try_parse_string(char *data)
{
    int length=0;
    memcpy(&length,data,sizeof(length));
    if (length>512 || length<1) return "";
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
QString CSX::try_parse_asci(char *data, int size)
{
    int length=0;
    if (size<6) return "";
    memcpy(&length,data,sizeof(length));
    if (length>512 || length<1) return "";
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
QString CSX::get_lbl(uint32_t addr)
{
    QMapIterator<QString, uint32_t> lbl(labels);
    while (lbl.hasNext()) {
        lbl.next();
//        if (abs(addr-lbl.value())>10 && abs(addr-lbl.value())<100){
//            printf(" %s-%i(%i) ",lbl.key().toLocal8Bit().constData(),lbl.value(),addr);
//        }
        if (lbl.value()!=0 && abs(addr-lbl.value())<21){
            return lbl.key();
        }
    }
    return "";
}
QString CSX::format_args(char *begin, char *end, char *global_pos, bool simple)
{
    QString res;
    if (!simple){
        if (end-begin==1 && *begin==1) return "<br>";
        if (global_pos){
            // 1 6 jump
            if (end-begin>=6 && *begin==1 && *(begin+1)==6){
                int jmp=0;
                memcpy(&jmp,begin+2,sizeof(int));
                int r_j=i-(global_pos-begin+6)+jmp;
                QString lbl=get_lbl(r_j);
                if (lbl.isEmpty()){
                    ++lbl_cnt;
                    lbl="l"+QString::number(lbl_cnt);
                    labels[lbl]=r_j;
                }
                ++jmp1c;
                res+= "jump1 "+lbl+"\n";
                begin+=+6;
            }
            // 0 6 jump
            if (end-begin>=6 && *begin==0 && *(begin+1)==6){
                int jmp=0;
                memcpy(&jmp,begin+2,sizeof(int));
                int r_j=i-(global_pos-begin+6)+jmp;
                QString lbl=get_lbl(r_j);
                if (lbl.isEmpty()){
                    ++lbl_cnt;
                    lbl="l"+QString::number(lbl_cnt);
                    labels[lbl]=r_j;
                }
                ++jmp0c;
                res+= "jump0 "+lbl+"\n";
                begin+=+6;
            }
        }
        if (end-begin==sizeof(hrPostfix) && memcmp(begin,hrPostfix,sizeof(hrPostfix))==0) return res+"<--hrp-->";
        if (end-begin==sizeof(zero) && memcmp(begin,zero,sizeof(zero))==0) return res+"<--zero-->";
        if (end-begin==sizeof(hrSufffix) && memcmp(begin,hrSufffix,sizeof(hrSufffix))==0) return res+"<--hrs-->";
        if (end-begin==sizeof(chSuffix) && memcmp(begin,chSuffix,sizeof(chSuffix))==0) return res+"<--chs-->";
        if (end-begin==sizeof(sksu) && memcmp(begin,sksu,sizeof(sksu))==0) return res+"<--sksu-->";
        if (end-begin==sizeof(aft) && memcmp(begin,aft,sizeof(aft))==0) return res+"<--aft-->";
    }
    char *pos=begin;
    while (pos<end) {
        QString s=try_parse_asci(pos,end-begin);
        if (s.isEmpty() || (s.contains("\"") && s.contains("'")))
        {
            res+=QString::number(pos[0])+" ";
            ++pos;
        }else{
            QString d="\"";
            if (s.contains(d)) d="'";
            res+=d+s+d+" ";
            pos+=s.length()*2+4;
        }
    }
    return res;
}
QString CSX::decompile()
{
    QString res;
    char *pos=image.content;
    char *last=image.content;
    QString args;
    char call_type=0;
    char b1,b2;
    // Короткие прыжки
    int total=addr_offs.length();
    int current=0;
    for (i=0; i<image.size;++i)
    {
        // Сколько байт осталось сканировать
        int nok_size=image.size-i;
        // Вывести прогресс
        foreach (uint32_t addr, addr_offs) {
            if (i>=addr){
                addr_offs.removeAll(addr);
                if (current!=0){
                    if (current % 50 == 0)
                        printf("%i/%i\n",current,total);
                    else
                        printf(".");
                }
                ++current;
                break;
            }
        }
        // Вывести метки
        QMapIterator<QString, uint32_t> lbl(labels);
        while (lbl.hasNext()) {
            lbl.next();
            if (lbl.value()!=0 && i>=lbl.value()){
                res+= format_args(last,pos,0);
                last=pos;
                res+= QString("\n") + "label "+lbl.key()+":\n";
                labels[lbl.key()]=0;
                break;
            }
        }
        // Начать анализ с найденной строки
        QString text=try_parse_asci(pos,nok_size);
        if (text.isEmpty()){
            ++pos;
            continue;
        }
        call_type=0;
        if (text=="@Initialize" && *(pos-1)==4){
            res+= format_args(last,pos-1,0);
            res+= "\n### @Initialize ###\n";
            pos+=text.length()*2+4;
            i+=text.length()*2+3;
            last=pos;
            continue;
        }
        if (functions.contains(text) && *(pos-1)==4) call_type=1;
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
                printf(" Error func arg - %i! ",arg_cnt);
                break;
            }
            args = text + "( "; // Строка текущей сканируемой функции вместе с аргументами
            char *ar_pos=pos-6; // Временная позиция для сканирования влево
            int int_val;
            QString txt_val;
            bool err=false; // При считывании аргументов что-то пошло не так
            int arg_len=0;
            // Найти все аргументы
            for (int a=0; a<arg_cnt && call_type==2;++a){
                // Идём влево, пока не встретим метки аргументов
                while (memcmp(int_div0,ar_pos,3)!=0 && memcmp(int_div1,ar_pos,3)!=0 && memcmp(str_div,ar_pos,3)!=0 && memcmp(f_div,ar_pos,3)!=0)
                {
                    --ar_pos;
                    if (ar_pos<last){
                        a=arg_cnt;
                        err=true;
                        break;
                    }
                }
                // Найдено число
                if (memcmp(int_div0,ar_pos,3)==0){
                    memcpy(&int_val,ar_pos+3,4);
                    args+=QString::number(int_val)+" ";
                    arg_len+=7;
                }
                // Найден адрес
                if (memcmp(int_div1,ar_pos,3)==0){
                    memcpy(&int_val,ar_pos+3,4);
                    args+="#"+QString::number(int_val)+" ";
                    arg_len+=7;
                }
                // Найдена строка
                if (memcmp(str_div,ar_pos,3)==0){
                    txt_val=try_parse_string(ar_pos+3);
                    if (txt_val.contains("\"") && txt_val.contains("'")){
                        printf("Unexpected string argument: %s",txt_val.toLatin1().constData());
                        res.clear();
                        return res;
                    }
                    if (txt_val.contains("\""))
                        args+="'"+txt_val+"' ";
                    else
                        args+="\""+txt_val+"\" ";
                    arg_len+=3+txt_val.length()*2+4;
                }
                // Найдено целое число
                if (memcmp(f_div,ar_pos,3)==0){
                    memcpy(&int_val,ar_pos+3,4);
                    args+="&"+QString::number(int_val)+" ";
                    arg_len+=7;
                }
                if (a<arg_cnt-1) --ar_pos; // Сместить ещё влево, чтобы поиск не зациклился
            }
            if (err || call_type==3 || pos-ar_pos!=arg_len+6){
                // Аргументы считать не удалось
                // Вывести RAW от last до pos, потом text
                res+= format_args(last,pos,0);
                res+= "\n" + text + "(?)\n";
            }else{
                // Всё OK
                // pos сейчас стоит на начале имени функции, где мы её и нашли
                // ar_pos стоит в начале аргументов функции
                // Вывести RAW от last до ar_pos, потом text
                res+= format_args(last,ar_pos,pos);
                if (call_type==2)
                    res+= "\n" + args + ")\n";
                else
                    res+= "\n" + args + ")"+QString::number(call_type)+"\n";
                // Количество использований
                if (call_type==2) {++f_used[text]; ++fcnt;}
            }
        }else{
            // Даже кода вызова 8 5 найти не удалось
            // Вывести RAW от last до pos-1, потом text
            res+= format_args(last,pos-1,0);
            res+= "\n" + text + "{ }\n";
        }
        pos+=text.length()*2+4;
        i+=text.length()*2+3;
        // Короткие прыжки после некоторых функций
        if (call_type==2 && (text=="ChkSelect" || text=="IsGameClear" || text=="ChkFlagOn")){
            b1=*pos; b2=*(pos+1);
            if (b1==7 && b2==0){
                int jmp=0;
                memcpy(&jmp,pos+2,sizeof(int));
                QString lbl=get_lbl(i+jmp+1+6);
                if (lbl.isEmpty()){
                    ++lbl_cnt;
                    lbl="l"+QString::number(lbl_cnt);
                    labels[lbl]=i+jmp+1+6;
                }
                pos+=6;
                i+=6;
                res+= "goto "+lbl+"\n";
                ++gotoc;
            }
        }
        if (call_type==2 && text=="IsRecollectMode" && memcmp(pos,rec_jmp_n,sizeof(rec_jmp_n))!=0){
            b1=*pos; b2=*(pos+1);
            if (b1==7 && b2==0){
                int jmp=0;
                memcpy(&jmp,pos+2,sizeof(int));
                QString lbl=get_lbl(i+jmp+1+6);
                if (lbl.isEmpty()){
                    ++lbl_cnt;
                    lbl="l"+QString::number(lbl_cnt);
                    labels[lbl]=i+jmp+1+6;
                }
                pos+=6;
                i+=6;
                res+= "goto "+lbl+"\n";
                ++gotoc;
            }
        }
        if (call_type==2 && text=="IsRecollectMode" && memcmp(pos,rec_jmp_n,sizeof(rec_jmp_n))==0){
            int jmp=0;
            memcpy(&jmp,pos+8,sizeof(int));
            QString lbl=get_lbl(i+jmp+1+12);
            if (lbl.isEmpty()){
                ++lbl_cnt;
                lbl="l"+QString::number(lbl_cnt);
                labels[lbl]=i+jmp+1+12;
            }
            pos+=12;
            i+=12;
            res+= "call "+lbl+"\n";
            ++callc;
        }
        last=pos;
    }
    res+= format_args(last,pos,0);
    // global
    res+= "\n\n@@@ global\n";
    res+= format_args(global.content,global.content+global.size,0,true)+"\n";
    res+= "\n@@@ data\n";
    res+= format_args(data.content,data.content+data.size,0,true)+"\n";
    res+= "\n@@@ conststr\n";
    res+= format_args(conststr.content,conststr.content+conststr.size,0,true)+"\n";
    res+= "\n@@@ linkinf\n";
    res+= format_args(linkinf.content,linkinf.content+linkinf.size,0,true)+"\n";
    return res;
}

bool CSX::compile(QString file_name)
{
    QFile inp(file_name);
    if (!inp.open(QIODevice::ReadOnly)){
        printf("Can't open file!\n");
        return false;
    }
    QTextStream inp_s(&inp);
    inp_s.setCodec("UTF-8");
    data_list=inp_s.readAll().replace("\r","").split("\n");
    inp.close();
    // image
    for (i=0; i<data_list.length();++i){
        QString line=data_list[i];
        if (i!=0){
            if (i % 50000 == 0)
                printf("%i/%i\n",i,data_list.length());
            else if (i % 1000 == 0)
                printf(".");
        }
        // trim
        while (line.left(1)==" " || line.left(1)=="\t") line=line.mid(1);
        // comment
        if (line.left(2)=="//") continue;
        //
        if (line.isEmpty()) continue;
        if (line=="<--hrp-->"){
            image.out.append(hrPostfix,sizeof(hrPostfix));
            continue;
        }
        if (line=="<--zero-->"){
            image.out.append(zero,sizeof(zero));
            continue;
        }
        if (line=="<--hrs-->"){
            image.out.append(hrSufffix,sizeof(hrSufffix));
            continue;
        }
        if (line=="<--chs-->"){
            image.out.append(chSuffix,sizeof(chSuffix));
            continue;
        }
        if (line=="<--sksu-->"){
            image.out.append(sksu,sizeof(sksu));
            continue;
        }
        if (line=="<--aft-->"){
            image.out.append(aft,sizeof(aft));
            continue;
        }
        if (line=="<br>"){
            image.out.append((char)1);
            continue;
        }
        if (line=="### @Initialize ###"){
            addr_offs.append(image.out.length());
            image.out.append((char)4);
            image.out.append(str_to_us("@Initialize"));
            continue;
        }
        if (line=="@@@ global"){
            if (i<data_list.length()-1){
                if (!global.out.isEmpty()){
                    printf("Multiple global!\n");
                    return false;
                }
                QByteArray tmp=parse_raw(data_list[i+1]);
                global.out=tmp;
            }else{
                printf("Unexpected EOF!\n");
                return false;
            }
            ++i;
            continue;
        }
        if (line=="@@@ data"){
            if (i<data_list.length()-1){
                if (!data.out.isEmpty()){
                    printf("Multiple data!\n");
                    return false;
                }
                QByteArray tmp=parse_raw(data_list[i+1]);
                data.out=tmp;
            }else{
                printf("Unexpected EOF!\n");
                return false;
            }
            ++i;
            continue;
        }
        if (line=="@@@ conststr"){
            if (i<data_list.length()-1){
                if (!conststr.out.isEmpty()){
                    printf("Multiple conststr!\n");
                    return false;
                }
                QByteArray tmp=parse_raw(data_list[i+1]);
                conststr.out=tmp;
            }else{
                printf("Unexpected EOF!\n");
                return false;
            }
            ++i;
            continue;
        }
        if (line=="@@@ linkinf"){
            if (i<data_list.length()-1){
                if (!linkinf.out.isEmpty()){
                    printf("Multiple linkinf!\n");
                    return false;
                }
                QByteArray tmp=parse_raw(data_list[i+1]);
                linkinf.out=tmp;
            }else{
                printf("Unexpected EOF!\n");
                return false;
            }
            ++i;
            continue;
        }
        if (line.right(2)==" )"){
            int pos=line.indexOf("( ");
            if (pos<1){
                printf("Error in line %i!\n'%s'\n",i,line.toLocal8Bit().constData());
                return false;
            }
            QString fname=line.left(pos);
            QByteArray tmp=parse_args(line.mid(pos+2,line.length()-pos-3));
            if (tmp.isEmpty()) return false;
            image.out.append(tmp);
            image.out.append(str_to_us(fname));
            continue;
        }
        if (line.right(3)=="{ }"){
            named_offs[line.left(line.length()-3)]=image.out.length();
            image.out.append((char)4);
            image.out.append(str_to_us(line.left(line.length()-3)));
            continue;
        }
        if (line.right(3)=="(?)"){
            image.out.append(str_to_us(line.left(line.length()-3)));
            continue;
        }
        if (line.left(6)=="label "){
            labels[line.mid(6,line.length()-7)]=image.out.length();
            continue;
        }
        if (line.left(5)=="goto "){
            image.out.append((char)7);
            image.out.append((char)0);
            jumps[image.out.length()]=line.mid(5);
            image.out.append((char)0);
            image.out.append((char)0);
            image.out.append((char)0);
            image.out.append((char)0);
            continue;
        }
        if (line.left(5)=="call "){
            image.out.append(rec_jmp_n,sizeof(rec_jmp_n));
            jumps[image.out.length()]=line.mid(5);
            image.out.append((char)0);
            image.out.append((char)0);
            image.out.append((char)0);
            image.out.append((char)0);
            continue;
        }
        if (line.left(6)=="jump0 "){
            image.out.append((char)0);
            image.out.append((char)6);
            jumps[image.out.length()]=line.mid(6);
            image.out.append((char)0);
            image.out.append((char)0);
            image.out.append((char)0);
            image.out.append((char)0);
            continue;
        }
        if (line.left(6)=="jump1 "){
            image.out.append((char)1);
            image.out.append((char)6);
            jumps[image.out.length()]=line.mid(6);
            image.out.append((char)0);
            image.out.append((char)0);
            image.out.append((char)0);
            image.out.append((char)0);
            continue;
        }
        QByteArray tmp=parse_raw(line);
        if (tmp.isEmpty()) return false;
        image.out.append(tmp);
    }
    // function
    char dw[4];
    int len=addr_offs.length();
    memcpy(dw,&len,sizeof(int));
    function.out.append(dw,4);
    foreach (uint32_t a, addr_offs) {
        memcpy(dw,&a,sizeof(int));
        function.out.append(dw,4);
    }
    len=0;
    memcpy(dw,&len,sizeof(int));
    function.out.append(dw,4);
    len=named_offs.size();
    memcpy(dw,&len,sizeof(int));
    function.out.append(dw,4);
    QMapIterator<QString, uint32_t> p(named_offs);
    while (p.hasNext()) {
        p.next();
        len=p.value();
        memcpy(dw,&len,sizeof(int));
        function.out.append(dw,4);
        function.out.append(str_to_us(p.key()));
    }
    // Обработать переходы
    QMapIterator<uint32_t,QString> j(jumps);
    while (j.hasNext()) {
        j.next();
        int addr=labels[j.value()];
        if (addr==0){
            printf("Label '%s' not found!\n",j.value().toLatin1().constData());
        }else{
            addr=addr-j.key()-4;
            memcpy(dw,&addr,sizeof(int));
            image.out[j.key()]=dw[0];
            image.out[j.key()+1]=dw[0+1];
            image.out[j.key()+2]=dw[0+2];
            image.out[j.key()+3]=dw[0+3];
        }
    }
    return true;
}
bool CSX::save_to_file(QString file_name)
{
    QFile out(file_name);
    if (!out.open(QIODevice::WriteOnly)){
        printf("Can't save file!\n");
        return false;
    }
    image.size=image.out.length();
    function.size=function.out.length();
    global.size=global.out.length();
    data.size=data.out.length();
    conststr.size=conststr.out.length();
    linkinf.size=linkinf.out.length();
    header.size=16 * 6 + image.size + function.size + global.size + data.size + conststr.size + linkinf.size;
    // Header
    char *h=new char[sizeof(header)];
    char dw[4];
    memcpy(h,&header,sizeof(header));
    out.write(h,sizeof(header));
    delete h; h=0;
    // image
    out.write(image_h,sizeof(image_h));
    memcpy(dw,&image.size,4);
    out.write(dw,4);
    memset(dw,0,4);
    out.write(dw,4);
    out.write(image.out);
    // function
    out.write(func_h,sizeof(func_h));
    memcpy(dw,&function.size,4);
    out.write(dw,4);
    memset(dw,0,4);
    out.write(dw,4);
    out.write(function.out);
    // global
    out.write(glob_h,sizeof(glob_h));
    memcpy(dw,&global.size,4);
    out.write(dw,4);
    memset(dw,0,4);
    out.write(dw,4);
    out.write(global.out);
    // data
    out.write(data_h,sizeof(data_h));
    memcpy(dw,&data.size,4);
    out.write(dw,4);
    memset(dw,0,4);
    out.write(dw,4);
    out.write(data.out);
    // conststr
    out.write(const_h,sizeof(const_h));
    memcpy(dw,&conststr.size,4);
    out.write(dw,4);
    memset(dw,0,4);
    out.write(dw,4);
    out.write(conststr.out);
    // linkinf
    out.write(link_h,sizeof(link_h));
    memcpy(dw,&linkinf.size,4);
    out.write(dw,4);
    memset(dw,0,4);
    out.write(dw,4);
    out.write(linkinf.out);
    out.close();
    return true;
}
QByteArray CSX::parse_raw(QString str)
{
    QByteArray res;
    // 0 - start
    // 1 - in ' string
    // 2 - in " string
    // 3 - in byte
    // 4 - need space
    char state=0;
    QString byte;
    QString string;
    for(int i=0; i<str.length();++i){
        QString ch=str.mid(i,1);
        //
        if (ch=="'" && state==0){ // Начало ' строки
            state=1;
            string.clear();
            byte.clear();
            continue;
        }
        if (ch=="\"" && state==0){ // Начало " строки
            state=2;
            string.clear();
            byte.clear();
            continue;
        }
        if (ch=="'" && state==1){ // Конец ' строки
            state=4; // need space
            res+=str_to_us(string);
            string.clear();
            continue;
        }
        if (ch=="\"" && state==2){ // Конец " строки
            state=4; // need space
            res+=str_to_us(string);
            string.clear();
            continue;
        }
        if (state==1 || state==2){ // Продолжение строки
            string+=ch;
            continue;
        }
        //
        if (ch==" " && state==0) continue; // Несколько пробелов подряд или в начале строки
        if (ch==" " && state==4) {state=0; continue;} // Пробел после строки
        if (ch==" " && state==3){ // Число кончилось
            state=0;
            int n=byte.toInt();
            if (n>128 || n<-128){
                printf("Error in line %i, position %i!\n'%s'\n",this->i,i,str.toLocal8Bit().constData());
                res.clear();
                return res;
            }
            res.append((char)n);
            byte.clear();
            continue;
        }
        if (ch=="-" && state==0){ // Начало отрицательного числа
            state=3;
            byte=ch;
            continue;
        }
        if (str.at(i).isDigit() && state==0){ // Начало положительного числа
            state=3;
            byte=ch;
            continue;
        }
        if (str.at(i).isDigit() && state==3){ // Продолжение числа
            byte+=ch;
            continue;
        }
        // Что-то пошло не так
        printf("Unexpected char in line %i, position %i!\n'%s'\n",this->i,i,str.toLocal8Bit().constData());
        res.clear();
        return res;
    }
    return res;
}
QByteArray CSX::str_to_us(QString str)
{
    char dw[4];
    char sh[2];
    int len=str.length();
    memcpy(dw,&len,sizeof(int));
    QByteArray res;
    bool asci=true;
    res.append(dw,4);
    for(int i=0; i<len;++i){
        ushort ch=str.at(i).unicode();
        memcpy(sh,&ch,sizeof(ushort));
        res.append(sh,2);
        if (sh[1]!=0) asci=false;
    }
    return res;
}
QByteArray CSX::parse_args(QString args)
{
    // 1 #2 &3 "4" '5"str"5'
    QByteArray res;
    int count=0;
    char dw[4];
    for(int i=0; i<args.length();++i){
        if (args.mid(i,1)==" ") continue;
        int pos=args.indexOf(" ",i);
        QString buf;
        if (pos<i){ // Конец аргументов
            buf=args.mid(i);
            i=args.length();
        }else{
            buf=args.mid(i,pos-i);
            if (buf.left(1)=="\""){
                // Строку нужно выделить отдельно
                pos=args.indexOf("\"",i+1);
                if (pos<i){
                    printf("String error in arguments! Line: %i\n'%s'\n",this->i,args.toLatin1().constData());
                    res.clear();
                    return res;
                }
                buf=args.mid(i,pos-i+1);
            }
            if (buf.left(1)=="'"){
                // Строку нужно выделить отдельно
                pos=args.indexOf("'",i+1);
                if (pos<i){
                    printf("String error in arguments! Line: %i\n'%s'\n",this->i,args.toLatin1().constData());
                    res.clear();
                    return res;
                }
                buf=args.mid(i,pos-i+1);
            }
            i=pos;
        }
        bool ok;
        ++count;
        if (buf.left(1)=="#"){
            pos=buf.mid(1).toInt(&ok);
            if (!ok){
                printf("Error # in arguments! Line: %i\n'%s'\n",this->i,args.toLatin1().constData());
                res.clear();
                return res;
            }
            memcpy(dw,&pos,sizeof(int));
            res.prepend(dw,4);
            res.prepend(int_div1,sizeof(int_div1));
        }else if(buf.left(1)=="&"){
            pos=buf.mid(1).toInt(&ok);
            if (!ok){
                printf("Error & in arguments! Line: %i\n'%s'\n",this->i,args.toLatin1().constData());
                res.clear();
                return res;
            }
            memcpy(dw,&pos,sizeof(int));
            res.prepend(dw,4);
            res.prepend(f_div,sizeof(f_div));
        }else if(buf.left(1)=="\"" || buf.left(1)=="'"){
            QByteArray tmp=str_to_us(buf.mid(1,buf.length()-2));
            res.prepend(tmp);
            res.prepend(str_div,sizeof(str_div));
        }else{
            pos=buf.toInt(&ok);
            if (!ok){
                printf("Error INT in arguments! Line: %i\n'%s'\n",this->i,buf.toLatin1().constData());
                res.clear();
                return res;
            }
            memcpy(dw,&pos,sizeof(int));
            res.prepend(dw,4);
            res.prepend(int_div0,sizeof(int_div0));
        }
    }
    res.append((char)8);
    res.append((char)5);
    memcpy(dw,&count,sizeof(int));
    res.append(dw,4);
    return res;
}

