#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "vector.h"

#define DECLARE_0x0
#define DECLARE_0x1 typedef int Dummy##__FILE__##_##__LINE__
#define DECLARE_0x2 \
    DECLARE_0x1;    \
    DECLARE_0x1
#define DECLARE_0x4 \
    DECLARE_0x2;    \
    DECLARE_0x2
#define DECLARE_0x8 \
    DECLARE_0x4;    \
    DECLARE_0x4
#define DECLARE_0x10 \
    DECLARE_0x8;     \
    DECLARE_0x8
#define DECLARE_0x20 \
    DECLARE_0x10;    \
    DECLARE_0x10
#define DECLARE_0x40 \
    DECLARE_0x20;    \
    DECLARE_0x20
#define DECLARE_0x80 \
    DECLARE_0x40;    \
    DECLARE_0x40

#define DECLARE(n)          \
    DECLARE_0x##(n & 0x80); \
    DECLARE_0x##(n & 0x40); \
    DECLARE_0x##(n & 0x20); \
    DECLARE_0x##(n & 0x10); \
    DECLARE_0x##(n & 0x08); \
    DECLARE_0x##(n & 0x04); \
    DECLARE_0x##(n & 0x02); \
    DECLARE_0x##(n & 0x01);

#ifdef DEBUG_ON
#define CHECK_BOUNDS(v, index)                                                                              \
    if (index < 0 || index >= v->count) {                                                                    \
        fprintf(stderr, "%s: warning: index %d is out-of-bounds: not in [0,%d]", __func__, index, v->count); \
        abort();                                                                                             \
    } (void)0
#else
#define CHECK_BOUNDS(v, index)
#endif

void Vector_Init(Vector* v) {
    v->capacity = VECTOR_INIT_CAPACITY;
    v->count = 0;
    v->items = malloc(sizeof(void*) * v->capacity);
}

int Vector_Count(Vector* v) {
    return v->count;
}

/* Internal resize function. Use AddItem or DeleteItem to  */
static void Vector_Resize(Vector* v, int capacity) {
#ifdef DEBUG_ON
    printf("Vector_Resize: %d to %d\n", v->capacity, capacity);
#endif

    void** items = realloc(v->items, sizeof(void*) * capacity);
    if (items) {
        v->items = items;
        v->capacity = capacity;
    }
}

/* Add an item to the end of the Vector */
void Vector_AddItem(Vector* v, void* item) {
    if (v->capacity == v->count) {
        Vector_Resize(v, v->capacity * 2);
    }
    v->items[v->count++] = item;
}

/* Overwrite an item */
void Vector_SetItem(Vector* v, int index, void* item) {
    CHECK_BOUNDS(v, index);
    v->items[index] = item;
}

void* Vector_GetItem(Vector* v, int index) {
    CHECK_BOUNDS(v, index);
    return v->items[index];
}

void Vector_DeleteItem(Vector* v, int index) {
    CHECK_BOUNDS(v, index);

    v->items[index] = NULL;

    for (int i = index; i < v->count - 1; i++) {
        v->items[i] = v->items[i + 1];
        v->items[i + 1] = NULL;
    }

    v->count--;

    if (v->count > 0 && v->count == v->capacity / 4) {
        Vector_Resize(v, v->capacity / 2);
    }
}

void Vector_Destroy(Vector* v) {
    free(v->items);
}
