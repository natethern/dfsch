/*
 * dfsch - dfox's quick and dirty scheme implementation
 *   Internal identity hash tables
 * Copyright (C) 2009 Ales Hakl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <dfsch/eqhash.h>
#include <stdint.h>
#include <limits.h>

#define INITIAL_MASK 0x7

static size_t fast_ptr_hash(dfsch_object_t* ptr){
  size_t p = (size_t) ptr;

  p ^= (p >> 3) | (p << (CHAR_BIT*sizeof(size_t) - 3));
  p ^= (p << 12) | (p >> (CHAR_BIT*sizeof(size_t) - 12));

  return p;
}

static dfsch_eqhash_entry_t** alloc_vector(size_t mask){
  return GC_MALLOC(sizeof(dfsch_eqhash_entry_t*) * (mask + 1));
}

void dfsch_eqhash_init(dfsch_eqhash_t* hash, int start_large){
  int i;
  if (start_large){
    for (i = 0; i < DFSCH_EQHASH_SMALL_SIZE * 2; i++){
      hash->contents.large.cache[i] = NULL;
    }
    hash->contents.large.mask = INITIAL_MASK;
    hash->contents.large.count = 0;
    hash->contents.large.vector = alloc_vector(INITIAL_MASK);
  } else {
    for (i = 0; i < DFSCH_EQHASH_SMALL_SIZE; i++){
      hash->contents.small.keys[i] = DFSCH_INVALID_OBJECT;
      hash->contents.small.values[i] = NULL;
      hash->contents.small.flags[i] = 0;
    }
  }
  hash->is_large = start_large;
}

static dfsch_eqhash_entry_t* alloc_entry(dfsch_object_t* key,
                                         dfsch_object_t* value,
                                         int flags,
                                         dfsch_eqhash_entry_t* next){
  dfsch_eqhash_entry_t* e = GC_NEW(dfsch_eqhash_entry_t);
  e->key = key;
  e->value = value;
  e->next = next;
  e->flags = flags;
  return e;
}

#define BUCKET(hash, index)                     \
  (hash->contents.large.vector[(index) & hash->contents.large.mask])

static void convert_to_large(dfsch_eqhash_t* hash){
  dfsch_eqhash_entry_t** vector = alloc_vector(INITIAL_MASK);
  size_t h;
  int i;
  
  for (i = 0; i < DFSCH_EQHASH_SMALL_SIZE; i++){
    if (hash->contents.small.keys[i] != DFSCH_INVALID_OBJECT){
      h = fast_ptr_hash(hash->contents.small.keys[i]);
      vector[h & INITIAL_MASK] = alloc_entry(hash->contents.small.keys[i],
                                             hash->contents.small.values[i],
                                             hash->contents.small.flags[i],
                                             vector[h & INITIAL_MASK]);
    }
  }

  for (i = 0; i < DFSCH_EQHASH_SMALL_SIZE * 2; i++){
    hash->contents.large.cache[i] = NULL;
  }
  
  hash->contents.large.vector = vector;
  hash->contents.large.count = DFSCH_EQHASH_SMALL_SIZE;
  hash->contents.large.mask = INITIAL_MASK;  
  hash->is_large = 1;
}

void dfsch_eqhash_put(dfsch_eqhash_t* hash,
                      dfsch_object_t* key, dfsch_object_t* value){
  if (!hash->is_large){
    int i;
    for (i = 0; i < DFSCH_EQHASH_SMALL_SIZE; i++){
      if (hash->contents.small.keys[i] == DFSCH_INVALID_OBJECT){
        hash->contents.small.keys[i] = key;
        hash->contents.small.values[i] = value;
        return;
      }
    }

    convert_to_large(hash);
  }
  size_t h = fast_ptr_hash(key);
  hash->contents.large.count++;
  BUCKET(hash, h) = alloc_entry(key, value, 0, BUCKET(hash,h));
}

static dfsch_eqhash_entry_t* find_entry(dfsch_eqhash_t* hash, 
                                        dfsch_object_t* key){
  dfsch_eqhash_entry_t* i;
  size_t h;
  h = fast_ptr_hash(key);
  i = hash->contents.large.cache[(h >> 10) % (DFSCH_EQHASH_SMALL_SIZE * 2)];
  if (i && i->key == key){
    return i;
  }
  
  i = BUCKET(hash, h);
  while (i){
    if (i->key == key){
      hash->contents.large.cache[(h >> 10) % (DFSCH_EQHASH_SMALL_SIZE * 2)] = i;
      return i;
    }
    i = i->next;
  }
  return NULL;
}

void dfsch_eqhash_set(dfsch_eqhash_t* hash,
                      dfsch_object_t* key, dfsch_object_t* value){
  if (hash->is_large){
    dfsch_eqhash_entry_t* e = find_entry(hash, key);
    if (e) {
      e->value = value;
      return;
    }
  } else {
    int i;
    for (i = 0; i < DFSCH_EQHASH_SMALL_SIZE; i++){
      if (hash->contents.small.keys[i] == key){
        hash->contents.small.values[i] = value;
        return;
      }
    }
  }
  dfsch_eqhash_put(hash, key, value);  
}
void dfsch_eqhash_set_flags(dfsch_eqhash_t* hash,
                            dfsch_object_t* key, long flags){
  if (hash->is_large){
    dfsch_eqhash_entry_t* e = find_entry(hash, key);
    if (e) {
      e->flags = flags;
      return;
    }
  } else {
    int i;
    for (i = 0; i < DFSCH_EQHASH_SMALL_SIZE; i++){
      if (hash->contents.small.keys[i] == key){
        hash->contents.small.flags[i] = flags;
        return;
      }
    }
  }
}
int dfsch_eqhash_set_if_exists(dfsch_eqhash_t* hash,
                               dfsch_object_t* key, dfsch_object_t* value,
                               long* flags){
  if (hash->is_large){
    dfsch_eqhash_entry_t* e = find_entry(hash, key);
    if (e) {
      e->value = value;
      return 1;
    }
  } else {
    int i;
    for (i = 0; i < DFSCH_EQHASH_SMALL_SIZE; i++){
      if (hash->contents.small.keys[i] == key){
        hash->contents.small.values[i] = value;
        return 1;
      }
    }
  }  
  return 0;
}
int dfsch_eqhash_unset(dfsch_eqhash_t* hash, dfsch_object_t* key){
  return 1; // TODO

}

int dfsch_eqhash_ref(dfsch_eqhash_t* hash,
                     dfsch_object_t* key, 
                     dfsch_object_t** value, long *flags,
                     dfsch_eqhash_entry_t** entry){
  if (hash->is_large){
    dfsch_eqhash_entry_t* e = find_entry(hash, key);
    if (e) {
      if (value){
        *value = e->value;
      }
      if (flags){
        *flags = e->flags;
      }
      if (entry){
        *entry = e;
      }
      return 1;
    }
  } else {
    int i;
    for (i = 0; i < DFSCH_EQHASH_SMALL_SIZE; i++){
      if (hash->contents.small.keys[i] == key){
        if (value) {
          *value = hash->contents.small.values[i];
        }
        if (flags) {
          *flags = hash->contents.small.flags[i];
        }
        return 1;
      }
    }
  }  
  return 0;  
}
