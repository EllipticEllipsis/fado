/**
 * Relocation reading part
 */
#include "fado.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fairy.h"


/* String-finding-related functions */

typedef struct {
    const char** list;
    unsigned int count;
} FadoStringList;

void Fado_InitStringList(FadoStringList* stringLists, FairyFileInfo* fileInfo, int numFiles) {
    int currentFile;
    int currentSym;
    int symCount;

    for (currentFile = 0; currentFile < numFiles; currentFile++) {
        FairySym* symtab = fileInfo[currentFile].symtabInfo.sectionData;
        symCount = 0;

        /* Count defined symbols */
        for (currentSym = 0; currentSym < fileInfo->symtabInfo.sectionSize / sizeof(FairySym); currentSym++) {
            if (symtab[currentSym].st_shndx != STN_UNDEF) {
                symCount++;
            }
        }

        stringLists[currentFile].list = malloc(symCount * sizeof(char*));
        stringLists[currentFile].count = symCount;

        /* Build array of pointers to defined symbols' names */
        symCount = 0;
        for (currentSym = 0; currentSym < fileInfo->symtabInfo.sectionSize / sizeof(FairySym); currentSym++) {
            if (symtab[currentSym].st_shndx != STN_UNDEF) {
                stringLists[currentFile].list[symCount] = fileInfo[currentFile].strtab[symtab[currentSym].st_name];
                symCount++;
            }
        }
    }
}

bool Fado_FindSymbolNameInOtherFiles(const char* name, int thisFile, FadoStringList* stringLists, int numFiles) {
    int currentFile;
    int currentString;

    for (currentFile; currentFile < numFiles; currentFile++) {
        if (currentFile == thisFile) {
            continue;
        }
        for (currentString = 0; currentString < stringLists[currentFile].count; currentString++) {
            if (strcmp(name, stringLists[currentFile].list[currentString]) == 0) {
                return true;
            }
        }
    }
    return false;
}

void Fado_DestroyStringList(FadoStringList* stringLists, int numFiles) {
    int currentFile;
    for (currentFile; currentFile < numFiles; currentFile++) {
        free(stringLists[currentFile].list);
    }
}

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

static const FairyDefineString relSectionNames[] = {
    FAIRY_DEF_STRING(FAIRY_SECTION, TEXT),
    FAIRY_DEF_STRING(FAIRY_SECTION, DATA),
    FAIRY_DEF_STRING(FAIRY_SECTION, RODATA),
    { 0 },
};

static const FairyDefineString relTypeNames[] = {
    FAIRY_DEF_STRING(R, MIPS_NONE),    /* No reloc */
    FAIRY_DEF_STRING(R, MIPS_16),      /* Direct 16 bit */
    FAIRY_DEF_STRING(R, MIPS_32),      /* Direct 32 bit */
    FAIRY_DEF_STRING(R, MIPS_REL32),   /* PC relative 32 bit */
    FAIRY_DEF_STRING(R, MIPS_26),      /* Direct 26 bit shifted */
    FAIRY_DEF_STRING(R, MIPS_HI16),    /* High 16 bit */
    FAIRY_DEF_STRING(R, MIPS_LO16),    /* Low 16 bit */
    FAIRY_DEF_STRING(R, MIPS_GPREL16), /* GP relative 16 bit */
    FAIRY_DEF_STRING(R, MIPS_LITERAL), /* 16 bit literal entry */
    FAIRY_DEF_STRING(R, MIPS_GOT16),   /* 16 bit GOT entry */
    FAIRY_DEF_STRING(R, MIPS_PC16),    /* PC relative 16 bit */
    FAIRY_DEF_STRING(R, MIPS_CALL16),  /* 16 bit GOT entry for function */
    FAIRY_DEF_STRING(R, MIPS_GPREL32), /* GP relative 32 bit */
    FAIRY_DEF_STRING(R, MIPS_SHIFT5),
    FAIRY_DEF_STRING(R, MIPS_SHIFT6),
    FAIRY_DEF_STRING(R, MIPS_64),
    FAIRY_DEF_STRING(R, MIPS_GOT_DISP),
    FAIRY_DEF_STRING(R, MIPS_GOT_PAGE),
    FAIRY_DEF_STRING(R, MIPS_GOT_OFST),
    FAIRY_DEF_STRING(R, MIPS_GOT_HI16),
    FAIRY_DEF_STRING(R, MIPS_GOT_LO16),
    FAIRY_DEF_STRING(R, MIPS_SUB),
    FAIRY_DEF_STRING(R, MIPS_INSERT_A),
    FAIRY_DEF_STRING(R, MIPS_INSERT_B),
    FAIRY_DEF_STRING(R, MIPS_DELETE),
    FAIRY_DEF_STRING(R, MIPS_HIGHER),
    FAIRY_DEF_STRING(R, MIPS_HIGHEST),
    FAIRY_DEF_STRING(R, MIPS_CALL_HI16),
    FAIRY_DEF_STRING(R, MIPS_CALL_LO16),
    FAIRY_DEF_STRING(R, MIPS_SCN_DISP),
    FAIRY_DEF_STRING(R, MIPS_REL16),
    FAIRY_DEF_STRING(R, MIPS_ADD_IMMEDIATE),
    FAIRY_DEF_STRING(R, MIPS_PJUMP),
    FAIRY_DEF_STRING(R, MIPS_RELGOT),
    FAIRY_DEF_STRING(R, MIPS_JALR),
    FAIRY_DEF_STRING(R, MIPS_TLS_DTPMOD32),    /* Module number 32 bit */
    FAIRY_DEF_STRING(R, MIPS_TLS_DTPREL32),    /* Module-relative offset 32 bit */
    FAIRY_DEF_STRING(R, MIPS_TLS_DTPMOD64),    /* Module number 64 bit */
    FAIRY_DEF_STRING(R, MIPS_TLS_DTPREL64),    /* Module-relative offset 64 bit */
    FAIRY_DEF_STRING(R, MIPS_TLS_GD),          /* 16 bit GOT offset for GD */
    FAIRY_DEF_STRING(R, MIPS_TLS_LDM),         /* 16 bit GOT offset for LDM */
    FAIRY_DEF_STRING(R, MIPS_TLS_DTPREL_HI16), /* Module-relative offset, high 16 bits */
    FAIRY_DEF_STRING(R, MIPS_TLS_DTPREL_LO16), /* Module-relative offset, low 16 bits */
    FAIRY_DEF_STRING(R, MIPS_TLS_GOTTPREL),    /* 16 bit GOT offset for IE */
    FAIRY_DEF_STRING(R, MIPS_TLS_TPREL32),     /* TP-relative offset, 32 bit */
    FAIRY_DEF_STRING(R, MIPS_TLS_TPREL64),     /* TP-relative offset, 64 bit */
    FAIRY_DEF_STRING(R, MIPS_TLS_TPREL_HI16),  /* TP-relative offset, high 16 bits */
    FAIRY_DEF_STRING(R, MIPS_TLS_TPREL_LO16),  /* TP-relative offset, low 16 bits */
    FAIRY_DEF_STRING(R, MIPS_GLOB_DAT),
    FAIRY_DEF_STRING(R, MIPS_COPY),
    FAIRY_DEF_STRING(R, MIPS_JUMP_SLOT),
    FAIRY_DEF_STRING(R, MIPS_NUM),
};

void Fado_Relocs(FILE* inputFile) {
    FairyFileInfo fileInfo;
    FadoRelocInfo* relocList[FAIRY_SECTION_OTHER]; // Maximum number of reloc sections
    size_t relocIndex;
    FairySection section;
    FairySym* symtab;
    size_t relocCount = 0;
    uint8_t padCount;

    Fairy_InitFile(&fileInfo, inputFile);
    symtab = fileInfo.symtabInfo.sectionData;
    // printf("symtab set\n");

    for (section = FAIRY_SECTION_TEXT; section < FAIRY_SECTION_OTHER; section++) {
        FairyRel* relSection = fileInfo.relocTablesInfo[section].sectionData;

        if (relSection == NULL) {
            // printf("Ignoring empty reloc section\n");
            continue;
        }
        relocList[section] =
            malloc(fileInfo.relocTablesInfo[section].sectionSize / sizeof(FairyRel) * sizeof(FadoRelocInfo));

        for (relocIndex = 0; relocIndex < fileInfo.relocTablesInfo[section].sectionSize / sizeof(FairyRel);
             relocIndex++) {
            FadoRelocInfo* currentReloc = &relocList[section][relocIndex];
            Fado_MakeReloc(currentReloc, section, &relSection[relocIndex]);
            if (symtab[currentReloc->symbolIndex].st_shndx != STN_UNDEF) {
                relocCount++;
            }
        }
    }

    {
        char overlayName[] = "ovl_Name_Goes_Here";
        printf(".section .ovl\n# %sOverlayInfo\n", overlayName);
        printf(".word _%sSegmentTextSize\n", overlayName);
        printf(".word _%sSegmentDataSize\n", overlayName);
        printf(".word _%sSegmentRoDataSize\n", overlayName);
        printf(".word _%sSegmentBssSize\n", overlayName);

        printf("\n.word %zd # relocCount\n", relocCount);
        padCount = -(relocCount + 2) & 3;

        for (section = FAIRY_SECTION_TEXT; section < FAIRY_SECTION_OTHER; section++) {
            FairyRel* relSection = fileInfo.relocTablesInfo[section].sectionData;

            if (relSection == NULL) {
                // printf("Ignoring empty reloc section\n");
                continue;
            }

            printf("\n# %s RELOCS\n", Fairy_StringFromDefine(relSectionNames, section));

            for (relocIndex = 0; relocIndex < fileInfo.relocTablesInfo[section].sectionSize / sizeof(FairyRel);
                 relocIndex++) {
                FadoRelocInfo* currentReloc = &relocList[section][relocIndex];

                if (symtab[currentReloc->symbolIndex].st_shndx != STN_UNDEF) {
                    printf(".word 0x%X # %-6s %-10s 0x%06X %s\n", currentReloc->relocWord,
                           relSectionNames[section].string,
                           Fairy_StringFromDefine(relTypeNames, (currentReloc->relocWord >> 0x18) & 0x3F),
                           currentReloc->relocWord & 0xFFFFFF,
                           &fileInfo.strtab[symtab[currentReloc->symbolIndex].st_name]);
                }
            }
        }
        /* print pads and section size */
        for (relocCount += 5; ((relocCount + 1) & 3) != 0; relocCount++) {
            printf(".word 0\n");
        }
        printf("\n.word 0x%08zX # %sOverlayInfoOffset\n", 4 * (relocCount + 1), overlayName);
    }

    Fairy_DestroyFile(&fileInfo);
}
