#include <stdio.h>
#include <QFile>
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
    printf("Memory OK\n");
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

    printf("Reading of sections...\n");
    // Reading of sections raw data and size checks

    int pos=sizeof(Header);
    pos+=image.read(buf.constData()+pos);
    if (pos>buf.length()){
        printf("OoB in content size!\n");
        return false;
    }
    printf("Image OK\n");
    pos+=function.read(buf.constData()+pos);
    if (pos>buf.length()){
        printf("OoB in content size!\n");
        return false;
    }
    printf("Function OK\n");
    pos+=global.read(buf.constData()+pos);
    if (pos>buf.length()){
        printf("OoB in content size!\n");
        return false;
    }
    printf("Global OK\n");
    pos+=data.read(buf.constData()+pos);
    if (pos>buf.length()){
        printf("OoB in content size!\n");
        return false;
    }
    printf("Data OK\n");
    pos+=conststr.read(buf.constData()+pos);
    if (pos>buf.length()){
        printf("OoB in content size!\n");
        return false;
    }
    pos+=linkinf.read(buf.constData()+pos);
    printf("Linkinf OK\n");

    if (header.size != 16 * 6 + image.size + function.size + global.size + data.size + conststr.size + linkinf.size){
        printf("Wrong content size!\n");
        return false;
    }

    // Парсинг первого блока функций
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
    return true;
}

int Section::read(const char *data)
{
    const char *pos=data;
    memcpy(id,pos,sizeof(id));
    pos+=sizeof(id);
    memcpy(&size,pos,sizeof(size));
    pos+=sizeof(size)*2;
    if (size>50000000) return 1000000000;
    content = new char[size];
    mempcpy(content,pos,size);
    pos+=size;
    return pos-data;
}
