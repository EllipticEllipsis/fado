#pragma once

#include <getopt.h>
#include <unistd.h>

typedef struct {
    struct option longOpt;
    char* helpArg;
    char* helpMsg;
} OptInfo;

void PrintHelp(const char* prologue, size_t optCount, const OptInfo* optInfo, const char* epilogue);
