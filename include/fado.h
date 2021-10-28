#pragma once

#include <stdio.h>

typedef enum {
    VERBOSITY_NONE,
    VERBOSITY_INFO,
    VERBOSITY_DEBUG //,
} VerbosityLevel;

void Fado_Relocs(FILE* outputFile, FILE** inputFiles, int inputFilesCount);
// void Fado_WriteRelocFile(FILE* outputFile, FILE** inputFiles, int inputFilesCount);
