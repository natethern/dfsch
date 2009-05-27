#include "dfsch/introspect.h"
#include <dfsch/magic.h>
#include "types.h"
#include "util.h"

#include <stdio.h>

static void safe_print_object();

char* dfsch_format_trace(dfsch_object_t* trace){
  return dfsch_object_2_string(trace, 100, 1);
}

void dfsch_print_trace_buffer(){
  int i;
  dfsch__thread_info_t* ti = dfsch__get_thread_info();

  for (i = 0; i <= ti->trace_depth; i++){
    if (i == ti->trace_ptr){
      fprintf(stderr, "> ");
    } else {
      fprintf(stderr, "  ");
    }
    switch (ti->trace_buffer[i].flags & 0xff){
    case DFSCH_TRACEPOINT_KIND_INVALID:
      fprintf(stderr, "0x00000000\n");
      break;
    case DFSCH_TRACEPOINT_KIND_APPLY:
      fprintf(stderr, "0x%08x %p (%s) %p (%s)\n", 
              ti->trace_buffer[i].flags,
              ti->trace_buffer[i].data.apply.proc,
              DFSCH_TYPE_OF(ti->trace_buffer[i].data.apply.proc)->name,
              ti->trace_buffer[i].data.apply.args,
              DFSCH_TYPE_OF(ti->trace_buffer[i].data.apply.args)->name);
      fprintf(stderr, "       %s\n     %s\n", 
              dfsch_object_2_string(ti->trace_buffer[i].data.apply.proc, 
                                    10, 1),
              dfsch_object_2_string(ti->trace_buffer[i].data.apply.args, 
                                    10, 1));
      break;
    case DFSCH_TRACEPOINT_KIND_EVAL:
      {
        dfsch_object_t* annot = 
          dfsch_get_list_annotation(ti->trace_buffer[i].data.eval.expr);
        fprintf(stderr, "0x%08x %p (%s) %p (%s)\n", 
                ti->trace_buffer[i].flags,
                ti->trace_buffer[i].data.eval.expr,
                DFSCH_TYPE_OF(ti->trace_buffer[i].data.eval.expr)->name,
                ti->trace_buffer[i].data.eval.env,
                DFSCH_TYPE_OF(ti->trace_buffer[i].data.eval.env)->name);
        fprintf(stderr, "     %s\n", 
                dfsch_object_2_string(ti->trace_buffer[i].data.apply.proc, 
                                      10, 1));
        if (annot){
          fprintf(stderr, "       @ %s:%s\n", 
                  dfsch_object_2_string(DFSCH_FAST_CAR(annot), 2, 0),
                  dfsch_object_2_string(DFSCH_FAST_CDR(annot), 1, 0));
          
        }
      }
      break;
    }
  }
}


dfsch_object_t* dfsch_get_trace(){
  int i;
  dfsch__thread_info_t* ti = dfsch__get_thread_info();
  dfsch_object_t* list = NULL;
  dfsch_object_t* record;
  i = ti->trace_ptr;

  do {
    if ((ti->trace_buffer[i].flags & 0xff) == DFSCH_TRACEPOINT_KIND_INVALID){
      break;
    }

    switch (ti->trace_buffer[i].flags & 0xff){
    case DFSCH_TRACEPOINT_KIND_APPLY:
      record = dfsch_list(3,
                          dfsch_make_symbol("apply"),
                          ti->trace_buffer[i].data.apply.proc,
                          ti->trace_buffer[i].data.apply.args);
      break;
    case DFSCH_TRACEPOINT_KIND_EVAL:
      record = dfsch_list(3,
                          dfsch_make_symbol("eval"),
                          ti->trace_buffer[i].data.eval.expr,
                          ti->trace_buffer[i].data.eval.env);
      break;
        
    default:
      record = DFSCH_MAKE_FIXNUM(ti->trace_buffer[i].flags);
      break;
    }

    list = dfsch_cons(record, list);

    i = (i - 1) & ti->trace_depth;
  } while (i != ti->trace_ptr);
  
  return list;
}


DFSCH_DEFINE_PRIMITIVE(set_debugger, 0){
  dfsch_object_t* proc;
  DFSCH_OBJECT_ARG(args, proc);
  DFSCH_ARG_END(args);
  
  dfsch_set_debugger(proc);
  return NULL;
}
DFSCH_DEFINE_PRIMITIVE(set_invoke_debugger_on_all_conditions, 0){
  dfsch_object_t* val;
  DFSCH_OBJECT_ARG(args, val);
  DFSCH_ARG_END(args);
  
  dfsch_set_invoke_debugger_on_all_conditions(val != NULL);
  return NULL;
}
DFSCH_DEFINE_PRIMITIVE(enter_debugger, 0){
  dfsch_object_t* reason;
  DFSCH_OBJECT_ARG(args, reason);
  DFSCH_ARG_END(args);
  
  dfsch_enter_debugger(reason);

  return NULL;
}

DFSCH_DEFINE_PRIMITIVE(lookup_in_environment, 0){
  dfsch_object_t* name;
  dfsch_object_t* env;
  DFSCH_OBJECT_ARG(args, name);
  DFSCH_OBJECT_ARG(args, env);
  DFSCH_ARG_END(args);

  return dfsch_lookup(name, env);
}
DFSCH_DEFINE_PRIMITIVE(set_in_environment, 0){
  dfsch_object_t* name;
  dfsch_object_t* env;
  dfsch_object_t* value;
  DFSCH_OBJECT_ARG(args, name);
  DFSCH_OBJECT_ARG(args, value);
  DFSCH_OBJECT_ARG(args, env);
  DFSCH_ARG_END(args);
  
  dfsch_set(name, value, env);
  return NULL;
}
DFSCH_DEFINE_PRIMITIVE(define_in_environment, 0){
  dfsch_object_t* name;
  dfsch_object_t* env;
  dfsch_object_t* value;
  DFSCH_OBJECT_ARG(args, name);
  DFSCH_OBJECT_ARG(args, value);
  DFSCH_OBJECT_ARG(args, env);
  DFSCH_ARG_END(args);
  
  dfsch_define(name, value, env);
  return NULL;
}
DFSCH_DEFINE_PRIMITIVE(unset_from_environment, 0){
  dfsch_object_t* name;
  dfsch_object_t* env;
  dfsch_object_t* value;
  DFSCH_OBJECT_ARG(args, name);
  DFSCH_OBJECT_ARG(args, env);
  DFSCH_ARG_END(args);
  
  dfsch_unset(name, env);
  return NULL;
}
DFSCH_DEFINE_PRIMITIVE(get_variables, 0){
  dfsch_object_t* env;
  DFSCH_OBJECT_ARG(args, env);
  DFSCH_ARG_END(args);
  
  return dfsch_get_environment_variables(env);
}

DFSCH_DEFINE_PRIMITIVE(load_into_environment, 0){
  dfsch_object_t* env;
  char* name;
  dfsch_object_t* path_list;
  DFSCH_OBJECT_ARG(args, env);
  DFSCH_STRING_OR_SYMBOL_ARG(args, name);
  DFSCH_OBJECT_ARG_OPT(args, path_list, NULL);
  DFSCH_ARG_END(args);
  
  dfsch_load(env, name, path_list);
  return NULL;
}

DFSCH_DEFINE_PRIMITIVE(make_environment, 0){
  dfsch_object_t* parent;
  DFSCH_OBJECT_ARG(args, parent);
  DFSCH_ARG_END(args);

  return dfsch_new_frame(parent);
}
DFSCH_DEFINE_PRIMITIVE(make_empty_environment, 0){
  DFSCH_ARG_END(args);

  return dfsch_new_frame(NULL);
}
DFSCH_DEFINE_PRIMITIVE(make_default_environment, 0){
  DFSCH_ARG_END(args);

  return dfsch_make_context();
}

void dfsch_introspect_register(dfsch_object_t* env){
  dfsch_provide(env, "introspect");

  dfsch_define_cstr(env, "set-invoke-debugger-on-all-conditions!", 
                    DFSCH_PRIMITIVE_REF(set_invoke_debugger_on_all_conditions));
  dfsch_define_cstr(env, "set-debugger!", DFSCH_PRIMITIVE_REF(set_debugger));
  dfsch_define_cstr(env, "enter-debugger", 
                    DFSCH_PRIMITIVE_REF(enter_debugger));

  dfsch_define_cstr(env, "lookup-in-environment",
                    DFSCH_PRIMITIVE_REF(lookup_in_environment));
  dfsch_define_cstr(env, "set-in-environment!",
                    DFSCH_PRIMITIVE_REF(set_in_environment));
  dfsch_define_cstr(env, "unset-from-environment!",
                    DFSCH_PRIMITIVE_REF(unset_from_environment));
  dfsch_define_cstr(env, "define-in-environment!",
                    DFSCH_PRIMITIVE_REF(define_in_environment));
  dfsch_define_cstr(env, "get-variables",
                    DFSCH_PRIMITIVE_REF(get_variables));
  dfsch_define_cstr(env, "load-into-environment!",
                    DFSCH_PRIMITIVE_REF(load_into_environment));

  dfsch_define_cstr(env, "make-environment",
                    DFSCH_PRIMITIVE_REF(make_environment));
  dfsch_define_cstr(env, "make-empty-environment",
                    DFSCH_PRIMITIVE_REF(make_empty_environment));
  dfsch_define_cstr(env, "make-default-environment",
                    DFSCH_PRIMITIVE_REF(make_default_environment));
}
