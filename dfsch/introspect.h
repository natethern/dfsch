#ifndef H__dfsch__introspect__
#define H__dfsch__introspect__

#include <dfsch/dfsch.h>

void dfsch_print_trace_buffer();
dfsch_object_t* dfsch_get_trace();

void dfsch_introspect_register(dfsch_object_t* env);

void dfsch_set_inspector(dfsch_object_t* proc);
void dfsch_inspect_object(dfsch_object_t* obj);
dfsch_object_t* dfsch_describe_object(dfsch_object_t* obj);

dfsch_object_t* dfsch_find_source_annotation(dfsch_object_t* list);

#endif
