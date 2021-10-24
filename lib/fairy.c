#include "fairy.h"

#include <assert.h>
#include <endian.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vc_vector/vc_vector.h"

#include "fairy_data.c"

// #if __BYTE_ORDER == __LITTLE_ENDIAN
// #define MAGIC 'FLE\x7F'
// #else
// #define MAGIC '\x7FELF'
// #endif

/* Endian readers. MIPS is BE, so only need these */
static Elf32_Half Fairy_ReadHalf(const uint8_t* data) {
    return data[0] << 8 | data[1] << 0;
}

static Elf32_Word Fairy_ReadWord(const uint8_t* data) {
    return data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3] << 0;
}

static bool Fairy_VerifyMagic(const uint8_t* data) {
    return (data[0] == 0x7F && data[1] == 'E' && data[2] == 'L' && data[3] == 'F');
}

static uint16_t Fairy_Swap16(uint16_t x) {
    return ((x & 0xFF) << 0x8) | ((x & 0xFF00) >> 0x8);
}

static uint32_t Fairy_Swap32(uint32_t x) {
    return ((x & 0xFF) << 0x18) | ((x & 0xFF00) << 0x8) | ((x & 0xFF0000) >> 0x8) | ((x & 0xFF000000) >> 0x18);
}

/* "Re-encode/Re-endianise", i.e. byteswap if required */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define REEND16(x) Fairy_Swap16(x)
#define REEND32(x) Fairy_Swap32(x)
#else
#define REEND16(x) (x)
#define REEND32(x) (x)
#endif

const char* Fairy_StringFromDefine(const FairyDefineString* dict, int define) {
    size_t i;
    for (i = 0; dict[i].string != NULL; i++) {
        if (dict[i].define == define) {
            return dict[i].string;
        }
    }
    return NULL;
}

// This *should* work, but needs to be tested
bool Fairy_StartsWith(const char* string, const char* initial) {
    char s;
    char i;
    do {
        s = *string++;
        i = *initial++;
        if (i == '\0') {
            return true;
        }
    } while (s == i);
    return false;
}

/* Reading functions */

/**
 * Every reading function:
 * - Returns the pointer to the struct
 * - Takes the ouput struct or array as its first argument. This must be pre-allocated
 * - Takes the input file as the second argument (At least until I am persuaded to read the whole file into RAM...)
 * - The rest of the arguments are important information about the struct it is reading (offset and size, usually)
 */

FairyFileHeader* Fairy_ReadFileHeader(FairyFileHeader* header, FILE* file) {
    fseek(file, 0, SEEK_SET);
    fread(header, 0x34, 1, file);

    if (!Fairy_VerifyMagic(header->e_ident)) {
        fprintf(stderr, "Not a valid ELF file\n");
        return NULL;
    }

    if (header->e_ident[EI_CLASS] != ELFCLASS32) {
        fprintf(stderr, "Not a 32-bit ELF file\n");
        return NULL;
    }

    header->e_type = REEND16(header->e_type);
    header->e_machine = REEND16(header->e_machine);
    header->e_version = REEND32(header->e_version);
    header->e_entry = REEND32(header->e_entry);
    header->e_phoff = REEND32(header->e_phoff);
    header->e_shoff = REEND32(header->e_shoff);
    header->e_flags = REEND32(header->e_flags);
    header->e_ehsize = REEND16(header->e_ehsize);
    header->e_phentsize = REEND16(header->e_phentsize);
    header->e_phnum = REEND16(header->e_phnum);
    header->e_shentsize = REEND16(header->e_shentsize);
    header->e_shnum = REEND16(header->e_shnum);
    header->e_shstrndx = REEND16(header->e_shstrndx);

    return header;
}

/* tableOffset and number should be obtained from the file header */
FairySecHeader* Fairy_ReadSectionTable(FairySecHeader* sectionTable, FILE* file, size_t tableOffset, size_t number) {
    size_t entrySize = sizeof(FairySecHeader);
    size_t tableSize = number * entrySize;

    fseek(file, tableOffset, SEEK_SET);
    fread(sectionTable, tableSize, 1, file);

    /* Since the section table happens to only have entries of width 4, we can byteswap it by pretending it is a raw
     * uint32_t array */
    {
        size_t i;
        uint32_t* data = (uint32_t*)sectionTable;
        for (i = 0; i < tableSize / sizeof(uint32_t); i++) {
            data[i] = REEND32(data[i]);
        }
    }

    return sectionTable;
}

FairySym* Fairy_ReadSymbolTable(FairySym* symbolTable, FILE* file, size_t tableOffset, size_t tableSize) {
    size_t number = tableSize / sizeof(FairySym);

    fseek(file, tableOffset, SEEK_SET);
    fread(symbolTable, tableSize, 1, file);

    /* Reend the variables that are larger than bytes */
    {
        size_t i;
        for (i = 0; i < number; i++) {
            symbolTable[i].st_name = REEND32(symbolTable[i].st_name);
            symbolTable[i].st_value = REEND32(symbolTable[i].st_value);
            symbolTable[i].st_size = REEND32(symbolTable[i].st_size);
            symbolTable[i].st_shndx = REEND16(symbolTable[i].st_shndx);
        }
    }

    return symbolTable;
}

/* Can be used for both the section header string table and the strtab */
char* Fairy_ReadStringTable(char* stringTable, FILE* file, size_t tableOffset, size_t tableSize) {
    fseek(file, tableOffset, SEEK_SET);
    fread(stringTable, tableSize, 1, file);
    return stringTable;
}

/* offset and number are attained from the section table */
FairyRel* Fairy_ReadRelocs(FairyRel* relocTable, FILE* file, size_t offset, size_t size) {
    fseek(file, offset, SEEK_SET);
    fread(relocTable, size, 1, file);

    /* Reend the variables that are larger than bytes */
    {
        size_t i;
        uint32_t* data = (uint32_t*)relocTable;
        for (i = 0; i < size / sizeof(uint32_t); i++) {
            data[i] = REEND32(data[i]);
        }
    }

    return relocTable;
}

char* Fairy_GetSectionName(FairySecHeader* sectionTable, char* shstrtab, size_t index) {
    return &shstrtab[sectionTable[index].sh_name];
}

/* Look up the index in the symbol table and return a pointer to the beginning of its string */
char* Fairy_GetSymbolName(FairySym* symtab, char* strtab, size_t index) {
    return &strtab[symtab[index].st_name];
}

/* FairyFileInfo functions */

void Fairy_InitFile(FairyFileInfo* fileInfo, FILE* file) {
    FairyFileHeader fileHeader;
    FairySecHeader* sectionTable;
    char* shstrtab;
    int i;
    
    assert(fileInfo != NULL);
    assert(file != NULL);

    fileInfo->progBitsSections = vc_vector_create(3, sizeof(Elf32_Section), NULL);
    for (i = 0; i < 3; i++) {
        fileInfo->progBitsSizes[i] = 0;
    }
    Fairy_ReadFileHeader(&fileHeader, file);

    sectionTable = malloc(fileHeader.e_shnum * fileHeader.e_shentsize);
    Fairy_ReadSectionTable(sectionTable, file, fileHeader.e_shoff, fileHeader.e_shnum);

    shstrtab = malloc(sectionTable[fileHeader.e_shstrndx].sh_size * sizeof(char));
    fseek(file, sectionTable[fileHeader.e_shstrndx].sh_offset, SEEK_SET);
    fread(shstrtab, sectionTable[fileHeader.e_shstrndx].sh_size, 1, file);

    /* Search for the sections we need */
    {
        size_t currentIndex;
        FairySecHeader currentSection;
        for (currentIndex = 0; currentIndex < 3; currentIndex++) {
            fileInfo->relocTablesInfo[currentIndex].sectionData = NULL;
        }

        for (currentIndex = 0; currentIndex < fileHeader.e_shnum; currentIndex++) {
            currentSection = sectionTable[currentIndex];

            switch (currentSection.sh_type) {
                case SHT_PROGBITS:
                    assert(vc_vector_push_back(fileInfo->progBitsSections, &currentIndex));
                    if (strcmp(&shstrtab[currentSection.sh_name + 1], "text") == 0) {
                        fileInfo->progBitsSizes[FAIRY_SECTION_TEXT] += currentSection.sh_size;
                        printf("text section size: 0x%X\n", fileInfo->progBitsSizes[FAIRY_SECTION_TEXT]);
                    } else if (strcmp(&shstrtab[currentSection.sh_name + 1], "data") == 0) {
                        fileInfo->progBitsSizes[FAIRY_SECTION_DATA] += currentSection.sh_size;
                        printf("data section size: 0x%X\n", fileInfo->progBitsSizes[FAIRY_SECTION_DATA]);
                    } else if (Fairy_StartsWith(&shstrtab[currentSection.sh_name + 1], "rodata")) { // May be several
                        fileInfo->progBitsSizes[FAIRY_SECTION_RODATA] += currentSection.sh_size;
                        printf("rodata section size: 0x%X\n", fileInfo->progBitsSizes[FAIRY_SECTION_RODATA]);
                    }
                    break;

                case SHT_SYMTAB:
                    if (strcmp(&shstrtab[currentSection.sh_name + 1], "symtab") == 0) {
                        fileInfo->symtabInfo.sectionSize = currentSection.sh_size;
                        fileInfo->symtabInfo.sectionData = malloc(currentSection.sh_size);
                        Fairy_ReadSymbolTable(fileInfo->symtabInfo.sectionData, file, currentSection.sh_offset,
                                              currentSection.sh_size);
                    }
                    break;

                case SHT_STRTAB:
                    if (strcmp(&shstrtab[currentSection.sh_name + 1], "strtab") == 0) {
                        // printf("strtab found\n");
                        fileInfo->strtab = malloc(currentSection.sh_size);
                        Fairy_ReadStringTable(fileInfo->strtab, file, currentSection.sh_offset, currentSection.sh_size);
                    }
                    break;

                case SHT_REL:
                    /* This assumes only one reloc section of each name */
                    // TODO: is this a problem?
                    {
                        FairySection relocSection = FAIRY_SECTION_OTHER;

                        if (strcmp(&shstrtab[currentSection.sh_name + 5], "text") == 0) {
                            relocSection = FAIRY_SECTION_TEXT;
                            printf("Found rel.text section\n");
                        } else if (strcmp(&shstrtab[currentSection.sh_name + 5], "data") == 0) {
                            relocSection = FAIRY_SECTION_DATA;
                            printf("Found rel.data section\n");
                        } else if (strcmp(&shstrtab[currentSection.sh_name + 5], "rodata") == 0) {
                            relocSection = FAIRY_SECTION_RODATA;
                            printf("Found rel.rodata section\n");
                        } else {
                            break;
                        }

                        fileInfo->relocTablesInfo[relocSection].sectionSize = currentSection.sh_size;
                        fileInfo->relocTablesInfo[relocSection].sectionData = malloc(currentSection.sh_size);
                        Fairy_ReadRelocs(fileInfo->relocTablesInfo[relocSection].sectionData, file,
                                         currentSection.sh_offset, currentSection.sh_size);
                    }
                    break;

                default:
                    break;
            }
        }
    }

    free(sectionTable);
    free(shstrtab);
    // printf("Init done.\n");
}

void Fairy_DestroyFile(FairyFileInfo* fileInfo) {
    size_t i;
    for (i = 0; i < 3; i++) {
        if (fileInfo->relocTablesInfo[i].sectionData != NULL) {
            // printf("Freeing reloc section %zd data\n", i);
            free(fileInfo->relocTablesInfo[i].sectionData);
        }
    }

    vc_vector_release(fileInfo->progBitsSections);

    // printf("Freeing symtab data\n");
    free(fileInfo->symtabInfo.sectionData);

    // printf("Freeing strtab data\n");
    free(fileInfo->strtab);
}

//=============================

void Fairy_PrintSymbolTable(FILE* inputFile) {
    FairyFileHeader fileHeader;
    FairySecHeader* sectionTable;
    size_t shstrndx;
    char* shstrtab;
    FairySym* symbolTable = NULL;
    size_t symbolTableNum = 0;
    char* strtab = NULL;

    Fairy_ReadFileHeader(&fileHeader, inputFile);
    sectionTable = malloc(fileHeader.e_shentsize * fileHeader.e_shnum);
    shstrndx = fileHeader.e_shstrndx;

    Fairy_ReadSectionTable(sectionTable, inputFile, fileHeader.e_shoff, fileHeader.e_shnum);

    shstrtab = malloc(sectionTable[shstrndx].sh_size * sizeof(char));

    fseek(inputFile, sectionTable[shstrndx].sh_offset, SEEK_SET);
    fread(shstrtab, sectionTable[shstrndx].sh_size, 1, inputFile);

    {
        size_t currentIndex;
        size_t strtabndx = 0;
        for (currentIndex = 0; currentIndex < fileHeader.e_shnum; currentIndex++) {
            FairySecHeader currentHeader = sectionTable[currentIndex];

            switch (currentHeader.sh_type) {
                case SHT_SYMTAB:
                    if (strcmp(&shstrtab[currentHeader.sh_name], ".symtab") == 0) {
                        printf("symtab found\n");
                        symbolTableNum = currentHeader.sh_size / sizeof(FairySym);
                        symbolTable = malloc(currentHeader.sh_size);
                        Fairy_ReadSymbolTable(symbolTable, inputFile, currentHeader.sh_offset, currentHeader.sh_size);
                    }
                    break;

                case SHT_STRTAB:
                    if (strcmp(&shstrtab[currentHeader.sh_name], ".strtab") == 0) {
                        strtabndx = currentIndex;
                    }
                    break;

                default:
                    break;
            }
        }

        if (symbolTable == NULL) {
            puts("No symtab found.");
            free(sectionTable);
            return;
        }

        if (strtabndx != 0) {
            printf("strtab found\n");
            printf("Size: %X bytes\n", sectionTable[strtabndx].sh_size);
            strtab = malloc(sectionTable[strtabndx].sh_size);
            printf("and mallocked\n");
            fseek(inputFile, sectionTable[strtabndx].sh_offset, SEEK_SET);
            printf("file offset sought: %X\n", sectionTable[strtabndx].sh_offset);
            fread(strtab, sectionTable[strtabndx].sh_size, 1, inputFile);
            printf("file read\n");
        }
    }

    {
        size_t currentIndex;
        printf("Symbol table\n");
        printf(" Num: Value    Size Type        Bind       Vis         Ndx  Name\n");
        for (currentIndex = 0; currentIndex < symbolTableNum; currentIndex++) {
            FairySym currentSymbol = symbolTable[currentIndex];
            printf("%4zd: ", currentIndex);
            printf("%08X ", currentSymbol.st_value);
            printf("%4X ", currentSymbol.st_size);
            printf("%-11s ", Fairy_StringFromDefine(stTypes, ELF32_ST_TYPE(currentSymbol.st_info)));
            printf("%-10s ", Fairy_StringFromDefine(stBinds, ELF32_ST_BIND(currentSymbol.st_info)));
            printf("%-11s ", Fairy_StringFromDefine(stVisibilities, ELF32_ST_VISIBILITY(currentSymbol.st_other)));

            if (currentSymbol.st_shndx != 0) {
                printf("%3X ", currentSymbol.st_shndx);
            } else {
                printf("UND ");
            }

            if (strtab != NULL) {
                printf("%s", &strtab[currentSymbol.st_name]);
            } else {
                printf("%4X ", currentSymbol.st_name);
            }
            putchar('\n');
        }
    }

    free(sectionTable);
    free(symbolTable);
    if (strtab != NULL) {
        free(strtab);
    }
}

// typedef struct {
//     int define;
//     const char *string;
// } FairyRelocType;

// #define FAIRY_RELOC_TYPE(x) \
//     { x, #x }

// static const FairyRelocType relocTypes[] = {
//     FAIRY_RELOC_TYPE(R_MIPS_NONE),
// };

void Fairy_PrintRelocs(FILE* inputFile) {
    FairyFileHeader fileHeader;
    FairySecHeader* sectionTable;
    FairyRel* relocs;
    size_t shstrndx;
    char* shstrtab;
    size_t currentSection;

    Fairy_ReadFileHeader(&fileHeader, inputFile);
    sectionTable = malloc(fileHeader.e_shentsize * fileHeader.e_shnum);
    shstrndx = fileHeader.e_shstrndx;

    Fairy_ReadSectionTable(sectionTable, inputFile, fileHeader.e_shoff, fileHeader.e_shnum);

    shstrtab = malloc(sectionTable[shstrndx].sh_size * sizeof(char));

    fseek(inputFile, sectionTable[shstrndx].sh_offset, SEEK_SET);
    fread(shstrtab, sectionTable[shstrndx].sh_size, 1, inputFile);

    for (currentSection = 0; currentSection < fileHeader.e_shnum; currentSection++) {
        if (sectionTable[currentSection].sh_type != SHT_REL) {
            continue;
        }
        printf("Section size: %d\n", sectionTable[currentSection].sh_size);

        relocs = malloc(sectionTable[currentSection].sh_size * sizeof(char));

        Fairy_ReadRelocs(relocs, inputFile, sectionTable[currentSection].sh_offset,
                         sectionTable[currentSection].sh_size);

        // fseek(inputFile, sectionTable[currentSection].sh_offset, SEEK_SET);
        // fread(relocs, sectionTable[currentSection].sh_size, 1, inputFile);

        printf("Relocs in section [%2zd]: %s:\n", currentSection, shstrtab + sectionTable[currentSection].sh_name);
        printf("Offset   Info     Type     Symbol\n");
        {
            size_t currentReloc;
            for (currentReloc = 0; currentReloc < sectionTable[currentSection].sh_size / sizeof(*relocs);
                 currentReloc++) {

                printf("%08X,%08X ", relocs[currentReloc].r_offset, relocs[currentReloc].r_info);

                switch (ELF32_R_TYPE(relocs[currentReloc].r_info)) {
                    case R_MIPS_NONE:
                        printf("%-15s", "R_MIPS_NONE");
                        break;
                    case R_MIPS_16:
                        printf("%-15s", "R_MIPS_16");
                        break;
                    case R_MIPS_32:
                        printf("%-15s", "R_MIPS_32");
                        break;
                    case R_MIPS_REL32:
                        printf("%-15s", "R_MIPS_REL32");
                        break;
                    case R_MIPS_26:
                        printf("%-15s", "R_MIPS_26");
                        break;
                    case R_MIPS_HI16:
                        printf("%-15s", "R_MIPS_HI16");
                        break;
                    case R_MIPS_LO16:
                        printf("%-15s", "R_MIPS_LO16");
                        break;
                    default:
                        break;
                }

                printf("%X", ELF32_R_SYM(relocs[currentReloc].r_info));

                putchar('\n');
            }
            putchar('\n');
        }
        putchar('\n');

        free(relocs);
    }
    free(sectionTable);
    free(shstrtab);
}

void Fairy_PrintSectionTable(FILE* inputFile) {
    FairyFileHeader fileHeader;
    FairySecHeader* sectionTable;
    size_t shstrndx;
    char* shstrtab;
    size_t currentSection;

    Fairy_ReadFileHeader(&fileHeader, inputFile);
    sectionTable = malloc(fileHeader.e_shentsize * fileHeader.e_shnum);
    shstrndx = fileHeader.e_shstrndx;

    Fairy_ReadSectionTable(sectionTable, inputFile, fileHeader.e_shoff, fileHeader.e_shnum);

    shstrtab = malloc(sectionTable[shstrndx].sh_size * sizeof(char));

    fseek(inputFile, sectionTable[shstrndx].sh_offset, SEEK_SET);
    fread(shstrtab, sectionTable[shstrndx].sh_size, 1, inputFile);

    printf("[Nr] Name           Type           Addr     Off    Size   ES Flg Lk Inf Al\n");
    for (currentSection = 0; currentSection < fileHeader.e_shnum; currentSection++) {
        FairySecHeader entry = sectionTable[currentSection];
        printf("[%2zd] ", currentSection);
        printf("%-15s", shstrtab + entry.sh_name);

        /* TODO: understand this switch better: there are several cases with the same number */
        switch (entry.sh_type) {
            case SHT_NULL:
                printf("%-15s", "NULL");
                break;
            case SHT_PROGBITS:
                printf("%-15s", "PROGBITS");
                break;
            case SHT_SYMTAB:
                printf("%-15s", "SYMTAB");
                break;
            case SHT_STRTAB:
                printf("%-15s", "STRTAB");
                break;
            case SHT_RELA:
                printf("%-15s", "RELA");
                break;
            case SHT_HASH:
                printf("%-15s", "HASH");
                break;
            case SHT_DYNAMIC:
                printf("%-15s", "DYNAMIC");
                break;
            case SHT_NOTE:
                printf("%-15s", "NOTE");
                break;
            case SHT_NOBITS:
                printf("%-15s", "NOBITS");
                break;
            case SHT_REL:
                printf("%-15s", "REL");
                break;
            case SHT_SHLIB:
                printf("%-15s", "SHLIB");
                break;
            case SHT_DYNSYM:
                printf("%-15s", "DYNSYM");
                break;
            case SHT_INIT_ARRAY:
                printf("%-15s", "INIT_ARRAY");
                break;
            case SHT_FINI_ARRAY:
                printf("%-15s", "FINI_ARRAY");
                break;
            case SHT_PREINIT_ARRAY:
                printf("%-15s", "PREINIT_ARRAY");
                break;
            case SHT_GROUP:
                printf("%-15s", "GROUP");
                break;
            case SHT_SYMTAB_SHNDX:
                printf("%-15s", "SYMTAB_SHNDX");
                break;
            case SHT_NUM:
                printf("%-15s", "NUM");
                break;
            case SHT_LOOS:
                printf("%-15s", "LOOS");
                break;
            case SHT_GNU_ATTRIBUTES:
                printf("%-15s", "GNU_ATTRIBUTES");
                break;
            case SHT_GNU_HASH:
                printf("%-15s", "GNU_HASH");
                break;
            case SHT_GNU_LIBLIST:
                printf("%-15s", "GNU_LIBLIST");
                break;
            case SHT_CHECKSUM:
                printf("%-15s", "CHECKSUM");
                break;
            case SHT_LOSUNW:
                printf("%-15s", "LOSUNW");
                break;
            // case SHT_SUNW_move:
            //     printf("%-15s", "SUNW_move");
            //     break;
            case SHT_SUNW_COMDAT:
                printf("%-15s", "SUNW_COMDAT");
                break;
            case SHT_SUNW_syminfo:
                printf("%-15s", "SUNW_syminfo");
                break;
            case SHT_GNU_verdef:
                printf("%-15s", "GNU_verdef");
                break;
            case SHT_GNU_verneed:
                printf("%-15s", "GNU_verneed");
                break;
            // case SHT_GNU_versym:
            //     printf("%-15s", "GNU_versym");
            //     break;
            // case SHT_HISUNW:
            //     printf("%-15s", "HISUNW");
            //     break;
            case SHT_HIOS:
                printf("%-15s", "HIOS");
                break;
            // case SHT_LOPROC:
            //     printf("%-15s", "LOPROC");
            //     break;
            case SHT_HIPROC:
                printf("%-15s", "HIPROC");
                break;
            case SHT_LOUSER:
                printf("%-15s", "LOUSER");
                break;
            case SHT_HIUSER:
                printf("%-15s", "HIUSER");
                break;

            case SHT_MIPS_LIBLIST:
                printf("%-15s", "MIPS_LIBLIST");
                break;
            case SHT_MIPS_MSYM:
                printf("%-15s", "MIPS_MSYM");
                break;
            case SHT_MIPS_CONFLICT:
                printf("%-15s", "MIPS_CONFLICT");
                break;
            case SHT_MIPS_GPTAB:
                printf("%-15s", "MIPS_GPTAB");
                break;
            case SHT_MIPS_UCODE:
                printf("%-15s", "MIPS_UCODE");
                break;
            case SHT_MIPS_DEBUG:
                printf("%-15s", "MIPS_DEBUG");
                break;
            case SHT_MIPS_REGINFO:
                printf("%-15s", "MIPS_REGINFO");
                break;
            case SHT_MIPS_PACKAGE:
                printf("%-15s", "MIPS_PACKAGE");
                break;
            case SHT_MIPS_PACKSYM:
                printf("%-15s", "MIPS_PACKSYM");
                break;
            case SHT_MIPS_RELD:
                printf("%-15s", "MIPS_RELD");
                break;
            case SHT_MIPS_IFACE:
                printf("%-15s", "MIPS_IFACE");
                break;
            case SHT_MIPS_CONTENT:
                printf("%-15s", "MIPS_CONTENT");
                break;
            case SHT_MIPS_OPTIONS:
                printf("%-15s", "MIPS_OPTIONS");
                break;
            case SHT_MIPS_SHDR:
                printf("%-15s", "MIPS_SHDR");
                break;
            case SHT_MIPS_FDESC:
                printf("%-15s", "MIPS_FDESC");
                break;
            case SHT_MIPS_EXTSYM:
                printf("%-15s", "MIPS_EXTSYM");
                break;
            case SHT_MIPS_DENSE:
                printf("%-15s", "MIPS_DENSE");
                break;
            case SHT_MIPS_PDESC:
                printf("%-15s", "MIPS_PDESC");
                break;
            case SHT_MIPS_LOCSYM:
                printf("%-15s", "MIPS_LOCSYM");
                break;
            case SHT_MIPS_AUXSYM:
                printf("%-15s", "MIPS_AUXSYM");
                break;
            case SHT_MIPS_OPTSYM:
                printf("%-15s", "MIPS_OPTSYM");
                break;
            case SHT_MIPS_LOCSTR:
                printf("%-15s", "MIPS_LOCSTR");
                break;
            case SHT_MIPS_LINE:
                printf("%-15s", "MIPS_LINE");
                break;
            case SHT_MIPS_RFDESC:
                printf("%-15s", "MIPS_RFDESC");
                break;
            case SHT_MIPS_DELTASYM:
                printf("%-15s", "MIPS_DELTASYM");
                break;
            case SHT_MIPS_DELTAINST:
                printf("%-15s", "MIPS_DELTAINST");
                break;
            case SHT_MIPS_DELTACLASS:
                printf("%-15s", "MIPS_DELTACLASS");
                break;
            case SHT_MIPS_DWARF:
                printf("%-15s", "MIPS_DWARF");
                break;
            case SHT_MIPS_DELTADECL:
                printf("%-15s", "MIPS_DELTADECL");
                break;
            case SHT_MIPS_SYMBOL_LIB:
                printf("%-15s", "MIPS_SYMBOL_LIB");
                break;
            case SHT_MIPS_EVENTS:
                printf("%-15s", "MIPS_EVENTS");
                break;
            case SHT_MIPS_TRANSLATE:
                printf("%-15s", "MIPS_TRANSLATE");
                break;
            case SHT_MIPS_PIXIE:
                printf("%-15s", "MIPS_PIXIE");
                break;
            case SHT_MIPS_XLATE:
                printf("%-15s", "MIPS_XLATE");
                break;
            case SHT_MIPS_XLATE_DEBUG:
                printf("%-15s", "MIPS_XLATE_DEBUG");
                break;
            case SHT_MIPS_WHIRL:
                printf("%-15s", "MIPS_WHIRL");
                break;
            case SHT_MIPS_EH_REGION:
                printf("%-15s", "MIPS_EH_REGION");
                break;
            case SHT_MIPS_XLATE_OLD:
                printf("%-15s", "MIPS_XLATE_OLD");
                break;
            case SHT_MIPS_PDR_EXCEPTION:
                printf("%-15s", "MIPS_PDR_EXCEPTION");
                break;
            case SHT_MIPS_XHASH:
                printf("%-15s", "MIPS_XHASH");
                break;
            default:
                break;
        }

        // printf("%08X ", entry.sh_type);
        printf("%08X ", entry.sh_addr);
        printf("%06X ", entry.sh_offset);
        printf("%06X ", entry.sh_size);
        printf("%02X ", entry.sh_entsize);
        // printf("%08X ", entry.sh_flags);

        {
            char flagChars[] = { 'W', 'A', 'X', 'M', 'S', 'I', 'L', 'O', 'G', 'T', 'C', 'x', 'o', 'E', 'p' };
            uint32_t flags = entry.sh_flags;
            size_t shift;
            int pad = 4;
            for (shift = 0; shift < sizeof(flagChars); shift++) {
                if ((flags >> shift) & 1) {
                    putchar(flagChars[shift]);
                    pad--;
                }
            }
            if (pad > 0) {
                printf("%*s", pad, "");
            }
        }

        printf("%2X ", entry.sh_link);
        printf("%3X ", entry.sh_info);
        printf("%2X", entry.sh_addralign);
        putchar('\n');
    }
}

/**
 * Returns true if the string 'initial' is contained in the string 'string'
 * 'initial' must be null-terminated, 'string' ideally is.
 */
bool Fairy_StringHasInitialChars(const char* string, const char* initial) {
    int i;
    printf("%s", "Fairy_StringHasInitialChars");
    printf(": \"%s\" vs \"%s\"", string, initial);
    for (i = 0; initial[i] != '\0'; i++) {
        if (initial[i] != string[i]) {
            printf(": %d\n", i);
            return false;
        }
    }
    printf(": %d\n", i);
    return true;
}

typedef enum {
    REL_SECTION_NONE,
    REL_SECTION_TEXT,
    REL_SECTION_DATA,
    REL_SECTION_RODATA
    // ,REL_SECTION_BSS
} FairyOverlayRelSection;

const char* relSectionStrings[] = {
    NULL,
    ".text",
    ".data",
    ".rodata",
};

uint32_t Fairy_PackReloc(FairyOverlayRelSection sec, FairyRel rel) {
    return (sec << 0x1E) | (ELF32_R_TYPE(rel.r_info) << 0x18) | rel.r_offset;
}

void Fairy_PrintSectionSizes(FairySecHeader* sectionTable, FILE* inputFile, size_t tableSize, char* shstrtab) {
    size_t number = tableSize / sizeof(FairySecHeader);
    FairySecHeader currentHeader;
    char* sectionName;
    size_t relocSectionsCount = 0;
    size_t* relocSectionIndices;
    int* relocSectionSection;
    size_t currentRelocSection = 0;
    FairySecHeader symtabHeader;
    FairySym* symtab;
    FairySecHeader strtabHeader;
    char* strtab;
    // size_t symtabSize;

    uint32_t textSize = 0;
    uint32_t dataSize = 0;
    uint32_t rodataSize = 0;
    uint32_t bssSize = 0;
    uint32_t relocCount = 0;

    size_t currentSection;
    bool symtabFound = false;
    bool strtabFound = false;
    /* Count the reloc sections */
    for (currentSection = 0; currentSection < number; currentSection++) {
        if (sectionTable[currentSection].sh_type == SHT_REL) {
            relocSectionsCount++;
        }
    }
    printf("relocSectionsCount: %zd\n", relocSectionsCount);

    relocSectionIndices = malloc(relocSectionsCount * sizeof(int));
    relocSectionSection = malloc(relocSectionsCount * sizeof(int));

    /* Find the section sizes and the reloc sections */
    for (currentSection = 0; currentSection < number; currentSection++) {
        currentHeader = sectionTable[currentSection];
        sectionName = &shstrtab[currentHeader.sh_name + 1]; /* ignore the initial '.' */
        switch (currentHeader.sh_type) {
            case SHT_PROGBITS:
                if (Fairy_StringHasInitialChars(sectionName, "rodata")) {
                    printf("rodata\n");
                    rodataSize += currentHeader.sh_size;
                    break;
                }
                if (Fairy_StringHasInitialChars(sectionName, "data")) {
                    printf("data\n");
                    dataSize += currentHeader.sh_size;
                    break;
                }
                if (Fairy_StringHasInitialChars(sectionName, "text")) {
                    printf("text\n");
                    textSize += currentHeader.sh_size;
                    break;
                }
                break;

            case SHT_NOBITS:
                if (Fairy_StringHasInitialChars(sectionName, "bss")) {
                    printf("bss\n");
                    bssSize += currentHeader.sh_size;
                }
                break;

            case SHT_REL:
                relocSectionIndices[currentRelocSection] = currentSection;
                sectionName += 4; /* ignore the "rel." part */
                if (Fairy_StringHasInitialChars(sectionName, "rodata")) {
                    printf(".rel.rodata\n");
                    relocSectionSection[currentRelocSection] = REL_SECTION_RODATA;
                } else if (Fairy_StringHasInitialChars(sectionName, "data")) {
                    printf(".rel.data\n");
                    relocSectionSection[currentRelocSection] = REL_SECTION_DATA;
                } else if (Fairy_StringHasInitialChars(sectionName, "text")) {
                    printf(".rel.text\n");
                    relocSectionSection[currentRelocSection] = REL_SECTION_TEXT;
                }

                currentRelocSection++;
                break;

            case SHT_SYMTAB:
                if (Fairy_StringHasInitialChars(sectionName, "symtab")) {
                    symtabHeader = currentHeader;
                    symtabFound = true;
                }
                break;

            case SHT_STRTAB:
                if (Fairy_StringHasInitialChars(sectionName, "strtab")) {
                    strtabHeader = currentHeader;
                    strtabFound = true;
                }
                break;

            default:
                break;
        }
    }
    /* Can use symbols here too */
    puts(".section .ovl");
    printf("# OverlayInfo\n");
    printf(".word 0x%08X # .text size\n", textSize);
    printf(".word 0x%08X # .data size\n", dataSize);
    printf(".word 0x%08X # .rodata size\n", rodataSize);
    printf(".word 0x%08X # .bss size\n\n", bssSize);

    if (!symtabFound) {
        printf("Symbol table not found");
        return;
    }
    /* Obtain the symbol table */

    symtab = malloc(symtabHeader.sh_size);

    // TODO: Consider replacing this with a lighter-weight read: sufficient to get the name, shndx
    Fairy_ReadSymbolTable(symtab, inputFile, symtabHeader.sh_offset, symtabHeader.sh_size);

    /* Obtain the string table */
    strtab = malloc(strtabHeader.sh_size);
    fseek(inputFile, strtabHeader.sh_offset, SEEK_SET);
    fread(strtab, strtabHeader.sh_size, 1, inputFile);

    /* Do single-file relocs */
    {
        FairyRel* relocs;
        for (currentSection = 0; currentSection < relocSectionsCount; currentSection++) {
            size_t currentReloc;
            size_t sectionRelocCount;
            currentHeader = sectionTable[relocSectionIndices[currentSection]];
            sectionRelocCount = currentHeader.sh_size / sizeof(FairyRel);
            relocs = malloc(currentHeader.sh_size);
            Fairy_ReadRelocs(relocs, inputFile, currentHeader.sh_offset, currentHeader.sh_size);

            for (currentReloc = 0; currentReloc < sectionRelocCount; currentReloc++) {
                FairySym symbol = symtab[ELF32_R_SYM(relocs[currentReloc].r_info)];
                if (symbol.st_shndx == SHN_UNDEF) {
                    continue; // TODO: this is where multifile has to look elsewhere
                }

                printf(".word 0x%08X", Fairy_PackReloc(relocSectionSection[currentSection], relocs[currentReloc]));
                printf(" # %X (%s), %X, 0x%06X", relocSectionSection[currentSection], &shstrtab[currentHeader.sh_name],
                       ELF32_R_TYPE(relocs[currentReloc].r_info), relocs[currentReloc].r_offset);
                printf(", %s", &strtab[symbol.st_name]);
                putchar('\n');

                relocCount++;
            }

            free(relocs);
        }
    }

    printf(".word %d # relocCount\n", relocCount);

    {
        uint32_t ovlSectionSize = ((relocCount + 8) & ~0x03) * sizeof(uint32_t);

        printf("\n.word 0x%08X # Overlay section size\n", ovlSectionSize);
    }

    free(relocSectionIndices);
    free(relocSectionSection);
    free(strtab);
}

void PrintZeldaReloc(FILE* inputFile) {
    FairyFileHeader fileHeader;
    FairySecHeader* sectionTable;
    size_t shstrndx;
    char* shstrtab;
    // FairySym *symbolTable = NULL;
    // size_t symbolTableNum = 0;
    // char *strtab = NULL;

    Fairy_ReadFileHeader(&fileHeader, inputFile);
    sectionTable = malloc(fileHeader.e_shentsize * fileHeader.e_shnum);
    shstrndx = fileHeader.e_shstrndx;

    Fairy_ReadSectionTable(sectionTable, inputFile, fileHeader.e_shoff, fileHeader.e_shnum);

    shstrtab = malloc(sectionTable[shstrndx].sh_size * sizeof(char));

    fseek(inputFile, sectionTable[shstrndx].sh_offset, SEEK_SET);
    fread(shstrtab, sectionTable[shstrndx].sh_size, 1, inputFile);

    Fairy_PrintSectionSizes(sectionTable, inputFile, fileHeader.e_shentsize * fileHeader.e_shnum, shstrtab);

    free(sectionTable);
    free(shstrtab);
}
