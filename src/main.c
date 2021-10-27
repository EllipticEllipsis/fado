#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "macros.h"
#include "help.h"
#include "fairy.h"
#include "fado.h"
#include "vc_vector/vc_vector.h"

#if defined _WIN32 || defined __CYGWIN__
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

/* (Bad) filename-parsing idea to get the overlay name from the filename. output must be freed separately. */
char* GetOverlayNameFromFilename(const char* src) {
    char* ret;
    const char* ptr;
    const char* start = src;
    const char* end = src;

    for (ptr = src; *ptr != '\0'; ptr++) {
        if (*ptr == PATH_SEPARATOR) {
            start = end + 1;
            end = ptr;
        }
    }

    if (end == src) {
        return NULL;
    }

    ret = malloc((end - start + 1) * sizeof(char));
    memcpy(ret, start, end - start);
    ret[end - start] = '\0';

    return ret;
}

#define OPTSTR "o:h"

// clang-format off
static const OptInfo optInfo[] = {
    { { "output-file", required_argument, NULL, 'o' }, "FILE", "select output file. Will use stdout if none is specified" },

    { { "help", no_argument, NULL, 'h' }, NULL, "Display this message and exit" },

    { { NULL, 0, NULL, 0 }, NULL, NULL },
};
// clang-format on

static size_t optCount = ARRAY_COUNT(optInfo);
static struct option longOptions[ARRAY_COUNT(optInfo)];

void ConstructLongOpts() {
    size_t i;

    for (i = 0; i < optCount; i++) {
        longOptions[i] = optInfo[i].longOpt;
    }
}

int main(int argc, char** argv) {
    int opt;
    int inputFilesCount;
    FILE** inputFiles;
    FILE* outputFile = stdout;

    ConstructLongOpts();

    if (argc < 2) {
        fprintf(stderr, "No input file specified\n");
        return EXIT_FAILURE;
    }

    while (true) {
        int optionIndex = 0;

        if ((opt = getopt_long(argc, argv, OPTSTR, longOptions, &optionIndex)) == -1) {
            break;
        }

        switch (opt) {
            case 'o':
                outputFile = fopen(optarg, "wb");
                break;

            case 'h':
                PrintHelp(optCount, optInfo);
                return EXIT_FAILURE;

            default:
                printf("?? getopt returned character code 0%o ??\n", opt);
                break;
        }
    }

    // printf("Options processed\n");

    {
        int i;

        inputFilesCount = argc - optind;
        inputFiles = malloc(inputFilesCount * sizeof(FILE*));
        for (i = 0; i < inputFilesCount; i++) {
            // printf("Using input file %s\n", argv[optind + i]);
            inputFiles[i] = fopen(argv[optind + i], "rb");
        }
        // printf("Found %d input file%s\n", inputFilesCount, (inputFilesCount == 1 ? "" : "s" ) );

        char* ovlName = GetOverlayNameFromFilename(argv[optind]);
        fprintf(outputFile, ".section .ovl\n");
        fprintf(outputFile, "# %sOverlayInfo\n", ovlName);
        fprintf(outputFile, ".word _%sSegmentTextSize\n", ovlName);
        fprintf(outputFile, ".word _%sSegmentDataSize\n", ovlName);
        fprintf(outputFile, ".word _%sSegmentRoDataSize\n", ovlName);
        fprintf(outputFile, ".word _%sSegmentBssSize\n", ovlName);

        Fado_Relocs(outputFile, inputFiles, inputFilesCount);

        fprintf(outputFile, "%sOverlayInfoOffset\n", ovlName);

        free(ovlName);

        for (i = 0; i < inputFilesCount; i++) {
            fclose(inputFiles[i]);
        }
        free(inputFiles);
        if (outputFile != stdout) {
            fclose(outputFile);
        }
    }

    return EXIT_SUCCESS;
}
