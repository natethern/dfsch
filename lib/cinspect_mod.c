#include <dfsch/dfsch.h>
#include <dfsch/lib/cinspect.h>

dfsch_object_t* dfsch_module_cinspect_register(dfsch_object_t* env){
  dfsch_package_t* cinspect_pkg = dfsch_define_package("cinspect");
  dfsch_define_pkgcstr(env, cinspect_pkg, "debugger-procedure", dfsch_cinspect_get_procedure());
}