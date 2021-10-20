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

int main(int argc, char **argv) {
    int opt;
    FILE *inputFile;

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

    // Fairy_PrintRelocs(inputFile);

    // typedef struct
    // {
    //   Elf32_Word	sh_name;		/* Section name (string tbl index) */
    //   Elf32_Word	sh_type;		/* Section type */
    //   Elf32_Word	sh_flags;		/* Section flags */
    //   Elf32_Addr	sh_addr;		/* Section virtual addr at execution */
    //   Elf32_Off	sh_offset;		/* Section file offset */
    //   Elf32_Word	sh_size;		/* Section size in bytes */
    //   Elf32_Word	sh_link;		/* Link to another section */
    //   Elf32_Word	sh_info;		/* Additional section information */
    //   Elf32_Word	sh_addralign;		/* Section alignment */
    //   Elf32_Word	sh_entsize;		/* Entry size if section holds table */
    // } Elf32_Shdr;

    fclose(inputFile);

    return EXIT_SUCCESS;
}
