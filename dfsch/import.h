/*
 * dfsch_import - Library for loading scheme and C code into dfsch interpreter
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

#include <dfsch/dfsch.h>

#ifdef __cplusplus
extern "C" {
#endif

  extern int dfsch_import_so(dfsch_ctx_t* ctx, char* so_name);
  extern int dfsch_import_so_ex(dfsch_ctx_t* ctx, 
                                char* so_name, 
                                char* sym_name);
  extern int dfsch_import_scm(dfsch_ctx_t* ctx, char* scm_name);

  extern int dfsch_import(dfsch_ctx_t* ctx, char* name);

  extern void dfsch_import_register(dfsch_ctx_t *ctx);

#ifdef __cplusplus
}
#endif