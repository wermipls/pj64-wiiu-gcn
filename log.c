#include "log.h"

#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "plugin_info.h"

const char prefix_warn[] = "warning: ";
const char prefix_error[] = "error: ";
const char prefix_none[] = "";

const char logpath[] = "./Logs/" PLUGIN_NAME ".txt";
FILE *logfile;

void log_open()
{
    CreateDirectory("Logs", NULL);
    logfile = fopen(logpath, "w");
}

void log_close()
{
    fclose(logfile);
}

void dlog(enum LogLevel l, char fmt[], ...)
{
    time_t rawtime;
    time(&rawtime);

    struct tm *timeinfo = localtime(&rawtime);
    char timestr[16];
    strftime(timestr, sizeof(timestr), "%H:%M:%S", timeinfo);

    const char *prefix;

    switch (l)
    {
    case LOG_ERR:
    case LOG_ERR_NO_MSGBOX:
        prefix = prefix_error;
        break;
    case LOG_WARN:
        prefix = prefix_warn;
        break;
    default:
        prefix = prefix_none;
        break;
    }

    va_list argv;
    va_start(argv, fmt);

    char msg[1024];
    vsnprintf(msg, 1024, fmt, argv);

    va_end(argv);

    // print to file
    fprintf(logfile, "[%s] %s%s\n", timestr, prefix, msg);
    
    if(l == LOG_ERR) {
        MessageBox(NULL, msg, PLUGIN_NAME " error", MB_OK | MB_ICONERROR);
    }
}
