#include <dfsch/lib/tk-gui.h>

DFSCH_DEFINE_PRIMITIVE(create_interpreter, 0){
  DFSCH_ARG_END(args);
  
  return dfsch_tcl_create_interpreter();
}

DFSCH_DEFINE_PRIMITIVE(eval_string, 0){
  Tcl_Interp* interp;
  char* code;

  DFSCH_TCL_INTERPRETER_ARG(args, interp);
  DFSCH_STRING_ARG(args, code);
  DFSCH_ARG_END(args);

  return dfsch_make_string_cstr(dfsch_tcl_eval(interp, code));
}

DFSCH_DEFINE_PRIMITIVE(eval, 0){
  Tcl_Interp* interp;

  DFSCH_TCL_INTERPRETER_ARG(args, interp);

  return dfsch_make_string_cstr(dfsch_tcl_eval(interp, 
                                               dfsch_tcl_quote_list(args)));
}
DFSCH_DEFINE_PRIMITIVE(eval_list, 0){
  Tcl_Interp* interp;
  dfsch_object_t* list;

  DFSCH_TCL_INTERPRETER_ARG(args, interp);
  DFSCH_OBJECT_ARG(args, list);
  DFSCH_ARG_END(args);

  return dfsch_make_string_cstr(dfsch_tcl_eval(interp, 
                                               dfsch_tcl_quote_list(list)));
}


DFSCH_DEFINE_PRIMITIVE(destroy_interpreter, 0){
  dfsch_object_t* interpreter;
  DFSCH_OBJECT_ARG(args, interpreter);
  DFSCH_ARG_END(args);
  
  dfsch_tcl_destroy_interpreter(interpreter);
  return NULL;
}
DFSCH_DEFINE_PRIMITIVE(wrap_command, 0){
  dfsch_object_t* interpreter;
  char* name;

  DFSCH_STRING_ARG(args, name);
  DFSCH_OBJECT_ARG_OPT(args, interpreter, NULL);
  DFSCH_ARG_END(args);

  return dfsch_tcl_wrap_command(name, interpreter);
}

DFSCH_DEFINE_PRIMITIVE(create_command, 0){
  Tcl_Interp* interp;
  char* name;
  dfsch_object_t* proc;

  DFSCH_TCL_INTERPRETER_ARG(args, interp);
  DFSCH_STRING_ARG(args, name);
  DFSCH_OBJECT_ARG(args, proc);
  DFSCH_ARG_END(args);
  
  dfsch_tcl_create_command(interp, name, proc);

  return NULL;
}
DFSCH_DEFINE_PRIMITIVE(delete_command, 0){
  Tcl_Interp* interp;
  char* name;

  DFSCH_TCL_INTERPRETER_ARG(args, interp);
  DFSCH_STRING_ARG(args, name);
  DFSCH_ARG_END(args);
  
  Tcl_DeleteCommand(interp, name);

  return NULL;
}
DFSCH_DEFINE_PRIMITIVE(event_loop, 0){
  DFSCH_ARG_END(args);

  dfsch_tcl_event_loop();
  return NULL;
}


void dfsch_module_tk_gui_interface_register(dfsch_object_t* env){
  dfsch_package_t* tk_gui = dfsch_make_package("tk-gui%interface");
  dfsch_provide(env, "tk-gui-interface");

  dfsch_define_pkgcstr(env, tk_gui, "<interpreter>", 
                       DFSCH_TCL_INTERPRETER_TYPE);
  dfsch_define_pkgcstr(env, tk_gui, "<command-wrapper>", 
                       DFSCH_TCL_COMMAND_WRAPPER_TYPE);

  dfsch_define_pkgcstr(env, tk_gui, "create-interpreter", 
                       DFSCH_PRIMITIVE_REF(create_interpreter));
  dfsch_define_pkgcstr(env, tk_gui, "destroy-interpreter", 
                       DFSCH_PRIMITIVE_REF(destroy_interpreter));
  dfsch_define_pkgcstr(env, tk_gui, "tcl-eval", 
                       DFSCH_PRIMITIVE_REF(eval));
  dfsch_define_pkgcstr(env, tk_gui, "tcl-eval-list", 
                       DFSCH_PRIMITIVE_REF(eval_list));
  dfsch_define_pkgcstr(env, tk_gui, "tcl-eval-string",
                       DFSCH_PRIMITIVE_REF(eval_string));
  dfsch_define_pkgcstr(env, tk_gui, "wrap-command", 
                       DFSCH_PRIMITIVE_REF(wrap_command));

  dfsch_define_pkgcstr(env, tk_gui, "create-command!", 
                       DFSCH_PRIMITIVE_REF(create_command));
  dfsch_define_pkgcstr(env, tk_gui, "delete-command!", 
                       DFSCH_PRIMITIVE_REF(delete_command));

  dfsch_define_pkgcstr(env, tk_gui, "event-loop", 
                       DFSCH_PRIMITIVE_REF(event_loop));

}
