#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>

void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation)
{   
    assert(elemSize > 0);
    if(initialAllocation==0){
        initialAllocation = 4;
    }
    v->allocated_length = initialAllocation;
    v->logical_length = 0;
    v->free_fn = freeFn;
    v->elem_size = elemSize;
    v->base = malloc(v->allocated_length*v->elem_size);
    assert(v->base);
}

void VectorDispose(vector *v)
{
    if (v == NULL) return;
    if(v->free_fn!=NULL){
        for(int i = 0 ; i < v->logical_length ; i ++){
            void * elem_adress = (char *)v->base + i*v->elem_size;
            v->free_fn(elem_adress);
        }
    }
    if (v->base != NULL) {
        free(v->base);
        v->base = NULL;
    }
}

int VectorLength(const vector *v)
{
    return v->logical_length; 
}

void *VectorNth(const vector *v, int position)
{ 
    assert(position >= 0 && position < v->logical_length);
    void *found_elem = (char *)v->base + position * v->elem_size;
    return found_elem;
}

void VectorReplace(vector *v, const void *elemAddr, int position)
{
    assert(position >= 0 && position < v->logical_length);
    void *to_replace = (char *)v->base + position * v->elem_size;
    if(v->free_fn!=NULL){
        v->free_fn(to_replace);
    }
     memcpy(to_replace, elemAddr, v->elem_size);
}

void VectorInsert(vector *v, const void *elemAddr, int position)
{
    assert(position >= 0 && position <= v->logical_length);
    if (v->logical_length == v->allocated_length) {
        v->allocated_length *= 2;
        v->base = realloc(v->base, v->allocated_length * v->elem_size);
        assert(v->base != NULL);
    }
    void *to_place = (char *)v->base + v->elem_size * position;

    memmove((char *)to_place + v->elem_size, to_place, v->elem_size * (v->logical_length - position));

    memcpy(to_place, elemAddr, v->elem_size);

    v->logical_length++;
}

void VectorAppend(vector *v, const void *elemAddr)
{
    if (v->logical_length == v->allocated_length) {
        v->allocated_length *= 2;
        v->base = realloc(v->base, v->allocated_length * v->elem_size);
        assert(v->base != NULL);
    }
    void * position = (char *)v->base +v->logical_length *v->elem_size;
    memcpy(position ,elemAddr ,v->elem_size);
    v->logical_length++; 
}

void VectorDelete(vector *v, int position)
{
    assert(position >= 0 && position < v->logical_length);
    assert(v->logical_length);//should not be empty
    void *to_remove = (char *)v->base + v->elem_size * position;
    if(v->free_fn!=NULL){
        v->free_fn(to_remove);
    } 
    //|0|1|2|3|4|5|6|
    //|0|2|3|4|5|6|6|
    memmove(to_remove,(char *)to_remove+v->elem_size, v->elem_size * (v->logical_length - 1 - position));
    void *extra_last_elem =(char *)v->base +v->elem_size * (v->logical_length-1);
    // if(v->free_fn !=NULL){
    //     v->free_fn(extra_last_elem);
    // }
    v->logical_length--;
}

void VectorSort(vector *v, VectorCompareFunction compare)
{
    assert(v->base != NULL); 
    if (v->logical_length > 0) {
        qsort(v->base, v->logical_length, v->elem_size, compare);
    }
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData)
{
    assert(mapFn);
    if (v->logical_length == 0) {
        return; 
    }
    for(int i = 0; i < v->logical_length ; i++){
        void * cur_elem = (char *)v->base + i * v->elem_size ;
        mapFn(cur_elem,auxData);
    }
}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted)
{ 
    assert(startIndex >= 0 && startIndex <= v->logical_length);
    assert(searchFn != NULL && key != NULL);

    if (startIndex == v->logical_length || v->logical_length == 0) {
        return kNotFound;
    }

    void *cur_base = (char *)v->base + startIndex * v->elem_size;
    int left_elems = v->logical_length - startIndex;

    if (isSorted) {
        void *found_elem = bsearch(key, cur_base, left_elems, v->elem_size, searchFn);
        if (found_elem == NULL) {
            return kNotFound; 
        }

        int index = ((char *)found_elem - (char *)v->base) / v->elem_size;
        return index;
    } else {
        for (int i = 0; i < left_elems; i++) {
            void *cur_elem = (char *)cur_base + i * v->elem_size;
            if (searchFn(key, cur_elem) == 0) {
                return startIndex + i; 
            }
        }
    }

    return kNotFound; 
} 
