/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for work to XFS test suite
 *
 * @defgroup blk-proxy-xfs-helper Helper for work to XFS test suite
 * @ingroup virtblk-blk-proxy-lib
 * @{
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "tapi_job.h"
#include "tapi_job_factory_rpc.h"

/** XFS test suite handler */
typedef struct xfs_test_suite {
    tapi_job_t *job;                /**< XFS test suite job */
    tapi_job_factory_t *factory;    /**< Factory job */
    tapi_job_channel_t *out[2];     /**< stdout and stderr channel */

    char *group;                    /**< Name of group */
    char *name;                     /**< Name of test */
    char *test_dev;                 /**< Device under test */
    char *test_dir;                 /**< Directory to mount block device */
    char *scratch_dev;              /**< Secondary block under test */
    char *scratch_dir;              /**< Secondary directory to mount
                                         block device */
} xfs_test_suite;

/** Defaults for XFS test suite handler */
#define XFS_TEST_SUITE_DEFAULT (xfs_test_suite) {   \
    .job = NULL,                                    \
    .factory = NULL,                                \
    .out = {},                                      \
    .group = NULL,                                  \
    .name = NULL,                                   \
    .test_dev = NULL,                               \
    .test_dir = NULL,                               \
    .scratch_dev = NULL,                            \
    .scratch_dir = NULL,                            \
}

/**
 * Initialize XFS test suite handler
 *
 * @param xfs_ts       XFS test suite handler
 * @param rpcs         RPC Server
 * @param xfs_path     Path to XFS test suite
 * @param group        Group of the tests to run
 * @param name         Name of the test to run
 *
 * @return Status code
 */
extern te_errno xfs_test_suite_init(xfs_test_suite *xfs_ts,
                                    rcf_rpc_server *rpcs,
                                    const char *xfs_path,
                                    const char *group,
                                    const char *name);

/**
 * Start XFS test suite handler
 *
 * @param xfs_ts       XFS test suite handler
 *
 * @return Status code
 */
extern te_errno xfs_test_suite_start(xfs_test_suite *xfs_ts);

/**
 * Wait while XFS test suite done
 *
 * @param xfs_ts       XFS test suite handler
 * @param seconds      Seconds to wait
 *
 * @return Status code
 */
extern te_errno xfs_test_suite_wait(xfs_test_suite *xfs_ts, unsigned seconds);

/**
 * Destroy XFS test suite handler
 *
 * @param xfs_ts       XFS test suite handler
 *
 * @return Status code
 */
extern te_errno xfs_test_suite_destroy(xfs_test_suite *xfs_ts);

/**
 * Prepare XFS file system on block
 *
 * @param rpcs      RPC Server
 * @param block     Block device
 *
 * @return Status code
 */
extern te_errno xfs_test_suite_make_xfs(rcf_rpc_server *rpcs,
                                        const char *block);

/**@} <!-- END blk-proxy-xfs-helper --> */
