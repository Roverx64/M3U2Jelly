#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ftw.h>

void printHelp(){
    printf("Usage: ./main.o [NAVIDROME] [JELLYFIN]\n");
    return;
}

char *readLine(FILE *fl, char readPoint){
    bool read = false;
    char *buff = NULL;
    fpos_t pos;
    uint32_t chCount = 0;
    for(char ch = 0;;){
        ch = fgetc(fl);
        if((ch == EOF) && (read == false)){return NULL;}
        if(read){
            if((ch == '\n') || (ch == EOF)){break;}
            ++chCount;
            continue;
        }
        else if(ch == readPoint){read = true; fgetpos(fl,&pos);}
    }
    buff = (char*)malloc(sizeof(char)*(chCount+1));
    if(!buff){return NULL;}
    fsetpos(fl,&pos);
    //Read in buffer
    int i = 0;
    for(; i < chCount; ++i){
        buff[i] = fgetc(fl);
    }
    buff[i] = '\0';
    return buff;
}

void replace(char *str, char dst, char src){
    size_t len = strlen(str);
    for(size_t i = 0; i < len; ++i){
        if(str[i] == dst){
            str[i] = src;
        }
    }
}

void writeXmlHeader(FILE *xml, char *plName){
    fprintf(xml,"<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\n<Item>\n");
    fprintf(xml,"  <Added>01/01/2025 00:00:00</Added>\n");
    fprintf(xml,"  <LockData>false</LockData>\n  <LocalTitle>%s</LocalTitle>\n  <PlaylistItems>\n",plName);
}

//Checks for special characters and fixes them
void writePath(FILE *xml, char *path){
    size_t len = strlen(path);
    for(size_t i = 0; i < len; ++i){
        switch(path[i]){
            case '&':
            fprintf(xml,"&amp;");
            break;
            case '<':
            fprintf(xml,"&lt;");
            break;
            case '>':
            fprintf(xml,"&gt;");
            break;
            case '"':
            fprintf(xml,"&quot;");
            break;
            case '\'':
            fprintf(xml,"&apos;");
            break;
            default:
            fputc(path[i],xml);
            break;
        }
    }
}

void writeXmlEnd(FILE *xml){
    fprintf(xml,"  </PlaylistItems>\n  <Shares />\n  <PlaylistMediaType>Audio</PlaylistMediaType>\n</Item>");
    fclose(xml);
}

void writePlaylistItem(FILE *xml, char *path){
    fprintf(xml,"    <PlaylistItem>\n      <Path>");
    writePath(xml,path);
    fprintf(xml,"</Path>\n    </PlaylistItem>\n",path);
}

void parseM3u(FILE *fl, char *output, char *trim, char *new){
    char *plName = NULL;
    char *m3u = readLine(fl,'#');
    if(!m3u){return;}
    if(strcmp(m3u,"EXTM3U")){printf("Skipping invalid M3U\n"); free(m3u); return;}
    free(m3u);
    plName = readLine(fl,':');
    printf("Parsing playlist: %s\n",plName);
    //5 for '.xml' and the null character
    char *outPath = (char*)malloc((strlen(output)+strlen(plName)+5)*sizeof(char));
    outPath[0] = '\0';
    strcat(outPath,output);
    strcat(outPath,plName);
    strcat(outPath,".xml");
    replace(outPath,' ','_');
    //Setup XML structure
    FILE *xml = fopen(outPath,"w");
    if(xml == NULL){printf("Failed to open XML file: %s\n",outPath); free(plName); free(outPath); return;}
    free(outPath);
    writeXmlHeader(xml,plName);
    free(plName);
    //Read in music files
    size_t trimLen = strlen(trim);
    size_t newLen = strlen(new);
    size_t songCount = 0;
    for(;;){
        char *song = readLine(fl,',');
        if(song == NULL){break;}
        free(song);
        char *songPath = readLine(fl,'/');
        char *newPath = (char*)malloc(sizeof(char)*(strlen(songPath)+newLen));
        newPath[0] = '\0'; //Ensure a NULL character is present
        strcat(newPath,new);
        strcat(newPath,&songPath[(trimLen-1)]);
        writePlaylistItem(xml,newPath);
        free(newPath);
        free(songPath);
        ++songCount;
    }
    printf("Added %lu songs\n",songCount);
    writeXmlEnd(xml);
}

char *ftrim = NULL;
char *fnew = NULL;

int addFile(const char *fp, const struct stat *sb, int flags, struct FTW *buf){
    if(flags != FTW_F){return 0;}
    FILE *fl = fopen(fp,"r");
    if(!fl){printf("Failed to open '%s', skipping.\n",fp); return 0;}
    parseM3u(fl,"./output/",ftrim,fnew);
    fclose(fl);
}

int main(int argc, char **argv){
    //Check for required arguments
    if(argc <= 1){printHelp(); return 1;}
    printf("Converting %s -> %s\n",argv[1],argv[2]);
    ftrim = argv[1];
    fnew = argv[2];
    int i = nftw("./convert",addFile,10,0);
    return 0;
}