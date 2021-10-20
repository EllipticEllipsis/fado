/**
 * Relocation reading part
 */
#include "fado.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "fairy.h"

typedef struct {
    size_t symbolIndex;
    uint32_t relocWord;
} FadoRelocInfo;

FadoRelocInfo* Fado_MakeReloc(FadoRelocInfo* relocInfo, FairySection section, FairyRel* data) {
    uint8_t sectionPrefix = 0;

    relocInfo->symbolIndex = ELF32_R_SYM(data->r_info);

    switch (section) {
        case FAIRY_SECTION_TEXT:
            sectionPrefix = 1;
            break;

        case FAIRY_SECTION_DATA:
            sectionPrefix = 2;
            break;

        case FAIRY_SECTION_RODATA:
            sectionPrefix = 3;
            break;

        default:
            fprintf(stderr, "Warning: Relocation section is invalid.\n");
            break;
    }
    relocInfo->relocWord = (sectionPrefix << 0x1E) | (ELF32_R_TYPE(data->r_info) << 0x18) | (data->r_offset & 0xFFFFFF);

    return relocInfo;
}

void Fado_Relocs(FILE* inputFile) {
    FairyFileInfo fileInfo;
    FadoRelocInfo* relocList[FAIRY_SECTION_OTHER]; // Maximum number of reloc sections
    size_t relocIndex;
    FairySection section;
    FairySym* symtab;

    Fairy_InitFile(&fileInfo, inputFile);
    symtab = fileInfo.symtabInfo.sectionData;
    printf("symtab set\n");

    for (section = FAIRY_SECTION_TEXT; section < FAIRY_SECTION_OTHER; section++) {
        FairyRel* relSection = fileInfo.relocTablesInfo[section].sectionData;

        if (relSection == NULL) {
            printf("Ignoring empty reloc section\n");
            continue;
        }
        relocList[section] =
            malloc(fileInfo.relocTablesInfo[section].sectionSize / sizeof(FairyRel) * sizeof(FadoRelocInfo));

        for (relocIndex = 0; relocIndex < fileInfo.relocTablesInfo[section].sectionSize / sizeof(FairyRel);
             relocIndex++) {
            FadoRelocInfo* currentReloc = &relocList[section][relocIndex];

            Fado_MakeReloc(currentReloc, section, &relSection[relocIndex]);
            printf(".word 0x%X # %s\n", currentReloc->relocWord,
                   &fileInfo.strtab[symtab[currentReloc->symbolIndex].st_name]);
        }
    }

    Fairy_DestroyFile(&fileInfo);
}
