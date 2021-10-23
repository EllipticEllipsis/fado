/**
 * Relocation reading part
 */
#include "fado.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fairy.h"
#include "vc_vector/vc_vector.h"

#define VC_FOREACH(i, v) for (i = vc_vector_begin(v); i != vc_vector_end(v); i = vc_vector_next(v, i))

/* String-finding-related functions */

bool Fado_CheckInProgBitsSections(Elf32_Section section, vc_vector* progBitsSections) {
    uint16_t* i;

    // for (i = vc_vector_begin(progBitsSections); i != vc_vector_end(progBitsSections);
    //      i = vc_vector_next(progBitsSections, i)) {
    VC_FOREACH(i, progBitsSections) {
        if (*i == section) {
            return true;
        }
    }
    return false;
}

void Fado_ConstructStringVectors(vc_vector** stringVectors, FairyFileInfo* fileInfo, int numFiles) {
    int currentFile;
    size_t currentSym;

    for (currentFile = 0; currentFile < numFiles; currentFile++) {
        FairySym* symtab = fileInfo[currentFile].symtabInfo.sectionData;

        stringVectors[currentFile] = vc_vector_create(0x10, sizeof(char*), free);

        /* Build array of pointers to defined symbols' names */
        for (currentSym = 0; currentSym < fileInfo[currentFile].symtabInfo.sectionSize / sizeof(FairySym);
             currentSym++) {
            if ((symtab[currentSym].st_shndx != STN_UNDEF) &&
                Fado_CheckInProgBitsSections(symtab[currentSym].st_shndx, fileInfo[currentFile].progBitsSections)) {
                assert(vc_vector_push_back(stringVectors[currentFile],
                                           &fileInfo[currentFile].strtab[symtab[currentSym].st_name]));
            }
        }
    }
}

bool Fado_FindSymbolNameInOtherFiles(const char* name, int thisFile, vc_vector** stringVectors, int numFiles) {
    int currentFile;
    char* currentString;

    for (currentFile = 0; currentFile < numFiles; currentFile++) {
        if (currentFile == thisFile) {
            continue;
        }

        // for (currentString = vc_vector_begin(stringVectors[currentFile]);
        //      currentString != vc_vector_end(stringVectors[currentFile]);
        //      currentString = vc_vector_next(stringVectors[currentFile], currentString)) {
        VC_FOREACH(currentString, stringVectors[currentFile]) {
            if (strcmp(name, currentString) == 0) {
                return true;
            }
        }
    }
    return false;
}

void Fado_DestroyStringVectors(vc_vector** stringVectors, int numFiles) {
    int currentFile;
    for (currentFile = 0; currentFile < numFiles; currentFile++) {
        vc_vector_release(stringVectors[currentFile]);
    }
}

typedef struct {
    size_t symbolIndex;
    uint32_t relocWord;
} FadoRelocInfo;

FadoRelocInfo* Fado_MakeReloc(FadoRelocInfo* relocInfo, FairySection section, FairyRel* data) {
    uint32_t sectionPrefix = 0;

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
            fprintf(stderr, "warning: Relocation section is invalid.\n");
            break;
    }
    relocInfo->relocWord =
        ((sectionPrefix & 3) << 0x1E) | (ELF32_R_TYPE(data->r_info) << 0x18) | (data->r_offset & 0xFFFFFF);

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

void Fado_Relocs(FILE** inputFiles, int inputFilesCount) {
    /* General information structs */
    FairyFileInfo* fileInfos = malloc(inputFilesCount * sizeof(FairyFileInfo));

    /* Symbol tables for each file */
    FairySym** symtabs = malloc(inputFilesCount * sizeof(FairySym*));

    /* Lists of names of symbols defined in files of the overlay */
    vc_vector** stringVectors = malloc(inputFilesCount * sizeof(vc_vector*));

    /* The relocs in the format we will print */
    vc_vector* relocList[FAIRY_SECTION_OTHER]; // Maximum number of reloc sections

    /* Offset of current file's current section into the overlay's whole section */
    uint32_t sectionOffset[FAIRY_SECTION_OTHER] = { 0 };

    /* Total number of relocs */
    uint32_t relocCount = 0;
    /* 0,1,2 to make the section a whole qword */
    uint8_t padCount;

    /* iterators */
    int currentFile;
    FairySection section;
    size_t relocIndex;

    // fileInfos = malloc(inputFilesCount * sizeof(FairyFileInfo));
    // symtabs = malloc(inputFilesCount * sizeof(FairySym*));
    for (currentFile = 0; currentFile < inputFilesCount; currentFile++) {
        Fairy_InitFile(&fileInfos[currentFile], inputFiles[currentFile]);
        symtabs[currentFile] = fileInfos[currentFile].symtabInfo.sectionData;
    }

    // stringVectors = malloc(inputFilesCount * sizeof(vc_vector*));
    Fado_ConstructStringVectors(stringVectors, fileInfos, inputFilesCount);
    // printf("symtabs set\n");

    /* Construct relocList of all relevant relocs */
    for (section = FAIRY_SECTION_TEXT; section < FAIRY_SECTION_OTHER; section++) {
        relocList[section] = vc_vector_create(0x20, sizeof(FadoRelocInfo), NULL);

        for (currentFile = 0; currentFile < inputFilesCount; currentFile++) {
            FairyRel* relSection = fileInfos[currentFile].relocTablesInfo[section].sectionData;
            // if (relSection == NULL) {
            //     // printf("Ignoring empty reloc section\n");
            //     relocList[section] = NULL;
            //     continue;
            // }

            // relocList[section] =
            //     malloc(fileInfos[0].relocTablesInfo[section].sectionSize / sizeof(FairyRel) * sizeof(FadoRelocInfo));

            for (relocIndex = 0;
                 relocIndex < fileInfos[currentFile].relocTablesInfo[section].sectionSize / sizeof(FairyRel);
                 relocIndex++) {
                FadoRelocInfo* currentReloc = NULL;
                Fado_MakeReloc(currentReloc, section, &relSection[relocIndex]);

                if (symtabs[currentFile][currentReloc->symbolIndex].st_shndx != STN_UNDEF) {
                    if (Fado_FindSymbolNameInOtherFiles(
                            &fileInfos[currentFile].strtab[symtabs[currentFile][currentReloc->symbolIndex].st_name],
                            currentFile, stringVectors, inputFilesCount)) {
                        continue;
                    }

                    currentReloc->relocWord += sectionOffset[section];
                    vc_vector_push_back(relocList[section], currentReloc);
                }
            }

            sectionOffset[section] += fileInfos[currentFile].progBitsSizes[section];
        }
    }

    {
        // char overlayName[] = "ovl_Name_Goes_Here";
        // printf(".section .ovl\n# %sOverlayInfo\n", overlayName);
        // printf(".word _%sSegmentTextSize\n", overlayName);
        // printf(".word _%sSegmentDataSize\n", overlayName);
        // printf(".word _%sSegmentRoDataSize\n", overlayName);
        // printf(".word _%sSegmentBssSize\n", overlayName);

        printf("\n.word %d # relocCount\n", relocCount);
        padCount = -(relocCount + 2) & 3;

        for (section = FAIRY_SECTION_TEXT; section < FAIRY_SECTION_OTHER; section++) {
            FairyRel* relSection = fileInfos[0].relocTablesInfo[section].sectionData;

            if (relSection == NULL) {
                // printf("Ignoring empty reloc section\n");
                continue;
            }

            printf("\n# %s RELOCS\n", Fairy_StringFromDefine(relSectionNames, section));

            for (relocIndex = 0; relocIndex < fileInfos[0].relocTablesInfo[section].sectionSize / sizeof(FairyRel);
                 relocIndex++) {
                FadoRelocInfo* currentReloc = vc_vector_at(relocList[section], relocIndex);

                if (symtabs[0][currentReloc->symbolIndex].st_shndx != STN_UNDEF) {
                    printf(".word 0x%X # %-6s %-10s 0x%06X %s\n", currentReloc->relocWord,
                           relSectionNames[section].string,
                           Fairy_StringFromDefine(relTypeNames, (currentReloc->relocWord >> 0x18) & 0x3F),
                           currentReloc->relocWord & 0xFFFFFF,
                           &fileInfos[0].strtab[symtabs[0][currentReloc->symbolIndex].st_name]);
                }
            }
        }
        /* print pads and section size */
        for (relocCount += 5; ((relocCount + 1) & 3) != 0; relocCount++) {
            printf(".word 0\n");
        }
        printf("\n.word 0x%08X # ", 4 * (relocCount + 1));
    }

    printf("\nFinish writing variables\n");
    for (currentFile = 0; currentFile < inputFilesCount; currentFile++) {
        printf("Freeing file %d\n", currentFile);
        Fairy_DestroyFile(&fileInfos[currentFile]);
        printf("Freed file %d\n", currentFile);
    }

    for (section = FAIRY_SECTION_TEXT; section < FAIRY_SECTION_OTHER; section++) {
        if (relocList[section] != NULL) {
            free(relocList[section]);
        }
        printf("Freed relocList[%d]\n", section);
    }

    printf("Freeing string vectors\n");
    Fado_DestroyStringVectors(stringVectors, inputFilesCount);
    printf("Freed string vectors\n");
    printf("Freeing symtabs\n");
    free(symtabs);
    printf("Freed symtabs\n");
    free(fileInfos);
}
