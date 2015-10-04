#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

void log_print(FILE *fp, char* filename, int line, char *fmt,...)
{
    va_list         list;
    time_t t;
    char            *p, *r;
    char string_time[128];/*Correct size?? TODO*/
    int             e;

    time(&t);
    snprintf(string_time,strlen(ctime(&t)),"%s ", ctime(&t));
    fprintf(fp,"%s ", string_time);
#ifdef DBG
    fprintf(fp," [%s][line: %d] ",filename, line);
#endif
    va_start( list, fmt );
 
    for (p = fmt ; *p ; ++p) {
        if (*p != '%') {
            fputc( *p,fp );
        }
        else {
            switch (*++p) {
            case 's': {
                r = va_arg( list, char * );
                fprintf(fp,"%s", r);
                continue;
            }
            case 'd': {
                e = va_arg( list, int );
                fprintf(fp,"%d", e);
                continue;
            }
            default:
                fputc( *p, fp );
            }
        }
    }
    va_end(list);
    fputc('\n', fp);
}
