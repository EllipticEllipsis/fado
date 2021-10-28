#pragma once

#include <stdio.h>

typedef enum {
    VERBOSITY_NONE,
    VERBOSITY_INFO,
    VERBOSITY_DEBUG //,
} VerbosityLevel;

void Fado_Relocs(FILE* outputFile, int inputFilesCount, FILE** inputFiles, const char* ovlName);
// void Fado_WriteRelocFile(FILE* outputFile, FILE** inputFiles, int inputFilesCount);
