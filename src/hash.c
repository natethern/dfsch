/*
 * dfsch - dfox's quick and dirty scheme implementation
 *   Hash tables
 * Copyright (C) 2005-2008 Ales Hakl
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

#include <dfsch/hash.h>
#include <dfsch/compile.h>
#include "internal.h"
#include <stdlib.h>
#include "util.h"

#include <stdio.h>

#define INITIAL_MASK 0x07

typedef struct hash_entry_t hash_entry_t;

typedef struct hash_t{
  dfsch_type_t* type;
  dfsch_object_t* proc;
  size_t count;
  size_t mask;
  hash_entry_t** vector;
  int mode;
  pthread_mutex_t* w_mutex;
  pthread_mutex_t* r_mutex;
}hash_t;

struct hash_entry_t {
  size_t hash;
  dfsch_object_t* key;
  dfsch_object_t* value;

  hash_entry_t* next;
};

static const dfsch_type_t hash_type = {
  DFSCH_STANDARD_TYPE,
  NULL,
  sizeof(hash_t),
  "hash",
  NULL,
  NULL,
  NULL
};

static hash_entry_t** alloc_vector(size_t mask){
  return GC_MALLOC((sizeof(hash_entry_t*))*(mask+1));
}


dfsch_object_t* dfsch_hash_make(int mode){
  hash_t *h = (hash_t*)dfsch_make_object(&hash_type); 

  h->count = 0;
  h->mask = INITIAL_MASK;
  h->vector = alloc_vector(h->mask);
  h->mode = mode;
  h->w_mutex = create_finalized_mutex();
  h->r_mutex = create_finalized_mutex();

  return (dfsch_object_t*)h;
}
int dfsch_hash_p(dfsch_object_t* obj){
  return obj && obj->type == &hash_type;
}

static size_t ptr_hash(dfsch_object_t* ptr){
  size_t a = (size_t)ptr;        
  size_t b = (size_t)ptr >> 16 | (size_t)ptr << 16;

  a ^= b >> 2;
  b ^= a >> 3;
  a ^= b << 5;
  b ^= a << 7;
  a ^= b >> 11;
  b ^= a >> 13;
  a ^= b << 17;
  b ^= a << 23;
  
  return b ^ a;
}

static size_t get_hash(hash_t* hash, dfsch_object_t*key){
  
  if (hash->mode == DFSCH_HASH_EQ){
    return ptr_hash(key);
  } else {
    return dfsch_hash(key);
  }
}

#define GET_HASH(obj,hash)\
   if (!obj || obj->type != &hash_type)\
     dfsch_error("exception:not-a-hash", obj); \
   hash = (hash_t*)obj;

/*
 * Synchronization is generally required only for modification - readers
 * can read concurrently with modification and see consistent state.
 */

dfsch_object_t* dfsch_hash_ref(dfsch_object_t* hash_obj, 
                               dfsch_object_t* key){
  
  size_t h;
  hash_t *hash;
  hash_entry_t *i;

  GET_HASH(hash_obj, hash);

  h = get_hash(hash, key);  

  pthread_mutex_lock(hash->r_mutex);
  i = hash->vector[h & hash->mask];
  pthread_mutex_unlock(hash->r_mutex);

  switch (hash->mode){
  case DFSCH_HASH_EQ:
    while (i){
      if (h == i->hash && (i->key == key)){
        return dfsch_list(1,i->value);
      }
      
      i = i->next;
    }
    break;
  case DFSCH_HASH_EQV:
    while (i){
      if (h == i->hash && dfsch_eqv_p(i->key, key))
        return dfsch_list(1,i->value);
      
      i = i->next;
    }
    break;
  case DFSCH_HASH_EQUAL:
    while (i){
      if (h == i->hash && dfsch_equal_p(i->key, key))
        return dfsch_list(1,i->value);
      
      i = i->next;
    }
    break;
  }
  return NULL;
}

static hash_entry_t* alloc_entry(size_t hash, 
                                 dfsch_object_t* key,
                                 dfsch_object_t* value,
                                 hash_entry_t* next){
  hash_entry_t *e = GC_NEW(hash_entry_t);
  e->hash = hash;
  e->key = key;
  e->value = value;
  e->next = next;
  return e;
}

static void hash_change_size(hash_t* hash, size_t new_mask){
  int j;
  hash_entry_t *i;
  hash_entry_t **vector = alloc_vector(new_mask);
  
  for (j = 0; j <= hash->mask; j++){
    i = hash->vector[j];
    while (i){
      hash_entry_t *next = i->next;
      size_t h = i->hash;
      if (i->next != vector[h & new_mask]){
        vector[h & new_mask] = alloc_entry(h,
                                           i->key,
                                           i->value,
                                           vector[h & new_mask]);
      } else {
        vector[h & new_mask] = i;
      }
      i = next;
    }
  }

  pthread_mutex_lock(hash->r_mutex);
  hash->mask = new_mask;
  hash->vector = vector;
  pthread_mutex_unlock(hash->r_mutex);
}

dfsch_object_t* dfsch_hash_set(dfsch_object_t* hash_obj,
                               dfsch_object_t* key,
                               dfsch_object_t* value){
  size_t h, len, count;
  hash_t *hash;
  hash_entry_t *entry;
  hash_entry_t *i;

  GET_HASH(hash_obj,hash);

  pthread_mutex_lock(hash->w_mutex);

  h = get_hash(hash, key);  
  i = entry = hash->vector[h & hash->mask];

  switch (hash->mode){
  case DFSCH_HASH_EQ:
    while (i){
      if (h == i->hash && i->key == key){
        i->value = value;
        pthread_mutex_unlock(hash->w_mutex);
        return hash_obj;
      }
      
      i = i->next;
    }
    break;
  case DFSCH_HASH_EQV:
    while (i){
      if (h == i->hash && dfsch_eqv_p(i->key, key)){
        i->value = value;
        pthread_mutex_unlock(hash->w_mutex);
        return hash_obj;
      }
      
      i = i->next;
    }
    break;
  case DFSCH_HASH_EQUAL:
    while (i){
      if (h == i->hash && dfsch_equal_p(i->key, key)){
        i->value = value;
        pthread_mutex_unlock(hash->w_mutex);
        return hash_obj;
      }
      
      i = i->next;
    }
    break;
  }
  
  // It isn't here, so we will add new item

  hash->count++;
  if (hash->count > (hash->mask+1)){ // Should table grow?
    hash_change_size(hash, ((hash->mask+1) * 2) - 1);
  }

  hash->vector[h & hash->mask] = alloc_entry(h,
                                             key,
                                             value,
                                             hash->vector[h & hash->mask]);
  
  pthread_mutex_unlock(hash->w_mutex);
  return hash_obj;
}

int dfsch_hash_unset(dfsch_object_t* hash_obj,
                     dfsch_object_t* key){
  size_t h;
  hash_t *hash;
  hash_entry_t *i, *j;

  GET_HASH(hash_obj, hash);

  pthread_mutex_lock(hash->w_mutex);

  h = get_hash(hash, key);  
  i = hash->vector[h & hash->mask];
  j = NULL;

  switch (hash->mode){
  case DFSCH_HASH_EQ:
    while (i){
      if (h == i->hash && (i->key == key)) {
        if (j){
          j->next = i->next;
        } else {
          hash->vector[h & hash->mask] = NULL;
        }
        hash->count --;
        
        if (hash->count+16 < (hash->mask+1)/2 
            && hash->mask != 0x3){ // Should table shrink?
          hash_change_size(hash, ((hash->mask+1) / 2) - 1);
        }
        
        
        pthread_mutex_unlock(hash->w_mutex);
        return 1;
      }
      
      j = i;
      i = i->next;
    }
    break;

  case DFSCH_HASH_EQV:
    while (i){
      if (h == i->hash && dfsch_eqv_p(i->key, key)) {
        if (j){
          j->next = i->next;
        } else {
          hash->vector[h & hash->mask] = NULL;
        }
        hash->count --;
        
        if (hash->count+16 < (hash->mask+1)/2 
            && hash->mask != 0x3){ // Should table shrink?
          hash_change_size(hash, ((hash->mask+1) / 2) - 1);
        }
        
        
        pthread_mutex_unlock(hash->w_mutex);
        return 1;
      }
      
      j = i;
      i = i->next;
    }
    break;

  case DFSCH_HASH_EQUAL:
    while (i){
      if (h == i->hash && dfsch_equal_p(i->key, key)) {
        if (j){
          j->next = i->next;
        } else {
          hash->vector[h & hash->mask] = NULL;
        }

        hash->count --;
        
        if (hash->count+16 < (hash->mask+1)/2
            && hash->mask != 0x3){ // Should table shrink?
          hash_change_size(hash, ((hash->mask+1) / 2) - 1);
        }
        
        
        pthread_mutex_unlock(hash->w_mutex);
        return 1;
      }
      
      j = i;
      i = i->next;
    }
    break;
  }

  pthread_mutex_unlock(hash->w_mutex);
  return 0;
  
}

/*
 * Im not fully sure whenever synchronization is strictly necessary here.
 */
int dfsch_hash_set_if_exists(dfsch_object_t* hash_obj, 
                             dfsch_object_t* key,
                             dfsch_object_t* value){
  
  size_t h;
  hash_t *hash;
  hash_entry_t *i;

  GET_HASH(hash_obj, hash);

  pthread_mutex_lock(hash->w_mutex);

  h = get_hash(hash, key);  
  i = hash->vector[h & hash->mask];
  
  switch (hash->mode){
  case DFSCH_HASH_EQ:
    while (i){
      if (h = i->hash && i->key == key){
        i->value = value;

        pthread_mutex_unlock(hash->w_mutex);
        return 1;
      }
      i = i->next;
    }
    break;
  case DFSCH_HASH_EQV:
    while (i){
      if (h = i->hash && dfsch_eqv_p(i->key, key)){
        i->value = value;

        pthread_mutex_unlock(hash->w_mutex);
        return 1;
      }
      i = i->next;
    }
    break;
  case DFSCH_HASH_EQUAL:
    while (i){
      if (h = i->hash && dfsch_equal_p(i->key, key)){
        i->value = value;

        pthread_mutex_unlock(hash->w_mutex);
        return 1;
      }
      i = i->next;
    }
    break;
  }

  pthread_mutex_unlock(hash->w_mutex);
  return 0;
}


dfsch_object_t* dfsch_hash_2_alist(dfsch_object_t* hash_obj){
  dfsch_object_t *alist = NULL;
  int j;
  hash_entry_t *i;
  hash_t *hash;
  
  GET_HASH(hash_obj, hash);
  
  for (j=0; j<(hash->mask+1); j++){
    i = hash->vector[j];
    while (i){
      alist = dfsch_cons(dfsch_list(2,
                                    i->key,
                                    i->value), 
                         alist);
      i = i->next;
    }
  }

  return alist;
}

dfsch_object_t* dfsch_alist_2_hash(dfsch_object_t* alist,
                                   int mode){
  dfsch_object_t* hash = dfsch_hash_make(mode);
  dfsch_object_t* i = alist;
  
  while (dfsch_pair_p(i)){
    dfsch_object_t* item = dfsch_car(i);
    dfsch_object_t* name = dfsch_list_item(item, 0);
    dfsch_object_t* value = dfsch_list_item(item, 1);

    dfsch_hash_set(hash, name, value);

    i = dfsch_cdr(i);
  }

  return hash;
}


/////////////////////////////////////////////////////////////////////////////
//
// Scheme binding
//
/////////////////////////////////////////////////////////////////////////////

static dfsch_object_t* native_make_hash(void* baton, dfsch_object_t* args,
                                        dfsch_tail_escape_t* esc){
  dfsch_object_t *mode;
  DFSCH_OBJECT_ARG_OPT(args, mode, NULL);
  DFSCH_ARG_END(args);

  if (!mode)
    return dfsch_hash_make(DFSCH_HASH_EQ);
  if (mode == dfsch_make_symbol("equal?"))
    return dfsch_hash_make(DFSCH_HASH_EQUAL);
  if (mode == dfsch_make_symbol("eqv?"))
    return dfsch_hash_make(DFSCH_HASH_EQV);
  if (mode == dfsch_make_symbol("eq?"))
    return dfsch_hash_make(DFSCH_HASH_EQ);

  dfsch_error("exception:unknown-mode", mode);
}
static dfsch_object_t* native_hash_p(void* baton, dfsch_object_t* args,
                                     dfsch_tail_escape_t* esc){
  dfsch_object_t* obj;
  DFSCH_OBJECT_ARG(args, obj);
  DFSCH_ARG_END(args);

  return dfsch_bool(dfsch_hash_p(obj));
}
static dfsch_object_t* native_hash_ref(void* baton, dfsch_object_t* args,
                                       dfsch_tail_escape_t* esc){
  dfsch_object_t* hash;
  dfsch_object_t* key;
  DFSCH_OBJECT_ARG(args, hash);
  DFSCH_OBJECT_ARG(args, key);
  DFSCH_ARG_END(args);

  return dfsch_hash_ref(hash, key);
}
static dfsch_object_t* native_hash_unset(void* baton, dfsch_object_t* args,
                                         dfsch_tail_escape_t* esc){
  dfsch_object_t* hash;
  dfsch_object_t* key;
  DFSCH_OBJECT_ARG(args, hash);
  DFSCH_OBJECT_ARG(args, key);
  DFSCH_ARG_END(args);

  return dfsch_bool(dfsch_hash_unset(hash, key));
}
static dfsch_object_t* native_hash_set(void* baton, dfsch_object_t* args,
                                       dfsch_tail_escape_t* esc){
  dfsch_object_t* hash;
  dfsch_object_t* key;
  dfsch_object_t* value;
  DFSCH_OBJECT_ARG(args, hash);
  DFSCH_OBJECT_ARG(args, key);
  DFSCH_OBJECT_ARG(args, value);
  DFSCH_ARG_END(args);

  return dfsch_hash_set(hash, key, value);
}
static dfsch_object_t* native_hash_set_if_exists(void* baton, 
                                                 dfsch_object_t* args,
                                                 dfsch_tail_escape_t* esc){
  dfsch_object_t* hash;
  dfsch_object_t* key;
  dfsch_object_t* value;
  DFSCH_OBJECT_ARG(args, hash);
  DFSCH_OBJECT_ARG(args, key);
  DFSCH_OBJECT_ARG(args, value);
  DFSCH_ARG_END(args);

  return dfsch_bool(dfsch_hash_set_if_exists(hash, key, value));
}
static dfsch_object_t* native_hash_2_alist(void *baton, dfsch_object_t* args,
                                           dfsch_tail_escape_t* esc){
  dfsch_object_t* hash;

  DFSCH_OBJECT_ARG(args, hash);
  DFSCH_ARG_END(args);

  return dfsch_hash_2_alist(hash);
}

static dfsch_object_t* native_alist_2_hash(void *baton, dfsch_object_t* args,
                                           dfsch_tail_escape_t* esc){
  dfsch_object_t* alist;
  dfsch_object_t *mode;

  DFSCH_OBJECT_ARG(args, alist);
  DFSCH_OBJECT_ARG_OPT(args, mode, NULL);
  DFSCH_ARG_END(args);

  if (!mode)
    return dfsch_alist_2_hash(alist, DFSCH_HASH_EQ);
  if (mode == dfsch_make_symbol("equal?"))
    return dfsch_alist_2_hash(alist, DFSCH_HASH_EQUAL);
  if (mode == dfsch_make_symbol("eqv?"))
    return dfsch_alist_2_hash(alist, DFSCH_HASH_EQV);
  if (mode == dfsch_make_symbol("eq?"))
    return dfsch_alist_2_hash(alist, DFSCH_HASH_EQ);

  dfsch_error("exception:unknown-mode", mode);
}

DFSCH_DEFINE_FORM_IMPL(with_hash, dfsch_form_compiler_eval_all){
  dfsch_object_t *hash;
  dfsch_object_t *code;

  DFSCH_OBJECT_ARG(args, hash);
  DFSCH_ARG_REST(args, code);

  hash = dfsch_eval(hash, env);

  if (!dfsch_hash_p(hash)){
    dfsch_error("exception:not-a-hash", hash);
  }

  return dfsch_eval_proc_tr(code, dfsch_new_frame_from_hash(env, hash), 
                            NULL, esc);
}


void dfsch__hash_native_register(dfsch_object_t *ctx){
  dfsch_define_cstr(ctx, "<hash>", &hash_type);

  dfsch_define_cstr(ctx, "make-hash", 
                   dfsch_make_primitive(&native_make_hash,NULL));
  dfsch_define_cstr(ctx, "hash?", 
                   dfsch_make_primitive(&native_hash_p,NULL));
  dfsch_define_cstr(ctx, "hash-ref", 
                   dfsch_make_primitive(&native_hash_ref,NULL));
  dfsch_define_cstr(ctx, "hash-unset!", 
                   dfsch_make_primitive(&native_hash_unset,NULL));
  dfsch_define_cstr(ctx, "hash-set!", 
                   dfsch_make_primitive(&native_hash_set,NULL));
  dfsch_define_cstr(ctx, "hash-set-if-exists!", 
                   dfsch_make_primitive(&native_hash_set_if_exists,NULL));
  dfsch_define_cstr(ctx, "hash->alist", 
                   dfsch_make_primitive(&native_hash_2_alist,NULL));
  dfsch_define_cstr(ctx, "alist->hash", 
                   dfsch_make_primitive(&native_alist_2_hash,NULL));
  dfsch_define_cstr(ctx, "with-hash", DFSCH_FORM_REF(with_hash));

}
