#pragma once

#include <getopt.h>
#include <unistd.h>

typedef struct {
    struct option longOpt;
    char* helpArg;
    char* helpMsg;
} OptInfo;

void PrintHelp(size_t optCount, const OptInfo* optInfo);
