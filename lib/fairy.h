#pragma once

#include <stddef.h>
#include <stdio.h>
#include <elf.h>

typedef Elf32_Ehdr FairyFileHeader;
typedef Elf32_Shdr FairySecHeader;
typedef Elf32_Sym FairySym;
typedef Elf32_Rel FairyRel;

FairyFileHeader *Fairy_ReadFileHeader(FairyFileHeader *header, FILE *file);
FairySecHeader *Fairy_ReadSectionTable(FairySecHeader *sectionTable, size_t tableOffset, size_t number, FILE *file);

void Fairy_PrintSymbolTable(FILE *inputFile);

FairyRel* Fairy_ReadRelocs(FairyRel* relocTable, size_t offset, size_t number, FILE* file);
void Fairy_PrintRelocs(FILE *inputFile);
void Fairy_PrintSectionTable(FILE *inputFile);

void PrintZeldaReloc(FILE* file);
