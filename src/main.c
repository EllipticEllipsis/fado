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
#include "hestu/hestu.h"

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
#include <vc_vector/vc_vector.h>

// void Test_vc_vector() {
//     vc_vector* v = vc_vector_create(0, sizeof(int), NULL);

//     {
//         size_t i;
//         int x = 1;
//         for (i = 0; i < 10; i++) {
//             x *= i + 1;
//             vc_vector_push_back(v, &x);
//         }
//         for (i = 0; i < vc_vector_count(v); i++) {
//             int* temp = vc_vector_at(v, i);
//             printf("%d\n", *temp);
//         }
//     }

//     vc_vector_release(v);
// }

// void Test() {
//     int* array = NULL;
//     array = HESTU_INIT(array, 4);

//     HESTU_ADDITEM(array, 1);
//     HESTU_ADDITEM(array, 2);
//     HESTU_ADDITEM(array, 3);
//     HESTU_ADDITEM(array, 18);
//     HESTU_ADDITEM(array, 36);

//     {
//         size_t i;
//         for (i = 0; i < HESTU_HEADER(array)->count; i++) {
//             printf("%d, ", array[i]);
//         }
//         putchar('\n');
//     }
//     printf("Freeing array\n");
//     HESTU_DESTROY(array);
//     printf("Array freed\n");
// }

// TYPEDEF_DYNAMIC_ARRAY(int)

// void Test_Template() {
//     DYNAMIC_ARRAY(int) dynArray;

//     intArray_Init(&dynArray);

//     intArray_AddItem(&dynArray, 1);
//     intArray_AddItem(&dynArray, 2);
//     intArray_AddItem(&dynArray, 3);
//     intArray_AddItem(&dynArray, 18);
//     intArray_AddItem(&dynArray, 36);

//     {
//         size_t i;
//         for (i = 0; i < dynArray.count; i++) {
//             printf("%d, ", dynArray.array[i]);
//         }
//         putchar('\n');
//     }
//     intArray_Destroy(&dynArray);
// }

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

            case 'h':
                PrintHelp(optCount, optInfo);
                return EXIT_FAILURE;

            default:
                printf("?? getopt returned character code 0%o ??\n", opt);
                break;
        }
    }

    printf("Options processed\n");

    {
        int i;

        inputFilesCount = argc - optind;
        inputFiles = malloc(inputFilesCount * sizeof(FILE*));
        for (i = 0; i < inputFilesCount; i++) {
            printf("Using input file %s\n", argv[optind + i]);
            inputFiles[i] = fopen(argv[optind + i], "rb");
        }
        printf("Found %d input file%s\n", inputFilesCount, (inputFilesCount == 1 ? "" : "s" ) );
        

        // Fairy_PrintSymbolTable(inputFile);
        // PrintZeldaReloc(inputFile);

        char* ovlName = GetOverlayNameFromFilename(argv[optind]);
        fprintf(outputFile, "# _%sOverlayInfo\n", ovlName);
        fprintf(outputFile, ".word _%sSegmentTextSize\n", ovlName);
        fprintf(outputFile, ".word _%sSegmentDataSize\n", ovlName);
        fprintf(outputFile, ".word _%sSegmentRoDataSize\n", ovlName);
        fprintf(outputFile, ".word _%sSegmentBssSize\n", ovlName);

        Fado_Relocs(inputFiles, inputFilesCount);

        fprintf(outputFile, "%sOverlayInfoOffset\n", ovlName);

        free(ovlName);

        // Fairy_PrintRelocs(inputFile);

        // Test_vc_vector();
        // Test();
        // Test_Template();
        for (i = 0; i < inputFilesCount; i++) {
            fclose(inputFiles[i]);
        }
        free(inputFiles);
    }

    return EXIT_SUCCESS;
}
