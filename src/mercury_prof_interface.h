/*
 * Copyright (C) 2013-2020 Argonne National Laboratory, Department of Energy,
 *                    UChicago Argonne, LLC and The HDF Group.
 * All rights reserved.
 *
 * The full copyright notice, including terms governing use, modification,
 * and redistribution, is contained in the COPYING file that can be
 * found at the root of the source code distribution tree.
 */

#ifndef MERCURY_PROF_INTERFACE_H
#define MERCURY_PROF_INTERFACE_H

#include "mercury_types.h"
#include "mercury_prof_types.h"


/*************************************/
/* Note */
/*************************************/

/* Refer to: https://www.mpi-forum.org/docs/mpi-3.1/mpi31-report/node372.htm#Node372 
 * for the corresponding MPI 3.1 PVAR interface specification that this implementation is largely based off of.
 *
 * Notable differences with MPI (implementation, not interface spec):
 * 1. PVAR sessions in Mercury are associated with the hg_class object to avoid the use of global variables.
 * 2. Similarly, PVARs themselves are not global variables and aren't declared globally and used as such. 
 *    Instead, each module interested in exporting PVARs must register these PVARs and refer to them by the 
 *    use of handles (that internally fetch the address of the PVAR) within the local function where these PVARs are updated.
 * 3. We used atomics for updating counter PVARs where MPI does not.
 */

/*************************************/
/* Public Type and Struct Definition */
/*************************************/

/* See mercury_prof_types.h */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Client-side API to initialize the PVAR profiling interface.
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PUBLIC hg_return_t 
HG_Prof_init(hg_class_t *hg_class);

/**
 * Client-side API to finalize the PVAR profiling interface.
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PUBLIC hg_return_t 
HG_Prof_finalize(hg_class_t *hg_class);

/**
 * Create a session.
 *
 * \param hg_class [IN/OUT]       pointer to HG_CLASS 
 * \param session [IN/OUT]        pointer to PVAR session object 
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PUBLIC hg_return_t 
HG_Prof_pvar_session_create(hg_class_t *hg_class, hg_prof_pvar_session_t *session);

/**
 * Destroy a session.
 *
 * \param hg_class [IN/OUT]       pointer to HG_CLASS 
 * \param session [IN/OUT]        pointer to PVAR session object 
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PUBLIC hg_return_t 
HG_Prof_pvar_session_destroy(hg_class_t *hg_class, hg_prof_pvar_session_t *session);

/**
 * Client-side API to retrieve the number of PVARs currently exported.
 *
 * \return int representing the number of PVARs
 */
HG_PUBLIC int 
HG_Prof_pvar_get_num(hg_class_t* hg_class);

/**
 * Gather information about every PVAR exported. This API is necessary in order for the client to discover the types, bindings, etc.
 * The client can then allocate the necessary data structures using this information.
 *
 * \param pvar_index [IN]        PVAR index
 * \param name [IN/OUT]		 pointer to the string array containing the PVAR name
 * \param name_len [IN/OUT]	 pointer to the integer address holding the number of characters in the name array
 * \param var_class [IN/OUT]	 pointer to the object denoting the PVAR class
 * \param datatype [IN/OUT]	 pointer to the object denoting the PVAR datatype
 * \param desc [IN/OUT]		 pointer to the string array containing the PVAR description
 * \param desc_len [IN/OUT]	 pointer to the integer address holding the number of characters in the description array
 * \param bind [IN/OUT]		 pointer to the object denoting PVAR binding
 * \param continuous [IN/OUT]	 pointer to the integer address denoting if the PVAR is continuous or not
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PUBLIC hg_return_t 
HG_Prof_pvar_get_info(hg_class_t *hg_class, int pvar_index, char *name, int *name_len, 
			hg_prof_class_t *var_class, hg_prof_datatype_t *datatype, 
			char *desc, int *desc_len, hg_prof_bind_t *bind, int *continuous);

/**
 * Allocate a handle for a PVAR at a given index.
 * This handle will later be used by the client to query the value for the PVAR.
 * This handle is an opaque object.
 *
 * \param session [IN]            opaque PVAR session object
 * \param pvar_index [IN]	  PVAR index
 * \param obj_handle [IN]	  pointer to the object handle with which the PVAR is associated; can be NULL
 * \param handle [IN/OUT]	  pointer to the opaque PVAR handle object that is returned to the calling client
 * \param count [IN/OUT]	  pointer to the integer address representing the number of values associated with the PVAR
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PUBLIC hg_return_t 
HG_Prof_pvar_handle_alloc(hg_prof_pvar_session_t session, 
			int pvar_index, void *obj_handle, hg_prof_pvar_handle_t *handle, int *count);

/**
 * Free handle for a PVAR at a given index.
 *
 * \param session [IN]            opaque PVAR session object
 * \param pvar_index [IN]	  PVAR index
 * \param handle [IN/OUT]	  pointer to the opaque PVAR handle object that is returned to the calling client
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PUBLIC hg_return_t 
HG_Prof_pvar_handle_free(hg_prof_pvar_session_t session, 
			int pvar_index, hg_prof_pvar_handle_t *handle);

/**
 * Start the PVAR is it is not continuous and has not been started yet.
 *
 * \param session [IN]        opaque PVAR session object 
 * \param handle [IN]         opaque PVAR handle object 
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PUBLIC hg_return_t 
HG_Prof_pvar_start(hg_prof_pvar_session_t session, hg_prof_pvar_handle_t handle);

/**
 * Stop the PVAR is it is not continuous and has been started.
 *
 * \param session [IN]        opaque PVAR session object 
 * \param handle [IN]         opaque PVAR handle object 
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PUBLIC hg_return_t 
HG_Prof_pvar_stop(hg_prof_pvar_session_t session, hg_prof_pvar_handle_t handle);

/**
 * Read the value of the PVAR when the client supplies the handle.
 * Note that the handle is necessary as the input (instead of the pvar_index) because there may be multiple PVAR sessions in flight.
 *
 * \param session [IN]        opaque PVAR session object 
 * \param handle [IN]         opaque PVAR handle object 
 * \param mercury_handle [IN] opaque hg_handle object, can be NULL
 * \param buf [IN/OUT]        buffer that contains the PVAR data
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PUBLIC hg_return_t 
HG_Prof_pvar_read(hg_prof_pvar_session_t session, hg_prof_pvar_handle_t handle, hg_handle_t mercury_handle, void *buf);

#ifdef __cplusplus
}
#endif

#endif /* MERCURY_PROF_INTERFACE_H */