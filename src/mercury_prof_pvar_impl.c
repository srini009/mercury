/*
 * Copyright (C) 2013-2020 Argonne National Laboratory, Department of Energy,
 *                    UChicago Argonne, LLC and The HDF Group.
 * All rights reserved.
 *
 * The full copyright notice, including terms governing use, modification,
 * and redistribution, is contained in the COPYING file that can be
 * found at the root of the source code distribution tree.
 */

#include "mercury_bulk.h"
#include "mercury_core.h"
#include "mercury_private.h"
#include "mercury_error.h"

#include "mercury_atomic.h"
#include "mercury_prof_interface.h"
#include "mercury_prof_pvar_impl.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*******************/
/* Local Variables */
/*******************/

static hg_hash_table_t *pvar_table; /* Internal hash table containing all PVAR info */

/* Internal routines for the hash tables */
static HG_INLINE int
hg_prof_uint_equal(void *vlocation1, void *vlocation2)
{
    return *((unsigned int *) vlocation1) == *((unsigned int *) vlocation2);
}

/*---------------------------------------------------------------------------*/
static HG_INLINE unsigned int
hg_prof_uint_hash(void *vlocation)
{
    return *((unsigned int *) vlocation);
}

/*---------------------------------------------------------------------------*/
hg_prof_pvar_data_t *
hg_prof_pvar_table_lookup(unsigned int key)
{
    return hg_hash_table_lookup(pvar_table, (hg_hash_table_key_t)(&key));
}

/*---------------------------------------------------------------------------*/
hg_atomic_int32_t * 
hg_prof_get_pvar_addr_from_name(const char* name)
{
   hg_prof_pvar_data_t * value = NULL;

   for(unsigned int i = 0; i < hg_hash_table_num_entries(pvar_table); i++) {
     value = hg_hash_table_lookup(pvar_table, (hg_hash_table_key_t)(&i));
     if(strcmp(value->name, name)==0) {
       break;
     }
     value = NULL;
   }

   if(value != NULL) {
     return (hg_atomic_int32_t *)value->addr;
   } else {
      return NULL;
   }
}

/*---------------------------------------------------------------------------*/
int 
hg_prof_get_pvar_index_from_name(const char* name)
{
   hg_prof_pvar_data_t * value = NULL;
   int index = -1;
   for(unsigned int i = 0; i < hg_hash_table_num_entries(pvar_table); i++) {
     value = hg_hash_table_lookup(pvar_table, (hg_hash_table_key_t)(&i));
     if(strcmp(value->name, name)==0) {
       index = i;
       break;
     }
     value = NULL;
   }

   return index;
}

/*---------------------------------------------------------------------------*/
void 
HG_PROF_PVAR_REGISTER_impl(hg_prof_class_t varclass, hg_prof_datatype_t dtype, const char* name, void *addr, int count,
    hg_prof_bind_t bind, int continuous, const char * desc) {

    unsigned int * key = NULL;
    key = (unsigned int *)malloc(sizeof(unsigned int));
    *key = hg_hash_table_num_entries(pvar_table);
    hg_prof_pvar_data_t * pvar_info = NULL;
    pvar_info = (hg_prof_pvar_data_t *)malloc(sizeof(hg_prof_pvar_data_t));

    (*pvar_info).pvar_class = varclass;
    (*pvar_info).pvar_datatype = dtype;
    (*pvar_info).pvar_bind = bind;
    (*pvar_info).count = count;
    (*pvar_info).addr = addr;
    strcpy((*pvar_info).name, name);
    strcpy((*pvar_info).description, desc);
    (*pvar_info).continuous = continuous;

    hg_hash_table_insert(pvar_table, (hg_hash_table_key_t)key, (hg_hash_table_value_t)(pvar_info));
}

/*---------------------------------------------------------------------------*/
hg_return_t 
hg_prof_pvar_init() {

    /*Initialize internal PVAR data structures*/
    pvar_table = hg_hash_table_new(hg_prof_uint_hash, hg_prof_uint_equal);
    /* Register available PVARs */
    HG_PROF_PVAR_UINT_COUNTER_REGISTER(HG_UINT, HG_PROF_BIND_NO_OBJECT, hg_pvar_num_posted_handles, "Number of posted handles", 256);
    HG_PROF_PVAR_UINT_COUNTER_REGISTER(HG_UINT, HG_PROF_BIND_NO_OBJECT, hg_pvar_hg_backfill_queue_count, "Backfill queue size", 0);
    HG_PROF_PVAR_UINT_COUNTER_REGISTER(HG_UINT, HG_PROF_BIND_NO_OBJECT, hg_pvar_hg_completion_queue_count, "Completion queue size", 0);
    HG_PROF_PVAR_UINT_COUNTER_REGISTER(HG_UINT, HG_PROF_BIND_NO_OBJECT, hg_pvar_hg_na_ofi_completion_count, "Number of actual events during a fi_cq_read operation", 0);
    HG_PROF_PVAR_UINT_COUNTER_REGISTER(HG_UINT, HG_PROF_BIND_NO_OBJECT, hg_pvar_hg_forward_count, "Number of times HG_Forward has been invoked", 0);
    HG_PROF_PVAR_DOUBLE_COUNTER_REGISTER(HG_DOUBLE, HG_PROF_BIND_HANDLE, hg_pvar_hg_origin_callback_completion_time, "Time taken for origin to trigger callback(s)", 0);
    HG_PROF_PVAR_DOUBLE_COUNTER_REGISTER(HG_DOUBLE, HG_PROF_BIND_HANDLE, hg_pvar_hg_internal_rdma_transfer_time, "Time taken for internal RDMA transfer(s)", 0);
    HG_PROF_PVAR_UINT_COUNTER_REGISTER(HG_UINT, HG_PROF_BIND_HANDLE, hg_pvar_hg_internal_rdma_transfer_size, "Size of internal RDMA transfer (bytes)", 0);
    HG_PROF_PVAR_DOUBLE_COUNTER_REGISTER(HG_DOUBLE, HG_PROF_BIND_HANDLE, hg_pvar_hg_input_serial_time, "Time taken to serialize input (s)", 0);
    HG_PROF_PVAR_DOUBLE_COUNTER_REGISTER(HG_DOUBLE, HG_PROF_BIND_HANDLE, hg_pvar_hg_input_deserial_time, "Time taken to de-serialize input (s)", 0);
    HG_PROF_PVAR_DOUBLE_COUNTER_REGISTER(HG_DOUBLE, HG_PROF_BIND_HANDLE, hg_pvar_hg_output_deserial_time, "Time taken to de-serialize output (s)", 0);
    HG_PROF_PVAR_DOUBLE_COUNTER_REGISTER(HG_DOUBLE, HG_PROF_BIND_HANDLE, hg_pvar_hg_output_serial_time, "Time taken to serialize output (s)", 0);

return HG_SUCCESS;
}

/*---------------------------------------------------------------------------*/
hg_return_t 
hg_prof_pvar_finalize() {

    /*Finalize internal PVAR data structures*/
    hg_hash_table_free(pvar_table);
    pvar_table = NULL;

return HG_SUCCESS;
}