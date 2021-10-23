/**
 * Implementation based on https://eddmann.com/posts/implementing-a-dynamic-vector-array-in-c/
 * Some parts have been adjusted to suit this use case.
 */
#ifndef VECTOR_H
#define VECTOR_H

#define VECTOR_INIT_CAPACITY 16

#define VECTOR_INIT(vec) \
    Vector vec;          \
    Vector_Init(&vec)
#define VECTOR_COUNT(vec) Vector_Count(&vec)
#define VECTOR_ADD(vec, item) Vector_AddItem(&vec, (void*)item)
#define VECTOR_SET(vec, id, item) Vector_SetItem(&vec, id, (void*)item)
#define VECTOR_GET(vec, type, id) (type) Vector_GetItem(&vec, id)
#define VECTOR_DELETE(vec, id) Vector_DeleteItem(&vec, id)
#define VECTOR_FREE(vec) Vector_Destroy(&vec)

typedef struct Vector {
    void** items;
    int capacity;
    int count;
} Vector;

void Vector_Init(Vector*);
int Vector_Count(Vector*);

void Vector_AddItem(Vector*, void*);
void Vector_SetItem(Vector*, int, void*);
void* Vector_GetItem(Vector*, int);
void Vector_Delete(Vector*, int);
void Vector_Destroy(Vector*);

#endif
