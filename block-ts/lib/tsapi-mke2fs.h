/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper to work with mke2fs test suite
 *
 * @defgroup blk-proxy-mke2fs-helper Helpers to work with mke2fs test suite
 * @ingroup virtblk-blk-proxy-lib
 * @{
 *
 * Helper to work with mke2fs test suite.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TSAPI_MKE2FS_H__
#define __TSAPI_MKE2FS_H__

#include "tapi_mke2fs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check the FS created by mke2fs.
 * The check is done in 3 steps:
 * 1) Check the FS using 'e2fsck' without fixing any errors.
 * 2) Check the type of the FS using 'file'.
 * 3) Check whether the FS was created with ext3 journal
 *    using tapi_mke2fs_check_journal().
 *
 * @param factory            Job factory.
 * @param app                mke2fs app handle.
 * @param fs_type            It will be checked if the FS has this type
 *                           (ext2, ext3, ext4). May be @c NULL to omit
 *                           the check.
 * @param device             A name of a device on which the FS was created.
 * @param e2fsck_timeout_ms  Timeout for which to wait e2fsck completion.
 *                           The bigger the FS is, the bigger should this
 *                           timeout be.
 *
 * @return Status code.
 * @retval TE_EPROTO         One of the checks revealed that the FS was
 *                           not created as expected.
 */
extern te_errno mke2fs_ts_check_fs(tapi_job_factory_t *factory,
                                   tapi_mke2fs_app *app,
                                   const char *fs_type,
                                   const char *device,
                                   int e2fsck_timeout_ms);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TSAPI_MKE2FS_H__ */

/** @} <!-- END blk-proxy-mke2fs-helper --> */
