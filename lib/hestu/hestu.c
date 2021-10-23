#include "hestu.h"
#include <stdio.h>
#include <stdlib.h>



// #define HESTU_INIT(array, initialCapacity) Hestu_Init(array, sizeof(*array), initialCapacity)
void* Hestu_Init(void* array, size_t elementSize, size_t initialCapacity) {
    HestuArrayHeader* header;

    if (array == NULL) {
        header = malloc(sizeof(HestuArrayHeader) + initialCapacity * elementSize);
        header->count = 0;
        header->capacity = initialCapacity;
        header->elementSize = elementSize;
        return header + 1;
    }

    fprintf(stderr, "error: array is not NULL, so cannot be initialised.\n\tEither initialise it to NULL, or use "
                    "Hestu_ReInit instead.");
    return NULL;
}

void* Hestu_ReInit(void* array, size_t elementSize, size_t initialCapacity) {
    HestuArrayHeader* header;

    if (array != NULL) {
        free((HestuArrayHeader*)array - 1);
        header = malloc(sizeof(HestuArrayHeader) + initialCapacity * elementSize);
        header->count = 0;
        header->capacity = initialCapacity;
        header->elementSize = elementSize;
        return header;
    }

    fprintf(stderr, "error: array is NULL, so cannot be reinitialised.");
    return NULL;
}

void* Hestu_Resize(HestuArrayHeader* header, size_t elementSize, size_t newCapacity) {
    if (header->count > newCapacity) {
        fprintf(stderr, "warning: count %zd > newCapacity %zd; elements will be lost\n", header->count, newCapacity);
    }
    printf("Reallocking to %d\n", newCapacity);
    header = realloc(header, sizeof(HestuArrayHeader) +  newCapacity * elementSize);
    assert( header != NULL || !"resizing header failed");
    return header + 1;
}

/* Because we don't expect this to be used too often with large numbers, limit the doubling range */
#define HESTU_EXPANSION_SWITCH (1 << 10)
#define HESTU_EXPANSION_FIXED_SIZE (1 << 10)
size_t Hestu_Expand(HestuArrayHeader* header) {
    if (header->capacity < HESTU_EXPANSION_SWITCH) {
        return header->capacity * 2;
    } else {
        return header->capacity + HESTU_EXPANSION_FIXED_SIZE;
    }
}

// #define HESTU_RESIZE(array, count) \
//     (HESTU_HEADER(array)->count == HESTU_HEADER(array)->capacity ? HESTU_EXPAND(array) : 0)
// #define HESTU_CHECK_EXPAND(array, count) \
//     (HESTU_HEADER(array)->count == HESTU_HEADER(array)->capacity ? HESTU_EXPAND(HESTU_HEADER(array)) : 0)
// #define HESTU_SET(array, item) array[HESTU_HEADER(array)->count++] = (item);

// /* Cannot work, see macro in header */
// #define HESTU_ADDITEM(array, item) Hestu_AddItem(array, sizeof(*array), item)
// void Hestu_AddItem(void** array, size_t elementSize, void* item) {
//     HestuArrayHeader* header = HESTU_HEADER(array);
//     if (header->count == header->capacity) {
//         Hestu_Resize(header, elementSize, Hestu_Expand(header));
//     }
//     array[HESTU_HEADER(array)->count++] = item;
// }

// #define HESTU_DESTROY(array) Hestu_Destroy(array)
void Hestu_Destroy(void* array) {
    free(((HestuArrayHeader*)array) - 1);
}

// void Test() {
//     int* array;
//     array = HESTU_INIT(array, 4);
//     HESTU_ADDITEM(array, 1);
//     HESTU_ADDITEM(array, 2);
//     HESTU_ADDITEM(array, 3);
//     HESTU_ADDITEM(array, 18);
//     HESTU_ADDITEM(array, 36);

//     {
//         int i;
//         for (i = 0; i < HESTU_HEADER(array)->count; i++) {
//             printf("%d", array[i]);
//         }
//         putchar('\n');
//     }
//     HESTU_DESTROY(array);
// }

// TYPEDEF_DYNAMIC_ARRAY(int);

// void Test_Template() {
//     DYNAMIC_ARRAY(int) dynArray;

//     intArray_Init(&dynArray);

//     intArray_AddItem(&dynArray, 1);
//     intArray_AddItem(&dynArray, 2);
//     intArray_AddItem(&dynArray, 3);
//     intArray_AddItem(&dynArray, 18);
//     intArray_AddItem(&dynArray, 36);

//     {
//         int i;
//         for (i = 0; i < dynArray.count; i++) {
//             printf("%d, ", dynArray.array[i]);
//         }
//         putchar('\n');
//     }
//     intArray_Destroy(&dynArray);
// }
