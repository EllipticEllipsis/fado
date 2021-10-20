#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

// #include <stdint.h>
// #include <string.h>
// #include <inttypes.h>

#include "macros.h"
#include "help.h"

#include "fairy.h"
#include "fado.h"

#if defined _WIN32 || defined __CYGWIN__
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif
/* (Bad) filename-parsing idea to get the overlay name from the filename. ovlName must be freed separately. */
// char* GetOverlayNameFromFilename(char* ovlName, const char* src) {
//     size_t ind;
//     size_t start = 0;
//     size_t end = 0;

//     for (ind = strlen(src); ind != 0; ind--) {
//         if (src[ind] == PATH_SEPARATOR) {
//             if (end != 0) {
//                 start = ind + 1;
//                 break;
//             }
//             end = ind;
//         }
//     }
//     if (end == 0) {
//         return NULL;
//     }

//     ovlName = malloc((end - start + 1) * sizeof(char));
//     memcpy(ovlName, src + start, end - start);
//     ovlName[end - start + 1] = '\0';

//     return ovlName;
// }

char* GetOverlayNameFromFilename(const char* src) {
    char* ret;
    const char* ptr;
    const char* start = src;
    const char* end = src;

    for (ptr = src; *ptr != '\0'; ptr++) {
        if (*ptr == PATH_SEPARATOR) {
            start = end;
            end = ptr;
        }
    }

    if (end == src) {
        return NULL;
    }

    ret = malloc((end - start) * sizeof(char));
    memcpy(ret, start + 1, end - start - 1);
    ret[end - start] = '\0';

    return ret;
}

#define OPTSTR "h"

static const OptInfo optInfo[] = {
    { { "help", no_argument, NULL, 'h' }, NULL, "Display this message and exit" },

    { { NULL, 0, NULL, 0 }, NULL, NULL },
};

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
    FILE* inputFile;

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

            case 'h':
                PrintHelp(optCount, optInfo);
                return EXIT_FAILURE;

            default:
                printf("?? getopt returned character code 0%o ??\n", opt);
                break;
        }
    }

    printf("Options processed\n");

    inputFile = fopen(argv[optind], "rb");
    printf("Using input file %s\n", argv[optind]);

    // Fairy_PrintSymbolTable(inputFile);
    // PrintZeldaReloc(inputFile);
    Fado_Relocs(inputFile);
    // {
    //     char* ovlName = GetOverlayNameFromFilename(argv[optind]);
    //     printf("%s\n", ovlName);
    //     free(ovlName);
    // }

    // Fairy_PrintRelocs(inputFile);

    fclose(inputFile);

    return EXIT_SUCCESS;
}
