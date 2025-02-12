#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn)
{	
	assert(elemSize > 0);
	assert(numBuckets>0);
	assert(hashfn != NULL && comparefn != NULL);
	h->elem_size =elemSize;
	h->cmp_fn = comparefn;
	h->free_fn = freefn;
	h->num_buckets = numBuckets;
	h->hash_fn = hashfn;
	h->buckets = malloc(numBuckets * sizeof(vector));
	assert(h->buckets);
	for (int i = 0; i < numBuckets; i++) {
    	VectorNew(&((vector *)h->buckets)[i], h->elem_size, h->free_fn, 4); 
	}
	h->logical_length = 0;
}

void HashSetDispose(hashset *h)
{
	if (h == NULL || h->buckets == NULL) return;
	for(int i = 0; i< h->num_buckets ; i++){
		VectorDispose(&((vector *)h->buckets)[i]);
	}
	free(h->buckets);
}

int HashSetCount(const hashset *h)
{ 
	if (h == NULL) return 0;
	return h->logical_length; 
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData)
{
	assert(mapfn);
	if(h->logical_length==0){
		return;
	}
	for(int i = 0 ; i< h->num_buckets ; i ++){
		VectorMap(&((vector *)h->buckets)[i],mapfn,auxData);
	}
}

void HashSetEnter(hashset *h, const void *elemAddr)
{
	assert(elemAddr!=NULL);
	int index = h->hash_fn(elemAddr,h->num_buckets);
	assert(index>=0&&index<h->num_buckets);
	int found_elem_index = VectorSearch(&((vector *)h->buckets)[index],elemAddr,
	h->cmp_fn,0,false);
	if(found_elem_index==-1){
		VectorAppend(&((vector *)h->buckets)[index],elemAddr);
		h->logical_length++;
	}else{
		VectorReplace(&((vector *)h->buckets)[index],elemAddr,found_elem_index);
	}
}

void *HashSetLookup(const hashset *h, const void *elemAddr)
{ 
	assert(elemAddr!=NULL);
	int index = h->hash_fn(elemAddr,h->num_buckets);
	assert(index>=0&&index<h->num_buckets);
	int found_elem_index = VectorSearch(&((vector *)h->buckets)[index],elemAddr,
	h->cmp_fn,0,false);
	if(found_elem_index==-1){
		return NULL; 
	}else{
		return VectorNth(&((vector *)h->buckets)[index],found_elem_index);
	}
}
