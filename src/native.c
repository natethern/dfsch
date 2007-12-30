/*
 * dfsch - dfox's quick and dirty scheme implementation
 *   Basic native functions
 * Copyright (C) 2005 Ales Hakl
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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "internal.h"
#include <dfsch/promise.h>
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <dfsch/number.h>
#include <dfsch/strings.h>

typedef dfsch_object_t object_t;

#define NEED_ARGS(args,count) \
  if (dfsch_list_length_check(args)!=(count)) \
    dfsch_error("exception:wrong-number-of-arguments",(args));
#define MIN_ARGS(args,count) \
  if (dfsch_list_length_check(args)<(count)) \
    dfsch_error("exception:too-few-arguments", (args));

// TODO: document all native functions somewhere

// Native procedures:

DFSCH_DEFINE_PRIMITIVE(gensym, 0){
  if (args)
    dfsch_error("exception:too-many-arguments", args);

  return dfsch_gensym();
}

DFSCH_DEFINE_PRIMITIVE(stack_trace, 0){
  if (args)
    dfsch_error("exception:too-many-arguments", args);

  return dfsch_get_stack_trace();
}


DFSCH_DEFINE_PRIMITIVE(unintern, 0){
  object_t* symbol;
  DFSCH_OBJECT_ARG(args, symbol);
  DFSCH_ARG_END(args);

  dfsch_unintern(symbol);
  return symbol;
}

DFSCH_DEFINE_PRIMITIVE(id, 0){
  object_t* object;
  DFSCH_OBJECT_ARG(args, object);
  DFSCH_ARG_END(args);

  return dfsch_make_number_from_long((long)object);
}
DFSCH_DEFINE_PRIMITIVE(hash, 0){
  object_t* object;
  DFSCH_OBJECT_ARG(args, object);
  DFSCH_ARG_END(args);

  return dfsch_make_number_from_long((long)dfsch_hash(object));
}
DFSCH_DEFINE_PRIMITIVE(type_of, 0){
  object_t* object;
  DFSCH_OBJECT_ARG(args, object);
  DFSCH_ARG_END(args);

  return (object_t*)DFSCH_TYPE_OF(object);
}
DFSCH_DEFINE_PRIMITIVE(superclass, 0){
  object_t* type;
  DFSCH_OBJECT_ARG(args, type);
  DFSCH_ARG_END(args);

  return dfsch_superclass(type);
}
DFSCH_DEFINE_PRIMITIVE(superclass_p, 0){
  dfsch_object_t* sub;
  dfsch_object_t* super;
  DFSCH_OBJECT_ARG(args, sub);
  DFSCH_OBJECT_ARG(args, super);
  DFSCH_ARG_END(args);

  if (!DFSCH_INSTANCE_P(sub, DFSCH_STANDARD_TYPE)){
    dfsch_error("excepiton:not-a-standard-type", sub);
  }

  if (super && !DFSCH_INSTANCE_P(super, DFSCH_STANDARD_TYPE)){
    dfsch_error("excepiton:not-a-standard-type", super);
  }

  return dfsch_bool(dfsch_superclass_p((dfsch_type_t*)sub, 
                                       (dfsch_type_t*)super));
}
DFSCH_DEFINE_PRIMITIVE(instance_p, 0){
  dfsch_object_t* object;
  dfsch_object_t* type;
  DFSCH_OBJECT_ARG(args, object);
  DFSCH_OBJECT_ARG(args, type);
  DFSCH_ARG_END(args);

  if (type && !DFSCH_INSTANCE_P(type, DFSCH_STANDARD_TYPE)){
    dfsch_error("excepiton:not-a-standard-type", type);
  }

  return dfsch_bool(dfsch_instance_p(object, (dfsch_type_t*)type));
}




/////////////////////////////////////////////////////////////////////////////
//
// Basic special forms
//
/////////////////////////////////////////////////////////////////////////////

static object_t* native_form_lambda(void *baton, object_t* args, dfsch_tail_escape_t* esc){

  MIN_ARGS(dfsch_cdr(args),1);

  return dfsch_lambda(dfsch_car(args),
		      dfsch_car(dfsch_cdr(args)),
		      dfsch_cdr(dfsch_cdr(args)));

}

static object_t* native_form_define(void *baton, object_t* args, dfsch_tail_escape_t* esc){

  MIN_ARGS(dfsch_cdr(args),1);  

  object_t* env = dfsch_car(args);
  object_t* name = dfsch_car(dfsch_cdr(args));

  if (dfsch_pair_p(name)){
    object_t* lambda = dfsch_named_lambda(env,dfsch_cdr(name),
                                          dfsch_cdr(dfsch_cdr(args)),
                                          dfsch_car(name));
    return dfsch_define(dfsch_car(name), lambda ,env);
  }else{
    object_t* value = dfsch_eval(dfsch_car(dfsch_cdr(dfsch_cdr(args))),env);
    return dfsch_define(name,value,env);
  }

}

static object_t* native_form_define_variable(void *baton, 
                                             object_t* args, 
                                             dfsch_tail_escape_t* esc){
  dfsch_object_t* env;
  dfsch_object_t* name;
  dfsch_object_t* value;

  DFSCH_OBJECT_ARG(args, env);
  DFSCH_OBJECT_ARG(args, name);
  DFSCH_OBJECT_ARG_OPT(args, value, NULL);
  
  if (!dfsch_env_get(name, env)){
    value = dfsch_eval(value, env);
    dfsch_define(name, value, env);
    return value;
  } else {
    return NULL;
  }
}


static object_t* native_form_set(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  
  NEED_ARGS(dfsch_cdr(args),2);  

  object_t* env = dfsch_car(args);
  object_t* name = dfsch_car(dfsch_cdr(args));
  object_t* value = dfsch_eval(dfsch_car(dfsch_cdr(dfsch_cdr(args))),env);

  return dfsch_set(name, value, env);

}
static object_t* native_form_unset(void *baton, object_t* args, 
                                 dfsch_tail_escape_t* esc){
  object_t* env;
  object_t* name;

  DFSCH_OBJECT_ARG(args, env);
  DFSCH_OBJECT_ARG(args, name);
  DFSCH_ARG_END(args);

  dfsch_unset(name, env);

  return NULL;
}
static object_t* native_form_defined_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(dfsch_cdr(args),1);
  object_t* env = dfsch_car(args);
  object_t* name = dfsch_car(dfsch_cdr(args));

  return dfsch_env_get(name, env);
}

static object_t* native_make_macro(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_make_macro(dfsch_car(args));
}


/////////////////////////////////////////////////////////////////////////////
//
// Pairs and lists
//
/////////////////////////////////////////////////////////////////////////////

static object_t* native_car(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_car(dfsch_car(args));
}
static object_t* native_cdr(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_cdr(dfsch_car(args));
}
static object_t* native_cons(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,2);  
  return dfsch_cons(dfsch_car(args),dfsch_car(dfsch_cdr(args)));
}
static object_t* native_list(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  return dfsch_list_copy(args);
}
static object_t* native_length(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  long len;
  NEED_ARGS(args,1);  

  len = dfsch_list_length(dfsch_car(args));

  if (len < 0)
    dfsch_error("exception:not-a-list", dfsch_car(args));
  
  return dfsch_make_number_from_long(len);
}
static object_t* native_set_car(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,2);  
  return dfsch_set_car(dfsch_car(args),dfsch_car(dfsch_cdr(args)));  
}
static object_t* native_set_cdr(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,2);  
  return dfsch_set_cdr(dfsch_car(args),dfsch_car(dfsch_cdr(args)));  
}
static object_t* native_zip(void* baton, object_t* args, 
			    dfsch_tail_escape_t* esc){
  return dfsch_zip(args);
}
static object_t* native_append(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  return dfsch_append(args);
}
static object_t* native_list_ref(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  int k;
  object_t* list;

  DFSCH_OBJECT_ARG(args, list);
  DFSCH_LONG_ARG(args, k);
  DFSCH_ARG_END(args);

  return dfsch_list_item(list, k);
}
static object_t* native_reverse(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* list;

  DFSCH_OBJECT_ARG(args, list);
  DFSCH_ARG_END(args);

  return dfsch_reverse(list);
}
static object_t* native_member(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* list;
  object_t* key;

  DFSCH_OBJECT_ARG(args, key);
  DFSCH_OBJECT_ARG(args, list);
  DFSCH_ARG_END(args);

  return dfsch_member(key, list);
}
static object_t* native_memv(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* list;
  object_t* key;

  DFSCH_OBJECT_ARG(args, key);
  DFSCH_OBJECT_ARG(args, list);
  DFSCH_ARG_END(args);

  return dfsch_memv(key, list);
}
static object_t* native_memq(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* list;
  object_t* key;

  DFSCH_OBJECT_ARG(args, key);
  DFSCH_OBJECT_ARG(args, list);
  DFSCH_ARG_END(args);

  return dfsch_memq(key, list);
}
static object_t* native_assoc(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* alist;
  object_t* key;

  DFSCH_OBJECT_ARG(args, key);
  DFSCH_OBJECT_ARG(args, alist);
  DFSCH_ARG_END(args);

  return dfsch_assoc(key, alist);
}
static object_t* native_assv(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* alist;
  object_t* key;

  DFSCH_OBJECT_ARG(args, key);
  DFSCH_OBJECT_ARG(args, alist);
  DFSCH_ARG_END(args);

  return dfsch_assv(key, alist);
}
static object_t* native_assq(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* alist;
  object_t* key;

  DFSCH_OBJECT_ARG(args, key);
  DFSCH_OBJECT_ARG(args, alist);
  DFSCH_ARG_END(args);

  return dfsch_assq(key, alist);
}
static object_t* native_for_each(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* func;
  object_t* list;
  size_t len;
  size_t i;

  DFSCH_OBJECT_ARG(args, func);
  list = dfsch_zip(args);

  if (!list){
    return NULL;
  }

  while (dfsch_pair_p(list)){
    dfsch_apply(func, dfsch_car(list));
    list = dfsch_cdr(list);
  }
  
  return NULL;
}
static object_t* native_map(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* func;
  object_t* list;
  object_t* head = NULL;
  object_t* tail;

  DFSCH_OBJECT_ARG(args, func);
  list = dfsch_zip(args);

  if (!list){
    return NULL;
  }

  while (dfsch_pair_p(list)){
    object_t *t = dfsch_cons(dfsch_apply(func, dfsch_car(list)), NULL);
    if (!head){
      head = tail = t;
    }else{
      dfsch_set_cdr(tail, t);
      tail = t;
    }
    list = dfsch_cdr(list);
  }
  
  return head;
}
static object_t* native_filter(void* baton, object_t* args, 
                               dfsch_tail_escape_t* esc){
  object_t* func;
  object_t* list;
  object_t* head = NULL;
  object_t* tail;

  DFSCH_OBJECT_ARG(args, func);
  DFSCH_OBJECT_ARG(args, list);
  DFSCH_ARG_END(args);

  while (dfsch_pair_p(list)){
    object_t* item =  dfsch_car(list);
    object_t* t;

    if (dfsch_apply(func, 
                    dfsch_list(1, item))){
      t = dfsch_cons(item, NULL);

      if (!head){
        head = tail = t;
      }else{
        dfsch_set_cdr(tail, t);
        tail = t;
      }
    }
    list = dfsch_cdr(list);
  }
  
  return head;
}
static object_t* native_reduce(void* baton, object_t* args, 
                               dfsch_tail_escape_t* esc){
  object_t* func;
  object_t* list;
  object_t* tally;

  DFSCH_OBJECT_ARG(args, func);
  DFSCH_OBJECT_ARG(args, list);
  DFSCH_ARG_END(args);

  if (!dfsch_pair_p(list)){
    return NULL;
  }

  tally = dfsch_car(list);
  list = dfsch_cdr(list);

  while (dfsch_pair_p(list)) {
    tally = dfsch_apply(func, dfsch_list(2, tally, dfsch_car(list)));
    list = dfsch_cdr(list);
  }
  
  return tally;
}



/////////////////////////////////////////////////////////////////////////////
//
// Type predicates
//
/////////////////////////////////////////////////////////////////////////////

static object_t* native_null_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_null_p(dfsch_car(args)));
}
static object_t* native_pair_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_pair_p(dfsch_car(args)));
}
static object_t* native_list_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_list_p(dfsch_car(args)));
}
static object_t* native_atom_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_atom_p(dfsch_car(args)));
}
static object_t* native_symbol_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_symbol_p(dfsch_car(args)));
}
static object_t* native_string_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_string_p(dfsch_car(args)));  
}
static object_t* native_primitive_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_primitive_p(dfsch_car(args))); 
}
static object_t* native_closure_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_closure_p(dfsch_car(args)));  
}
static object_t* native_procedure_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_procedure_p(dfsch_car(args)));  
}
static object_t* native_vector_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_vector_p(dfsch_car(args)));  
}
static object_t* native_macro_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_macro_p(dfsch_car(args)));  
}
static object_t* native_form_p(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_form_p(dfsch_car(args)));  
}

/////////////////////////////////////////////////////////////////////////////
//
// Equality predicates
//
/////////////////////////////////////////////////////////////////////////////

static object_t* native_eq(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,2);  
  return dfsch_bool(dfsch_eq_p(dfsch_car(args),dfsch_car(dfsch_cdr(args))));
}
static object_t* native_eqv(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,2);  
  return dfsch_bool(dfsch_eqv_p(dfsch_car(args),dfsch_car(dfsch_cdr(args))));
}
static object_t* native_equal(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,2);  
  return dfsch_bool(dfsch_equal_p(dfsch_car(args),dfsch_car(dfsch_cdr(args))));
}

/////////////////////////////////////////////////////////////////////////////
//
// Logic
//
/////////////////////////////////////////////////////////////////////////////


static object_t* native_form_or(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* env;
  object_t* i;
  object_t* r = NULL;
  MIN_ARGS(args, 1);
  env = dfsch_car(args); 
  i = dfsch_cdr(args);
 
  while(i){
    r = dfsch_eval(dfsch_car(i), env);
    if (r)
      return r;
    i = dfsch_cdr(i);
  }

  return r;
}
static object_t* native_form_and(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* env;
  object_t* i;
  object_t* r = dfsch_sym_true();
  MIN_ARGS(args, 1);
  env = dfsch_car(args); 
  i = dfsch_cdr(args);
 
  while(i){
    r = dfsch_eval(dfsch_car(i), env);
    if (!r)
      return r;

    i = dfsch_cdr(i);
  }

  return r;
}
static object_t* native_not(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  NEED_ARGS(args,1);  
  object_t *a = dfsch_car(args);
  return dfsch_bool(!a);
}

/////////////////////////////////////////////////////////////////////////////
//
// Vectors
//
/////////////////////////////////////////////////////////////////////////////


static object_t* native_make_vector(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  size_t length;
  object_t* fill;

  DFSCH_LONG_ARG(args, length);
  DFSCH_OBJECT_ARG_OPT(args, fill, NULL);
  DFSCH_ARG_END(args);

  return dfsch_make_vector(length,fill);
}

static object_t* native_vector(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  return dfsch_list_2_vector(args);
}
static object_t* native_vector_length(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* vector;
  
  DFSCH_OBJECT_ARG(args,vector);
  DFSCH_ARG_END(args);

  if (!dfsch_vector_p(vector))
    dfsch_error("exception:not-a-vector",vector);

  return dfsch_make_number_from_long(dfsch_vector_length(vector));

}
static object_t* native_vector_ref(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* vector;
  size_t k;

  DFSCH_OBJECT_ARG(args, vector);
  DFSCH_LONG_ARG(args, k);
  DFSCH_ARG_END(args);

  return dfsch_vector_ref(vector, k);
}

static object_t* native_vector_set(void* baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* vector;
  size_t k;
  object_t* obj;

  DFSCH_OBJECT_ARG(args, vector);
  DFSCH_LONG_ARG(args, k);
  DFSCH_OBJECT_ARG(args, obj);
  DFSCH_ARG_END(args);

  return dfsch_vector_set(vector, k, obj);
}

static object_t* native_vector_2_list(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* vector;

  DFSCH_OBJECT_ARG(args, vector);
  DFSCH_ARG_END(args);

  return dfsch_vector_2_list(vector);
}

static object_t* native_list_2_vector(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* list;

  DFSCH_OBJECT_ARG(args, list);
  DFSCH_ARG_END(args);

  return dfsch_list_2_vector(list);
}

/////////////////////////////////////////////////////////////////////////////
//
// Reading and writing strings
//
/////////////////////////////////////////////////////////////////////////////

static object_t* native_object_2_string(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* object;
  object_t* readable;
  long depth;

  DFSCH_OBJECT_ARG(args, object);
  DFSCH_LONG_ARG_OPT(args, depth, 256);
  DFSCH_OBJECT_ARG_OPT(args, readable, NULL);
  DFSCH_ARG_END(args);

  return dfsch_make_string_cstr(dfsch_obj_write(object, depth, 
                                                readable != NULL));
}
static object_t* native_string_2_object(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  char* string;

  DFSCH_STRING_ARG(args, string);
  DFSCH_ARG_END(args);

  return dfsch_obj_read(string);
}

/////////////////////////////////////////////////////////////////////////////
//
// Symbols
//
/////////////////////////////////////////////////////////////////////////////

static object_t* native_symbol_2_string(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  object_t* object;
  char* str;

  DFSCH_OBJECT_ARG(args, object);
  DFSCH_ARG_END(args);

  str = dfsch_symbol(object);
  if (str)
    return dfsch_make_string_cstr(str);
  else
    dfsch_error("exception:not-a-symbol", object);
}
static object_t* native_string_2_symbol(void *baton, object_t* args, dfsch_tail_escape_t* esc){
  char* string;

  DFSCH_STRING_ARG(args, string);
  DFSCH_ARG_END(args);

  return dfsch_make_symbol(string);
}

/////////////////////////////////////////////////////////////////////////////
//
// Registering function
//
/////////////////////////////////////////////////////////////////////////////

void dfsch__native_register(dfsch_object_t *ctx){ 
  dfsch_define_cstr(ctx, "gensym", &gensym);
  dfsch_define_cstr(ctx, "stack-trace", &stack_trace);
  dfsch_define_cstr(ctx, "unintern", &unintern);
  dfsch_define_cstr(ctx, "id", &id);
  dfsch_define_cstr(ctx, "hash", &hash);
  dfsch_define_cstr(ctx, "type-of", &type_of);
  dfsch_define_cstr(ctx, "superclass?", &superclass_p);
  dfsch_define_cstr(ctx, "instance?", &instance_p);
  dfsch_define_cstr(ctx, "superclass", &superclass);

  dfsch_define_cstr(ctx, "eq?", dfsch_make_primitive(&native_eq,NULL));
  dfsch_define_cstr(ctx, "eqv?", dfsch_make_primitive(&native_eqv,NULL));
  dfsch_define_cstr(ctx, "equal?", dfsch_make_primitive(&native_equal,NULL));

  dfsch_define_cstr(ctx, "and", 
                   dfsch_make_form(dfsch_make_primitive(&native_form_and,
                                                        NULL)));
  dfsch_define_cstr(ctx, "or", 
                   dfsch_make_form(dfsch_make_primitive(&native_form_or,
                                                        NULL)));
  dfsch_define_cstr(ctx, "not", dfsch_make_primitive(&native_not,NULL));

  dfsch_define_cstr(ctx, "lambda", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_lambda,
							NULL)));
  dfsch_define_cstr(ctx, "define", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_define,
							 NULL)));
  dfsch_define_cstr(ctx, "define-variable", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_define_variable,
							 NULL)));
  dfsch_define_cstr(ctx, "defined?", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_defined_p,
							 NULL)));
  dfsch_define_cstr(ctx, "set!", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_set,
							 NULL)));
  dfsch_define_cstr(ctx, "unset!", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_unset,
							 NULL)));

  dfsch_define_cstr(ctx, "make-macro", 
		   dfsch_make_primitive(&native_make_macro,NULL));
  dfsch_define_cstr(ctx, "cons", dfsch_make_primitive(&native_cons,NULL));
  dfsch_define_cstr(ctx, "list", dfsch_make_primitive(&native_list,NULL));
  dfsch_define_cstr(ctx, "car", dfsch_make_primitive(&native_car,NULL));
  dfsch_define_cstr(ctx, "cdr", dfsch_make_primitive(&native_cdr,NULL));
  dfsch_define_cstr(ctx, "set-car!", dfsch_make_primitive(&native_set_car,
							 NULL));
  dfsch_define_cstr(ctx, "set-cdr!", dfsch_make_primitive(&native_set_cdr,
							 NULL));

  dfsch_define_cstr(ctx, "length", dfsch_make_primitive(&native_length,NULL));
  dfsch_define_cstr(ctx, "zip", dfsch_make_primitive(&native_zip, NULL));
  dfsch_define_cstr(ctx, "append", dfsch_make_primitive(&native_append,NULL));
  dfsch_define_cstr(ctx, "for-each", dfsch_make_primitive(&native_for_each,
							 NULL));
  dfsch_define_cstr(ctx, "map", dfsch_make_primitive(&native_map,
							 NULL));
  dfsch_define_cstr(ctx, "filter", dfsch_make_primitive(&native_filter,
                                                       NULL));
  dfsch_define_cstr(ctx, "reduce", dfsch_make_primitive(&native_reduce,
                                                       NULL));
  dfsch_define_cstr(ctx, "list-ref", dfsch_make_primitive(&native_list_ref,
                                                         NULL));
  dfsch_define_cstr(ctx, "reverse", dfsch_make_primitive(&native_reverse,
                                                         NULL));
  dfsch_define_cstr(ctx, "member", dfsch_make_primitive(&native_member,NULL));
  dfsch_define_cstr(ctx, "memq", dfsch_make_primitive(&native_memq,NULL));
  dfsch_define_cstr(ctx, "memv", dfsch_make_primitive(&native_memv,NULL));
  dfsch_define_cstr(ctx, "assoc", dfsch_make_primitive(&native_assoc,NULL));
  dfsch_define_cstr(ctx, "assq", dfsch_make_primitive(&native_assq,NULL));
  dfsch_define_cstr(ctx, "assv", dfsch_make_primitive(&native_assv,NULL));

  dfsch_define_cstr(ctx, "null?", dfsch_make_primitive(&native_null_p,NULL));
  dfsch_define_cstr(ctx, "atom?", dfsch_make_primitive(&native_atom_p,NULL));
  dfsch_define_cstr(ctx, "pair?", dfsch_make_primitive(&native_pair_p,NULL));
  dfsch_define_cstr(ctx, "list?", dfsch_make_primitive(&native_list_p,NULL));
  dfsch_define_cstr(ctx, "symbol?", dfsch_make_primitive(&native_symbol_p,
							NULL));
  dfsch_define_cstr(ctx, "string?", dfsch_make_primitive(&native_string_p,
							NULL));
  dfsch_define_cstr(ctx, "primitive?", 
		   dfsch_make_primitive(&native_primitive_p,NULL));
  dfsch_define_cstr(ctx, "closure?", dfsch_make_primitive(&native_closure_p,
							 NULL));
  dfsch_define_cstr(ctx, "procedure?", 
		   dfsch_make_primitive(&native_procedure_p,NULL));
  dfsch_define_cstr(ctx, "macro?", dfsch_make_primitive(&native_macro_p,NULL));
  dfsch_define_cstr(ctx, "form?", dfsch_make_primitive(&native_form_p,NULL));
  dfsch_define_cstr(ctx, "vector?", dfsch_make_primitive(&native_vector_p,
                                                        NULL));


  dfsch_define_cstr(ctx, "true", dfsch_sym_true());
  dfsch_define_cstr(ctx, "nil", NULL);
  dfsch_define_cstr(ctx, "else", dfsch_sym_true());
  dfsch_define_cstr(ctx, "T", dfsch_sym_true());


  dfsch_define_cstr(ctx, "make-vector", 
                   dfsch_make_primitive(&native_make_vector,NULL));
  dfsch_define_cstr(ctx, "vector", 
                   dfsch_make_primitive(&native_vector,NULL));
  dfsch_define_cstr(ctx, "vector-length", 
                   dfsch_make_primitive(&native_vector_length,NULL));
  dfsch_define_cstr(ctx, "vector-set!", 
                   dfsch_make_primitive(&native_vector_set,NULL));
  dfsch_define_cstr(ctx, "vector-ref", 
                   dfsch_make_primitive(&native_vector_ref,NULL));
  dfsch_define_cstr(ctx, "vector->list", 
                   dfsch_make_primitive(&native_vector_2_list,NULL));
  dfsch_define_cstr(ctx, "list->vector", 
                   dfsch_make_primitive(&native_list_2_vector,NULL));

  dfsch_define_cstr(ctx, "object->string", 
                   dfsch_make_primitive(&native_object_2_string,NULL));
  dfsch_define_cstr(ctx, "string->object", 
                   dfsch_make_primitive(&native_string_2_object,NULL));

  dfsch_define_cstr(ctx, "symbol->string", 
                   dfsch_make_primitive(&native_symbol_2_string,NULL));
  dfsch_define_cstr(ctx, "string->symbol", 
                   dfsch_make_primitive(&native_string_2_symbol,NULL));

  dfsch__native_cxr_register(ctx);
  dfsch__control_register(ctx);
  dfsch__system_register(ctx);
  dfsch__generic_register(ctx);
  dfsch__hash_native_register(ctx);
  dfsch__promise_native_register(ctx);
  dfsch__number_native_register(ctx);
  dfsch__string_native_register(ctx);
  dfsch__wrapper_native_register(ctx);
  dfsch__object_native_register(ctx);
  dfsch__weak_native_register(ctx);
  dfsch__format_native_register(ctx);
  dfsch__port_native_register(ctx);

}
