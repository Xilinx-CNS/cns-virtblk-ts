/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief FS dbench test
 *
 * @defgroup blk-proxy-fs-dbench FS dbench test
 * @grouporder{3}
 * @ingroup virtblk-blk-proxy-fs
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 *
 * Check a filesytem created on a block device using dbench tool.
 *
 * @param numclients  Number of clients to run simultaneously.
 */

#define TE_TEST_NAME "blk-proxy/fs/dbench"

#include "tsapi.h"
#include "tapi_job_factory_rpc.h"
#include "tsapi_dbench.h"
#include "tsapi-fs.h"

int
main(int argc, char *argv[])
{
    const char *ta = "Agt_host";

    rcf_rpc_server      *rpcs = NULL;
    tapi_job_factory_t  *factory = NULL;
    tsapi_dbench_app    *app = NULL;
    tsapi_dbench_opt     opt = tsapi_dbench_default_opt;
    char                *mount_dir = NULL;

    char         **blocks;
    unsigned int   blocks_count;

    unsigned int i;

    TEST_START;

    TEST_GET_RPCS(ta, "pco", rpcs);

    mount_dir = tsapi_get_mount_dir_name(ta, 0);
    RING("Dbench working directory is %s", mount_dir);
    opt.working_dir = mount_dir;

    TEST_STEP("Read dbench options");
    opt.numclients = TEST_UINT_PARAM(numclients);
    RING("Options: numclients=%u", opt.numclients);

    TEST_STEP("Initialize dbench app");
    CHECK_RC(tapi_job_factory_rpc_create(rpcs, &factory));
    CHECK_RC(tsapi_dbench_create(factory, &opt, &app));

    TEST_STEP("Start dbench");
    CHECK_RC(tsapi_dbench_start(app));

    TEST_STEP("Wait for dbench to completed");
    /* Wait for additional time for cleanup operations to complete */
    CHECK_RC(tsapi_dbench_wait(app,
                               TE_SEC2MS(2 * TSAPI_DBENCH_DEFAULT_RUNTIME)));

    TEST_STEP("Print dbench report");
    CHECK_RC(tsapi_dbench_print_results(app));

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tsapi_dbench_destroy(app));
    tapi_job_factory_destroy(factory);

    TEST_STEP("Recreate the filsystem on the block device under test "
              "in order to free space");
    CLEANUP_CHECK_RC(tsapi_get_devices(ta, &blocks, &blocks_count));
    CLEANUP_CHECK_RC(fs_ts_recreate_filesystem(rpcs, blocks[0],
                                               mount_dir, FALSE));
    free(mount_dir);

    if (blocks != NULL)
    {
        for (i = 0; i < blocks_count; i++)
            free(blocks[i]);
        free(blocks);
    }

    TEST_END;
}
