/*
 * dfsch - dfox's quick and dirty scheme implementation
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

/** @file dfsch.c This is implementation of dfsch interpreter. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "native.h"

#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <gc/gc.h>

typedef dfsch_object_t object_t;

#define NEED_ARGS(args,count) \
  if (dfsch_list_length(args)!=(count)) \
    DFSCH_THROW("exception:wrong-number-of-arguments",(args));
#define MIN_ARGS(args,count) \
  if (dfsch_list_length(args)<(count)) \
    DFSCH_THROW("exception:too-few-arguments", (args));
#define EXCEPTION_CHECK(x) {if (dfsch_object_exception_p(x)) return x;}

// Native procedures:

/////////////////////////////////////////////////////////////////////////////
//
// Arithmetic on numbers
//
/////////////////////////////////////////////////////////////////////////////


static object_t* native_plus(void *baton, object_t* args){
  object_t* i = args;
  double s=0;
  while(dfsch_object_pair_p(i)){
    
    if (dfsch_object_number_p(dfsch_car(i))){
      s+=dfsch_number(dfsch_car(i));
    }else{
      DFSCH_THROW("exception:not-a-number", dfsch_car(i));
      
    }
    i = dfsch_cdr(i);
  }

  return dfsch_make_number(s); 
}
static object_t* native_minus(void *baton, object_t* args){
  object_t* i = args;
  double s;
  if (!dfsch_object_pair_p(i))
    DFSCH_THROW("exception:too-few-arguments",i);

  if (dfsch_object_number_p(dfsch_car(i))){
    s=dfsch_number(dfsch_car(i));
  }else{
    DFSCH_THROW("exception:not-a-number", dfsch_car(i));
    
  }
  i = dfsch_cdr(i);
  while(dfsch_object_pair_p(i)){
    if (dfsch_object_number_p(dfsch_car(i))){
      s-=dfsch_number(dfsch_car(i));
    }else{
      DFSCH_THROW("exception:not-a-number", dfsch_car(i));
      
    }
    i = dfsch_cdr(i);
  }

  return dfsch_make_number(s); 
}
static object_t* native_mult(void *baton, object_t* args){
  object_t* i = args;
  double s=1;
  while(dfsch_object_pair_p(i)){
    if (dfsch_object_number_p(dfsch_car(i))){
      s*=dfsch_number(dfsch_car(i));
    }else{
      DFSCH_THROW("exception:not-a-number", dfsch_car(i));
      
    }
    i = dfsch_cdr(i);
  }

  return dfsch_make_number(s); 
}
static object_t* native_slash(void *baton, object_t* args){
  object_t* i = args;
  double s;
  if (!dfsch_object_pair_p(i))
    DFSCH_THROW("exception:too-few-arguments",i);

  if (dfsch_object_number_p(dfsch_car(i))){
    s=dfsch_number(dfsch_car(i));
  }else{
    DFSCH_THROW("exception:not-a-number", dfsch_car(i));
    
  }
  i = dfsch_cdr(i);
  
  while(dfsch_object_pair_p(i)){
    if (dfsch_object_number_p(i)){
      s/=dfsch_number(dfsch_car(i));
    }else{
      DFSCH_THROW("exception:not-a-number", dfsch_car(i));
      
    }
    i = dfsch_cdr(i);
  }

  return dfsch_make_number(s); 
}

/////////////////////////////////////////////////////////////////////////////
//
// Basic special forms
//
/////////////////////////////////////////////////////////////////////////////

static object_t* native_form_lambda(void *baton, object_t* args){

  MIN_ARGS(dfsch_cdr(args),1);

  return dfsch_lambda(dfsch_car(args),
		      dfsch_car(dfsch_cdr(args)),
		      dfsch_cdr(dfsch_cdr(args)));

}

static object_t* native_form_define(void *baton, object_t* args){

  MIN_ARGS(dfsch_cdr(args),1);  

  object_t* env = dfsch_car(args);
  object_t* name = dfsch_car(dfsch_cdr(args));

  if (dfsch_object_pair_p(name)){
    object_t* lambda = dfsch_named_lambda(env,dfsch_cdr(name),
                                          dfsch_cdr(dfsch_cdr(args)),
                                          dfsch_car(name));
    return dfsch_define(dfsch_car(name), lambda ,env);
  }else{
    object_t* value = dfsch_eval(dfsch_car(dfsch_cdr(dfsch_cdr(args))),env);
    EXCEPTION_CHECK(value);
    return dfsch_define(name,value,env);
  }

}
static object_t* native_form_set(void *baton, object_t* args){
  
  NEED_ARGS(dfsch_cdr(args),2);  

  object_t* env = dfsch_car(args);
  object_t* name = dfsch_car(dfsch_cdr(args));
  object_t* value = dfsch_eval(dfsch_car(dfsch_cdr(dfsch_cdr(args))),env);

  EXCEPTION_CHECK(value);

  return dfsch_set(name, value, env);

}
static object_t* native_form_defined_p(void *baton, object_t* args){
  NEED_ARGS(dfsch_cdr(args),1);
  object_t* env = dfsch_car(args);
  object_t* name = dfsch_car(dfsch_cdr(args));

  return dfsch_bool(dfsch_object_exception_p(dfsch_lookup(name,env)));
}

static object_t* native_macro_if(void *baton, object_t* args){

  NEED_ARGS(dfsch_cdr(args),3);    
  object_t* env = dfsch_car(args);
  object_t* cond = dfsch_car(dfsch_cdr(args));
  object_t* true = dfsch_car(dfsch_cdr(dfsch_cdr(args)));
  object_t* false = dfsch_car(dfsch_cdr(dfsch_cdr(dfsch_cdr(args))));

  EXCEPTION_CHECK(cond);

  return dfsch_cons(dfsch_eval(cond,env)?true:false, NULL);
}

static object_t* native_macro_cond(void *baton, object_t* args){
  

  object_t* env = dfsch_car(args);
  object_t* i = dfsch_cdr(args);

  while (dfsch_object_pair_p(i)){
    object_t *o = dfsch_eval(dfsch_car(dfsch_car(i)), env);
    EXCEPTION_CHECK(o);
    if (o){
      return dfsch_cdr(dfsch_car(i));
    }
    
    i = dfsch_cdr(i); 
  }

  return NULL;
}


static object_t* native_form_quote(void *baton, object_t* args){
  NEED_ARGS(dfsch_cdr(args),1);  
  return dfsch_car(dfsch_cdr(args));
}

static object_t* native_form_quasiquote(void *baton, object_t* args){
  object_t* env;
  object_t* arg;
  DFSCH_OBJECT_ARG(args, env);
  DFSCH_OBJECT_ARG(args, arg);

  return dfsch_quasiquote(env,arg);
}

static object_t* native_macro_begin(void *baton, object_t* args){
  return dfsch_cdr(args);
}
static object_t* native_form_let(void *baton, object_t* args){
  MIN_ARGS(args,2);

  object_t *env = dfsch_car(args);
  object_t *vars = dfsch_car(dfsch_cdr(args));
  object_t *code = dfsch_cdr(dfsch_cdr(args));

  object_t* ext = NULL;

  while (dfsch_object_pair_p(vars)){
    object_t* var = dfsch_list_item(dfsch_car(vars),0);
    object_t* exp = dfsch_list_item(dfsch_car(vars),1);

    ext = dfsch_cons(dfsch_cons(var,
                                dfsch_cons(dfsch_eval(exp,env),
                                           NULL)),
                     ext);
    
    vars = dfsch_cdr(vars);
  }

  return dfsch_eval_proc(code,dfsch_cons(ext,env));
}



static object_t* native_make_form(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_make_form(dfsch_car(args));
}
static object_t* native_make_macro(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_make_macro(dfsch_car(args));
}

/////////////////////////////////////////////////////////////////////////////
//
// EVAL + APPLY
//
/////////////////////////////////////////////////////////////////////////////

static object_t* native_eval(void *baton, object_t* args){
  NEED_ARGS(args,2);  
  return dfsch_eval(dfsch_car(args),dfsch_car(dfsch_cdr(args)));
}
static object_t* native_apply(void *baton, object_t* args){
  NEED_ARGS(args,2);  
  return dfsch_apply(dfsch_car(args),dfsch_car(dfsch_cdr(args)));
}

/////////////////////////////////////////////////////////////////////////////
//
// Pairs and lists
//
/////////////////////////////////////////////////////////////////////////////

static object_t* native_car(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_car(dfsch_car(args));
}
static object_t* native_cdr(void *baton, object_t* args){
  NEED_ARGS(args,1);  
return dfsch_cdr(dfsch_car(args));
}
static object_t* native_cons(void *baton, object_t* args){
  NEED_ARGS(args,2);  
  return dfsch_cons(dfsch_car(args),dfsch_car(dfsch_cdr(args)));
}
static object_t* native_list(void *baton, object_t* args){
  return dfsch_list_copy(args);
}
static object_t* native_length(void *baton, object_t* args){
  NEED_ARGS(args,1);  

  return dfsch_make_number((double)dfsch_list_length(args));
}
static object_t* native_set_car(void *baton, object_t* args){
  NEED_ARGS(args,2);  
  return dfsch_set_car(dfsch_car(args),dfsch_car(dfsch_cdr(args)));  
}
static object_t* native_set_cdr(void *baton, object_t* args){
  NEED_ARGS(args,2);  
  return dfsch_set_cdr(dfsch_car(args),dfsch_car(dfsch_cdr(args)));  
}
static object_t* native_append(void* baton, object_t* args){
  return dfsch_append(args);
}
static object_t* native_list_ref(void* baton, object_t* args){
  int k;
  object_t* list;

  DFSCH_OBJECT_ARG(args, list);
  DFSCH_NUMBER_ARG(args, k, int);

  return dfsch_list_item(list, k);
}

/////////////////////////////////////////////////////////////////////////////
//
// Type predicates
//
/////////////////////////////////////////////////////////////////////////////

static object_t* native_null_p(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_object_null_p(dfsch_car(args)));
}
static object_t* native_pair_p(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_object_pair_p(dfsch_car(args)));
}
static object_t* native_atom_p(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_object_atom_p(dfsch_car(args)));
}
static object_t* native_symbol_p(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_object_symbol_p(dfsch_car(args)));
}
static object_t* native_number_p(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_object_number_p(dfsch_car(args)));  
}
static object_t* native_string_p(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_object_string_p(dfsch_car(args)));  
}
static object_t* native_primitive_p(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_object_primitive_p(dfsch_car(args))); 
}
static object_t* native_closure_p(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_object_closure_p(dfsch_car(args)));  
}
static object_t* native_procedure_p(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_object_procedure_p(dfsch_car(args)));  
}
static object_t* native_vector_p(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_object_vector_p(dfsch_car(args)));  
}
static object_t* native_macro_p(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  return dfsch_bool(dfsch_object_macro_p(dfsch_car(args)));  
}

/////////////////////////////////////////////////////////////////////////////
//
// Comparisons
//
/////////////////////////////////////////////////////////////////////////////

static object_t* native_eq(void *baton, object_t* args){
  NEED_ARGS(args,2);  
  return dfsch_bool(dfsch_eq_p(dfsch_car(args),dfsch_car(dfsch_cdr(args))));
}
static object_t* native_lt(void *baton, object_t* args){
  NEED_ARGS(args,2);  
  object_t *a = dfsch_car(args);
  object_t *b = dfsch_car(dfsch_cdr(args));
  if (!dfsch_object_number_p(a))
    DFSCH_THROW("exception:not-a-number", a);
  if (!dfsch_object_number_p(b))
    DFSCH_THROW("exception:not-a-number", b);

  return dfsch_bool(dfsch_number(a)<dfsch_number(b));  
}
static object_t* native_gt(void *baton, object_t* args){
  NEED_ARGS(args,2);  
  object_t *a = dfsch_car(args);
  object_t *b = dfsch_car(dfsch_cdr(args));
  if (!dfsch_object_number_p(a))
    DFSCH_THROW("exception:not-a-number", a);
  if (!dfsch_object_number_p(b))
    DFSCH_THROW("exception:not-a-number", b);
    

  return dfsch_bool(dfsch_number(a)>dfsch_number(b));  
}
static object_t* native_lte(void *baton, object_t* args){
  NEED_ARGS(args,2);  
  object_t *a = dfsch_car(args);
  object_t *b = dfsch_car(dfsch_cdr(args));
  if (!dfsch_object_number_p(a))
    DFSCH_THROW("exception:not-a-number", a);
  if (!dfsch_object_number_p(b))
    DFSCH_THROW("exception:not-a-number", b);

  return dfsch_bool(dfsch_number(a)<=dfsch_number(b));  
}
static object_t* native_gte(void *baton, object_t* args){
  NEED_ARGS(args,2);  
  object_t *a = dfsch_car(args);
  object_t *b = dfsch_car(dfsch_cdr(args));
  if (!dfsch_object_number_p(a))
    DFSCH_THROW("exception:not-a-number", a);
  if (!dfsch_object_number_p(b))
    DFSCH_THROW("exception:not-a-number", b);
    

  return dfsch_bool(dfsch_number(a)>=dfsch_number(b));  
}

/////////////////////////////////////////////////////////////////////////////
//
// Logic
//
/////////////////////////////////////////////////////////////////////////////


static object_t* native_or(void *baton, object_t* args){
  /*  NEED_ARGS(args,2);  
  object_t *a = dfsch_car(args);
  object_t *b = dfsch_car(dfsch_cdr(args));
  return (a||b)?
    dfsch_true():
    NULL;  */ // TODO
}
static object_t* native_and(void *baton, object_t* args){
  /*  NEED_ARGS(args,2);  
  object_t *a = dfsch_car(args);
  object_t *b = dfsch_car(dfsch_cdr(args));
  return (a&&b)?
    dfsch_true():
    NULL;  */ //TODO
}
static object_t* native_not(void *baton, object_t* args){
  NEED_ARGS(args,1);  
  object_t *a = dfsch_car(args);
  return dfsch_bool(!a);
}

/////////////////////////////////////////////////////////////////////////////
//
// Exception handling
//
/////////////////////////////////////////////////////////////////////////////


static object_t* native_throw(void *baton, object_t* args){
  NEED_ARGS(args,2);  
  return dfsch_make_exception(dfsch_car(args),dfsch_car(dfsch_cdr(args)));
}
static object_t* native_form_try(void *baton, object_t* args){
  NEED_ARGS(args,3);  
  object_t *env = dfsch_car(args);
  object_t *value = dfsch_eval(dfsch_car(dfsch_cdr(args)),env);
  object_t *except = dfsch_eval(dfsch_car(dfsch_cdr(dfsch_cdr(args))),env);

  EXCEPTION_CHECK(except);

  return dfsch_object_exception_p(value)
    ?dfsch_apply(except,dfsch_list(3,
                                   dfsch_exception_type(value),
                                   dfsch_exception_data(value),
                                   dfsch_exception_trace(value)))
    :value;
  
}

/////////////////////////////////////////////////////////////////////////////
//
// Strings
//
/////////////////////////////////////////////////////////////////////////////


static object_t* native_string_append(void *baton, object_t* args){
  NEED_ARGS(args,2);
  object_t* a = dfsch_car(args);
  object_t* b = dfsch_car(dfsch_cdr(args));
  char *s;

  if (!dfsch_object_string_p(a))
    DFSCH_THROW("exception:not-a-string", a);
  if (!dfsch_object_string_p(b))
    DFSCH_THROW("exception:not-a-string", b);

  s = stracat(dfsch_string(a),dfsch_string(b));

  object_t* o = dfsch_make_string(s); 
  return o;
}
static object_t* native_string_ref(void *baton, object_t* args){
  NEED_ARGS(args,2);
  object_t* a = dfsch_car(args);
  object_t* b = dfsch_car(dfsch_cdr(args));

  if (!dfsch_object_string_p(a))
    DFSCH_THROW("exception:not-a-string", a);

  char *s = dfsch_string(a);
  size_t len = strlen(s);
  size_t index = (size_t)(dfsch_number(b));
  
  if (index < 0)
    index = index + len;
  if (index>=len)
    DFSCH_THROW("exception:index-too-large", b);



  return dfsch_make_number((double)s[index]);
}
static object_t* native_string_length(void *baton, object_t* args){
  NEED_ARGS(args,1);

  object_t* a = dfsch_car(args);
  if (!dfsch_object_string_p(a))
    DFSCH_THROW("exception:not-a-string", a);

  return dfsch_make_number((double)strlen(dfsch_string(a)));
}

/////////////////////////////////////////////////////////////////////////////
//
// Vectors
//
/////////////////////////////////////////////////////////////////////////////


static object_t* native_make_vector(void* baton, object_t* args){
  size_t length;
  object_t* fill;

  DFSCH_NUMBER_ARG(args, length, size_t);
  DFSCH_OBJECT_ARG_OPT(args, fill, NULL);
  DFSCH_ARG_END(args);

  return dfsch_make_vector(length,fill);
}

static object_t* native_vector(void* baton, object_t* args){
  return dfsch_list_2_vector(args);
}
static object_t* native_vector_length(void* baton, object_t* args){
  object_t* vector;
  
  DFSCH_OBJECT_ARG(args,vector);
  DFSCH_ARG_END(args);

  if (!dfsch_object_vector_p)
    DFSCH_THROW("exception:not-a-vector",vector);

  return dfsch_make_number(dfsch_vector_length(vector));

}
static object_t* native_vector_ref(void* baton, object_t* args){
  object_t* vector;
  size_t k;

  DFSCH_OBJECT_ARG(args, vector);
  DFSCH_NUMBER_ARG(args, k, size_t);
  DFSCH_ARG_END(args);

  return dfsch_vector_ref(vector, k);
}

static object_t* native_vector_set(void* baton, object_t* args){
  object_t* vector;
  size_t k;
  object_t* obj;

  DFSCH_OBJECT_ARG(args, vector);
  DFSCH_NUMBER_ARG(args, k, size_t);
  DFSCH_OBJECT_ARG(args, obj);
  DFSCH_ARG_END(args);

  return dfsch_vector_set(vector, k, obj);
}

static object_t* native_vector_2_list(void *baton, object_t* args){
  object_t* vector;

  DFSCH_OBJECT_ARG(args, vector);
  DFSCH_ARG_END(args);

  return dfsch_vector_2_list(vector);
}

static object_t* native_list_2_vector(void *baton, object_t* args){
  object_t* list;

  DFSCH_OBJECT_ARG(args, list);
  DFSCH_ARG_END(args);

  return dfsch_list_2_vector(list);
}


/////////////////////////////////////////////////////////////////////////////
//
// Hash operations
//
/////////////////////////////////////////////////////////////////////////////


object_t* native_make_hash(void* baton, object_t* args){
  object_t* proc;
  DFSCH_OBJECT_ARG_OPT(args, proc, NULL);
  DFSCH_ARG_END(args);

  return dfsch_hash_make(proc);
}
object_t* native_hash_p(void* baton, object_t* args){
  object_t* obj;
  DFSCH_OBJECT_ARG(args, obj);
  DFSCH_ARG_END(args);

  return dfsch_bool(dfsch_hash_p(obj));
}
object_t* native_hash_ref(void* baton, object_t* args){
  object_t* hash;
  object_t* key;
  DFSCH_OBJECT_ARG(args, hash);
  DFSCH_OBJECT_ARG(args, key);
  DFSCH_ARG_END(args);

  return dfsch_hash_ref(hash, key);
}
object_t* native_hash_set(void* baton, object_t* args){
  object_t* hash;
  object_t* key;
  object_t* value;
  DFSCH_OBJECT_ARG(args, hash);
  DFSCH_OBJECT_ARG(args, key);
  DFSCH_OBJECT_ARG(args, value);
  DFSCH_ARG_END(args);

  return dfsch_hash_set(hash, key, value);
}


/////////////////////////////////////////////////////////////////////////////
//
// Registering function
//
/////////////////////////////////////////////////////////////////////////////

dfsch_object_t* dfsch_native_register(dfsch_ctx_t *ctx){ 
  dfsch_ctx_define(ctx, "+", dfsch_make_primitive(&native_plus,NULL));
  dfsch_ctx_define(ctx, "-", dfsch_make_primitive(&native_minus,NULL));
  dfsch_ctx_define(ctx, "*", dfsch_make_primitive(&native_mult,NULL));
  dfsch_ctx_define(ctx, "/", dfsch_make_primitive(&native_slash,NULL));
  dfsch_ctx_define(ctx, "=", dfsch_make_primitive(&native_eq,NULL));
  dfsch_ctx_define(ctx, "<", dfsch_make_primitive(&native_lt,NULL));
  dfsch_ctx_define(ctx, ">", dfsch_make_primitive(&native_gt,NULL));
  dfsch_ctx_define(ctx, "<=", dfsch_make_primitive(&native_lte,NULL));
  dfsch_ctx_define(ctx, ">=", dfsch_make_primitive(&native_gte,NULL));
  dfsch_ctx_define(ctx, "and", dfsch_make_primitive(&native_and,NULL));
  dfsch_ctx_define(ctx, "or", dfsch_make_primitive(&native_or,NULL));
  dfsch_ctx_define(ctx, "not", dfsch_make_primitive(&native_not,NULL));

  dfsch_ctx_define(ctx, "lambda", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_lambda,
							 NULL)));
  dfsch_ctx_define(ctx, "define", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_define,
							 NULL)));
  dfsch_ctx_define(ctx, "defined?", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_defined_p,
							 NULL)));
  dfsch_ctx_define(ctx, "begin", 
		   dfsch_make_macro(dfsch_make_primitive(&native_macro_begin,
							 NULL)));
  dfsch_ctx_define(ctx, "let", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_let,
							 NULL)));

  dfsch_ctx_define(ctx, "set!", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_set,
							 NULL)));
  dfsch_ctx_define(ctx, "quasiquote", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_quasiquote,
							 NULL)));
  dfsch_ctx_define(ctx, "quote", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_quote,
							 NULL)));
  dfsch_ctx_define(ctx, "if", 
		   dfsch_make_macro(dfsch_make_primitive(&native_macro_if,
							      NULL)));
  dfsch_ctx_define(ctx, "cond", 
		   dfsch_make_macro(dfsch_make_primitive(&native_macro_cond,
							      NULL)));

  dfsch_ctx_define(ctx, "make-form", 
		   dfsch_make_primitive(&native_make_form,NULL));
  dfsch_ctx_define(ctx, "make-macro", 
		   dfsch_make_primitive(&native_make_macro,NULL));
  dfsch_ctx_define(ctx, "cons", dfsch_make_primitive(&native_cons,NULL));
  dfsch_ctx_define(ctx, "list", dfsch_make_primitive(&native_list,NULL));
  dfsch_ctx_define(ctx, "car", dfsch_make_primitive(&native_car,NULL));
  dfsch_ctx_define(ctx, "cdr", dfsch_make_primitive(&native_cdr,NULL));
  dfsch_ctx_define(ctx, "set-car!", dfsch_make_primitive(&native_set_car,
							 NULL));
  dfsch_ctx_define(ctx, "set-cdr!", dfsch_make_primitive(&native_set_cdr,
							 NULL));

  dfsch_ctx_define(ctx, "length", dfsch_make_primitive(&native_length,NULL));
  dfsch_ctx_define(ctx, "append", dfsch_make_primitive(&native_append,NULL));

  dfsch_ctx_define(ctx, "null?", dfsch_make_primitive(&native_null_p,NULL));
  dfsch_ctx_define(ctx, "atom?", dfsch_make_primitive(&native_atom_p,NULL));
  dfsch_ctx_define(ctx, "pair?", dfsch_make_primitive(&native_pair_p,NULL));
  dfsch_ctx_define(ctx, "symbol?", dfsch_make_primitive(&native_symbol_p,
							NULL));
  dfsch_ctx_define(ctx, "number?", dfsch_make_primitive(&native_number_p,
							NULL));
  dfsch_ctx_define(ctx, "string?", dfsch_make_primitive(&native_string_p,
							NULL));
  dfsch_ctx_define(ctx, "primitive?", 
		   dfsch_make_primitive(&native_primitive_p,NULL));
  dfsch_ctx_define(ctx, "closure?", dfsch_make_primitive(&native_closure_p,
							 NULL));
  dfsch_ctx_define(ctx, "procedure?", 
		   dfsch_make_primitive(&native_procedure_p,NULL));
  dfsch_ctx_define(ctx, "macro?", dfsch_make_primitive(&native_macro_p,NULL));
  dfsch_ctx_define(ctx, "vector?", dfsch_make_primitive(&native_vector_p,
                                                        NULL));


  dfsch_ctx_define(ctx, "throw", 
		   dfsch_make_primitive(&native_throw,NULL));
  dfsch_ctx_define(ctx, "try", 
		   dfsch_make_form(dfsch_make_primitive(&native_form_try,
							 NULL)));

  dfsch_ctx_define(ctx, "string-append", 
		   dfsch_make_primitive(&native_string_append,NULL));
  dfsch_ctx_define(ctx, "string-ref", 
		   dfsch_make_primitive(&native_string_ref,NULL));
  dfsch_ctx_define(ctx, "string-length", 
		   dfsch_make_primitive(&native_string_length,NULL));

  dfsch_ctx_define(ctx, "true", dfsch_sym_true());
  dfsch_ctx_define(ctx, "pi", dfsch_make_number(3.1415926535897931));
  dfsch_ctx_define(ctx, "nil", NULL);
  dfsch_ctx_define(ctx, "else", dfsch_sym_true());
  dfsch_ctx_define(ctx, "T", dfsch_sym_true());

  dfsch_ctx_define(ctx, "eval", dfsch_make_primitive(&native_eval,NULL));
  dfsch_ctx_define(ctx, "apply", dfsch_make_primitive(&native_apply,NULL));

  dfsch_ctx_define(ctx, "make-vector", 
                   dfsch_make_primitive(&native_make_vector,NULL));
  dfsch_ctx_define(ctx, "vector", 
                   dfsch_make_primitive(&native_vector,NULL));
  dfsch_ctx_define(ctx, "vector-length", 
                   dfsch_make_primitive(&native_vector_length,NULL));
  dfsch_ctx_define(ctx, "vector-set!", 
                   dfsch_make_primitive(&native_vector_set,NULL));
  dfsch_ctx_define(ctx, "vector-ref", 
                   dfsch_make_primitive(&native_vector_ref,NULL));
  dfsch_ctx_define(ctx, "vector->list", 
                   dfsch_make_primitive(&native_vector_2_list,NULL));
  dfsch_ctx_define(ctx, "list->vector", 
                   dfsch_make_primitive(&native_list_2_vector,NULL));


  dfsch_ctx_define(ctx, "make-hash", 
                   dfsch_make_primitive(&native_make_hash,NULL));
  dfsch_ctx_define(ctx, "hash?", 
                   dfsch_make_primitive(&native_hash_p,NULL));
  dfsch_ctx_define(ctx, "hash-ref", 
                   dfsch_make_primitive(&native_hash_ref,NULL));
  dfsch_ctx_define(ctx, "hash-set!", 
                   dfsch_make_primitive(&native_hash_set,NULL));


  return NULL;
}
