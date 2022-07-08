/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helpers for the performance package
 *
 * @defgroup blk-proxy-performance-lib Helpers for the performance package
 * @ingroup virtblk-blk-proxy-performance
 * @{
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

#ifndef __VIRTBLK_TS_PERFORMANCE_LIB_H__
#define __VIRTBLK_TS_PERFORMANCE_LIB_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Default blk-proxy process name for the performance testing */
#define PERFORMANCE_LIB_BLK_PROXY_PS_NAME "perf-blk-proxy"

/** Default blk-proxy config name for the performance testing */
#define PERFORMANCE_LIB_BLK_PROXY_CONF_NAME "perf_bp_map"

/** Default Ceph pool containing RBD */
#define PERFORMANCE_LIB_DEFAULT_RBD_POOL_NAME "rampool"

/** Default RBD name */
#define PERFORMANCE_LIB_DEFAULT_RBD_NAME "ramdev"

/** The total size of file I/O to fill RBD */
#define PERFORMANCE_LIB_DEFAULT_FIO_OPT_SIZE "100%"

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __VIRTBLK_TS_PERFORMANCE_LIB_H__ */
