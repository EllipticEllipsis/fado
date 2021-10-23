#pragma once

#include <stdlib.h>
#include <assert.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * Functionful implementation: mostly functions, with element sizes hidden behind macros.
 */

#define HESTU_INITIAL_CAPACITY 4

/* Used to access the array's header */
typedef struct {
    size_t count;
    size_t capacity;
    size_t elementSize;
} HestuArrayHeader;

#define HESTU_HEADER(array) ((HestuArrayHeader*)(array)-1)

#define HESTU_INIT(array, initialCapacity) Hestu_Init(array, sizeof(*array), initialCapacity)
#define HESTU_GETCOUNT(array) (HESTU_HEADER(array)->count)
#define HESTU_ADDITEM(array, item)                                              \
    do {                                                                        \
        HestuArrayHeader* header = HESTU_HEADER(array);                         \
        if (header->count == header->capacity) {                                \
            array = Hestu_Resize(header, sizeof(*array), Hestu_Expand(header)); \
        }                                                                       \
        array[HESTU_HEADER(array)->count++] = item;                             \
    } while (0)

#define HESTU_DESTROY(array) Hestu_Destroy(array)

void* Hestu_Init(void* array, size_t elementSize, size_t initialCapacity);

size_t Hestu_Expand(HestuArrayHeader* header);
void* Hestu_Resize(HestuArrayHeader* header, size_t elementSize, size_t newCapacity);

// void Hestu_AddItem(void** array, size_t elementSize, void* item);

/* Beware, this assumes that the array was at least correctly initialised with this library! */
void Hestu_Destroy(void* array);

//=================================================

/**
 * Template-like implementation
 * the TYPEDEF macro makes a custom struct typeArrayInfo (used via the DYNAMIC_ARRAY(type) macro) and
 * all the functions desired to manipulate it, which are of the form typeArray_FunctionName.
 */

#define DYNAMIC_ARRAY_INITIAL_CAPACITY 4

#define DYNAMIC_ARRAY(type) type##Array##Info
#define TYPEDEF_DYNAMIC_ARRAY(type)                                                                        \
    typedef struct {                                                                                       \
        type* array;                                                                                       \
        size_t count;                                                                                      \
        size_t capacity;                                                                                   \
    } type##Array##Info;                                                                                   \
                                                                                                           \
    DYNAMIC_ARRAY(type) * type##Array_Init(DYNAMIC_ARRAY(type) * dynArray) {                               \
        dynArray->array = malloc(DYNAMIC_ARRAY_INITIAL_CAPACITY * sizeof(type));                           \
        dynArray->count = 0;                                                                               \
        dynArray->capacity = DYNAMIC_ARRAY_INITIAL_CAPACITY;                                               \
        return dynArray;                                                                                   \
    }                                                                                                      \
                                                                                                           \
    type* type##Array_ReSize(DYNAMIC_ARRAY(type) * dynArray, size_t newCapacity) {                         \
        if (dynArray->count > newCapacity) {                                                               \
            fprintf(stderr, "%s: warning: count %zd > newCapacity %zd; elements will be lost\n", __func__, \
                    dynArray->count, newCapacity);                                                         \
        }                                                                                                  \
        dynArray->array = realloc(dynArray->array, newCapacity * sizeof(*(dynArray->array)));              \
        return dynArray->array;                                                                            \
    }                                                                                                      \
                                                                                                           \
    void type##Array_AddItem(DYNAMIC_ARRAY(type) * dynArray, type item) {                                  \
        if (dynArray->count == dynArray->capacity) {                                                       \
            dynArray->array = type##Array_ReSize(dynArray, dynArray->capacity * 2);                        \
        }                                                                                                  \
        dynArray->array[dynArray->count] = item;                                                           \
        dynArray->count++;                                                                                 \
    }                                                                                                      \
    void type##Array_Destroy(DYNAMIC_ARRAY(type) * dynArray) {                                             \
        free(dynArray->array);                                                                             \
    }
