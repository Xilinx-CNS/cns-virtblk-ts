/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper functions for building inventory files
 *
 * @defgroup blk-proxy-inventory-helper Helpers for building inventory files
 * @ingroup virtblk-blk-proxy-lib
 * @{
 *
 * @author Denis Prazhennikov <Denis.Pryazhennikov@arknetworks.am>
 */

#ifndef __TSAPI_INVENTORY_H__
#define __TSAPI_INVENTORY_H__

#include "tsapi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prepare ceph invetory file on the agent
 *
 * @param[in]  rpcs             RPC server
 * @param[in]  kv_params        List of kv-pair parameters
 * @param[in]  mode             Backend mode
 * @param[out] inventory_path   Path to inventory on the Agent
 *
 * @return Status code
 */
extern te_errno tsapi_inventory_prepare(rcf_rpc_server *rpcs,
                                        te_kvpair_h *kv_params,
                                        tsapi_backend_mode mode,
                                        const char **inventory_path);

/**
 * Add an instance with the string value to /local/backend/inventory tree.
 *
 * @param backend Name of the backend
 * @param name    Name of the adding parameter
 * @param value   Value to adding
 *
 * @return Status code
 */
extern te_errno tsapi_inventory_local_add_str(const char *backend,
                                              const char *name,
                                              const char *value);

/**
 * Add an instance with the integer value to /local/backend/inventory tree.
 *
 * @param backend Name of the backend
 * @param name    Name of the adding parameter
 * @param value   Value to adding
 *
 * @return Status code
 */
extern te_errno tsapi_inventory_local_add_int(const char *backend,
                                              const char *name,
                                              int value);

/**
 * Parse /local/backend/inventory tree and put these values to
 * key-value pairs.
 *
 * /local:/backend:backend/inventory:key = value
 *
 * @param backend Name of the backend
 * @param params  Key-value pairs
 *
 * @return Status code
 */
extern te_errno tsapi_inventory_local_to_kv_pair(const char *backend,
                                                 te_kvpair_h *params);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TSAPI_INVENTORY_H__ */

/** @} <!-- END blk-proxy-inventory-helper --> */
