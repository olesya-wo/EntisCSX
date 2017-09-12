#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include "csxparser.h"
#include <conio.h>

bool str_to_file(QString s, QString file)
{
    QFile outf(file);
    if (!outf.open(QIODevice::WriteOnly)) return false;
    QTextStream out_s(&outf);
    out_s.setCodec("UTF-8");
    out_s << s;
    outf.close();
    return true;
}
void usage(){
#ifdef WIN32
#define exe "csxtool.exe"
#else
#define exe "csxtool"
#endif
    printf("Entis CSX (Cotopha Image file) compiler/decompiler tool.\n");
    printf("Warning, the compiler does not provide 100% compatibility of the output file!\n\n");
    printf("Usage:\n");
    printf("%s -d -i <csx_file> [-o <txt_file>]\n",exe);
    printf("%s -c -i <txt_file> [-o <csx_file>]\n\n",exe);
    printf("Press any key to exit...");
}
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    char c=0;
    bool compile=false;
    QString inp_f;
    QString out_f;
    QFile inp_ch;
    if (a.arguments().length()==4 || a.arguments().length()==6){
        if (a.arguments().at(1)=="-c"){
            compile=true;
        }else if(a.arguments().at(1)=="-d"){
             compile=false;
        }else{
            usage();
            c=getch();
            return 0;
        }
        if (a.arguments().at(2)!="-i"){
            usage();
            c=getch();
            return 0;
        }
        inp_f=a.arguments().at(3);
        inp_ch.setFileName(inp_f);
        if (!inp_ch.exists()){
            printf("Input file not found!\nPress any key to exit...");
            c=getch();
            return 0;
        }
    }else{
        usage();
        c=getch();
        return 0;
    }
    if (a.arguments().length()==6){
        if (a.arguments().at(4)!="-o"){
            usage();
            c=getch();
            return 0;
        }
        out_f=a.arguments().at(5);
        if (inp_f==out_f){
            printf("Input file is the same as output file!\nPress any key to exit...");
            c=getch();
            return 0;
        }
        QFile out_ch(out_f);
        if (!out_ch.open(QIODevice::WriteOnly)){
            printf("Can't save output file!\nPress any key to exit...");
            c=getch();
            return 0;
        }else{
            out_ch.close();
            out_ch.remove();
        }
    }else{
        QFileInfo inf(inp_ch);
        out_f=inf.absoluteFilePath().left(inf.absoluteFilePath().length()-inf.completeSuffix().length()-1);
        if (compile)
            out_f+=".csx";
        else
            out_f+=".txt";
    }
    CSX csx;
    if (compile){
        if (!csx.compile(inp_f)){
            printf("Fail!\n");
        }else{
            csx.save_to_file(out_f);
            printf("Complete!\n");
        }
    }else{
        if (!csx.read_from_file(inp_f)){
            c=getch();
            return 0;
        }
        printf("\nChunks: %i DefFunctions: %i\n\nDecompiling...\n",csx.addr_offs.length(), csx.functions.count());

        QString res=csx.decompile();
        if (res.isEmpty()){
            printf("Fail!\n");
            printf("Press any key to exit...");
            c=getch();
            return 0;
        }
        if (!str_to_file(res,out_f)){
            printf("Can't save file!\n");
            printf("Press any key to exit...");
            c=getch();
            return 0;
        }

        // Количество использований в файл
        QString ftmp;
        QMapIterator<QString, int> f(csx.f_used);
        while (f.hasNext()) {
            f.next();
            //if (f.value()>1)
            ftmp+=QString::number(f.value())+"\t"+f.key()+"\n";
        }
        str_to_file(ftmp,out_f.left(out_f.length()-4)+"_f.txt");

        printf("Complete!\n");
        printf("Functions:\t%i\n",csx.fcnt);
        printf("'label':\t%i\n",csx.lbl_cnt);
        printf("'jump':\t\t%i+%i\n",csx.jmp0c,csx.jmp1c);
        printf("'goto':\t\t%i\n",csx.gotoc);
        printf("'call':\t\t%i\n",csx.callc);
    }

    printf("Press any key to exit...");
    c=getch();
    return 0;
}
