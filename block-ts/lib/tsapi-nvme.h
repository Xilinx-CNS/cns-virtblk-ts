/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for work with NVME
 *
 * @defgroup blk-proxy-nvme Helper for work backends
*  @ingroup virtblk-blk-proxy-lib
 * @{
 *
 * @author Artemii Morozov <Artemii.Morozov@arknetworks.am>
 */

#ifndef __TSAPI_NVME_H__
#define __TSAPI_NVME_H__

#include "tsapi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prepare parameters for inventory file.
 *
 * @param mon_addr    IP address of the NVME server
 * @param block_count Number of devices to create
 *
 * @return Status code.
 */
extern te_errno tsapi_nvme_prepare_params(const struct sockaddr *addr,
                                          unsigned int block_count);

/**
 * Start NVME server.
 *
 * @param rpcs                    RPC server
 * @param helper_path             Path to backend-helpers directory
 * @param virtblk_ci_ansible_path Path to virtblk-ci/ansible directory
 * @param inventory_path          Path to inventory file
 *
 * @return Status code.
 */
extern te_errno tsapi_nvme_start_server(rcf_rpc_server *rpcs,
                                        const char *helper_path,
                                        const char *virtblk_ci_ansible_path,
                                        const char *inventory_path);

/**
 * Stop NVME server.
 *
 * @param rpcs        RPC server
 * @param helper_path Path to backend-helpers directory
 *
 * @return Status code.
 */
extern te_errno tsapi_nvme_stop_server(rcf_rpc_server *rpcs,
                                       const char *helpers_path);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TSAPI_NVME_H__ */

/** @} <!-- END blk-proxy-nvme --> */
