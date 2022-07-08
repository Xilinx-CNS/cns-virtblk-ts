/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper to work with fs test suite
 *
 * @defgroup blk-proxy-fs-helper Helpers to work with fs test suite
 * @ingroup virtblk-blk-proxy-lib
 * @{
 *
 * Helper to work with fs test suite.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TSAPI_FS_H__
#define __TSAPI_FS_H__

#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create ext4 filesystem on a block device using mke2fs.
 *
 * @param rpcs         RPC server.
 * @param device       Device on which to create filesystem.
 * @param step         If @c TRUE, actions performed by function will be logged
 *                     with TEST_STEP(), otherwise with TEST_SUBSTEP().
 *
 * @return Status code.
 */
extern te_errno fs_ts_create_filesystem(rcf_rpc_server *rpcs,
                                        const char *device,
                                        te_bool step);

/**
 * Mount a filesystem on a block device.
 *
 * @param rpcs         RPC server.
 * @param device       Device on which to create filesystem.
 * @param mount_dir    Directory at which to mount.
 * @param step         If @c TRUE, actions performed by function will be logged
 *                     with TEST_STEP(), otherwise with TEST_SUBSTEP().
 *
 * @return Status code.
 */
extern te_errno fs_ts_mount_filesystem(rcf_rpc_server *rpcs,
                                       const char *device,
                                       const char *mount_dir,
                                       te_bool step);

/**
 * Unmount a filesystem.
 *
 * @param rpcs         RPC server.
 * @param mount_dir    Directory at which the filesystem is mounted.
 * @param step         If @c TRUE, actions performed by function will be logged
 *                     with TEST_STEP(), otherwise with TEST_SUBSTEP().
 *
 * @return Status code.
 */
extern te_errno fs_ts_umount_filesystem(rcf_rpc_server *rpcs,
                                        const char *mount_dir,
                                        te_bool step);

/**
 * Recreate a filesystem on a block device.
 * Useful for freeing space instead of removing all files.
 *
 * @param rpcs         RPC server.
 * @param device       Device on which to recreate filesystem.
 * @param mount_dir    Directory at which the current filesystem is mounted
 *                     and at which newly created filesystem will be mounted.
 * @param step         If @c TRUE, actions performed by function will be logged
 *                     with TEST_STEP(), otherwise with TEST_SUBSTEP().
 *
 * @return Status code.
 */
extern te_errno fs_ts_recreate_filesystem(rcf_rpc_server *rpcs,
                                          const char *device,
                                          const char *mount_dir,
                                          te_bool step);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TSAPI_FS_H__ */

/** @} <!-- END blk-proxy-fs-helper --> */
