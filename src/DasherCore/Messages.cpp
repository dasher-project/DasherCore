#include "Messages.h"
#include <stdarg.h>
#include <stdio.h>

void CMessageDisplay::FormatMessage(const char* format, va_list args) {
    if (format == nullptr) return;

    std::string lineToPrint;

    va_list args_copy;
    va_copy(args_copy, args);
    int length = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    lineToPrint.resize(length);
    vsnprintf(&lineToPrint[0], length + 1, format, args);

    Message(lineToPrint, true);
}

void CMessageDisplay::FormatMessage(const char* format, ...) {
    va_list args;

    va_start(args, format);
    FormatMessage(format, args);
    va_end(args);
}