#pragma once

enum LogLevel
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERR,
    LOG_ERR_NO_MSGBOX,
};

void log_open();
void log_close();
void dlog(enum LogLevel l, char fmt[], ...);
