/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for work with backends
 *
 * @defgroup blk-proxy-backend-helper Helper for work backends
*  @ingroup virtblk-blk-proxy-lib
 * @{
 *
 * @author Artemii Morozov <Artemii.Morozov@arknetworks.am>
 */

#ifndef __TSAPI_BACKEND_H__
#define __TSAPI_BACKEND_H__

#include "tsapi.h"
#include "tsapi-ceph.h"
#include "block-ts.h"
#include "tsapi-nvme.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Callback for start the backend.
 *
 * @param rpcs RPC Server
 *
 * @return Status code
 */
typedef te_errno (* tsapi_backend_start)(rcf_rpc_server *rpcs);

/**
 * Callback for stop the backend.
 *
 * @param rpcs RPC Server
 *
 * @return Status code
 */
typedef te_errno (* tsapi_backend_stop)(rcf_rpc_server *rpcs);

/**
 * Callback for client preparation for the backend.
 *
 * @param rpcs RPC Server
 * @param data A pointer to the data that is needed to prepare the client.
 *
 * @return Status code
 */
typedef te_errno (* tsapi_backend_prepapre_client)(rcf_rpc_server *rpcs,
                                                   const void *data);

/**
 * Callback for prepare parameters the backend.
 *
 * @param addr        Address of the client.
 * @param block_count Number of the block devices.
 *
 * @return Status code
 */
typedef te_errno (* tsapi_backend_prepare_params)(const struct sockaddr *,
                                                  unsigned int);

/**
 * Callback for additional hook.
 *
 * @param data A pointer to the data that is needed
 *
 * @return Status code
 */
typedef te_errno (* tsapi_backend_additional_hook)(void *data);

/** Callback functions for a specific backend. */
typedef struct backend_callbacks_t {
    tsapi_backend_start start;
    tsapi_backend_stop stop;
    tsapi_backend_prepare_params prepare_params;
    tsapi_backend_prepapre_client prepare_clients;
    tsapi_backend_additional_hook hook;
} backend_callbacks_t;

/** Types of the backend. */
typedef enum tsapi_backend_types {
    /** Start ceph in container */
    TSAPI_BACKEND_TYPES_CEPH_CONTAINER = 0,
    /** Do not start ceph. Just connect to existing one */
    TSAPI_BACKEND_TYPES_CEPH_CONNECT,
    /** Start ceph in qemu */
    TSAPI_BACKEND_TYPES_CEPH_QEMU,
    /** Start nvme server */
    TSAPI_BACKEND_TYPES_NVME,
    /** Do not start NVME server. Just connect to existing one */
    TSAPI_BACKEND_TYPES_NVME_CONNECT,
    /** Use ramdisk */
    TSAPI_BACKEND_TYPES_MALLOC,
    /** Use nullblock */
    TSAPI_BACKEND_TYPES_NULLBLOCK,
    TSAPI_BACKEND_TYPES_NONE,
} tsapi_backend_types;

/**
 * Generic function for start the backend server.
 *
 * @param rpcs RPC server
 *
 * @return Status code.
 */
extern te_errno tsapi_backend_start_server(rcf_rpc_server *rpcs);

/**
 * Generic function for stop the backend server.
 *
 * @param rpcs RPC server
 *
 * @return Status code.
 */
extern te_errno tsapi_backend_stop_server(rcf_rpc_server *rpcs);


/* Array of callbacks for each type of backend */
extern const backend_callbacks_t backend_callbacks[];

/**
 * Get backend type.
 *
 * @return Backend type.
 */
extern tsapi_backend_types tsapi_backend_get_type(void);

/**
 * Get backend mode as a string.
 *
 * @param mode Backend mode.
 *
 * @return Backend mode
 */
extern char * tsapi_backend_mode2str(tsapi_backend_mode mode);

/**
 * Get backend mode.
 *
 * @return Backend mode
 */
extern tsapi_backend_mode tsapi_backend_get_mode(void);

/**
 * Determine if network ENV is needed.
 *
 * @return @c TRUE if needed, otherwise @c FALSE
 */
extern te_bool tsapi_backend_need_env(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TSAPI_BACKEND_H__ */

/** @} <!-- END blk-proxy-backend-helper --> */
