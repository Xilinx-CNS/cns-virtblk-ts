/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief FS compilebench test.
 *
 * @defgroup blk-proxy-fs-compilebench FS compilebench test
 * @grouporder{2}
 * @ingroup virtblk-blk-proxy-fs
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 *
 * Check a filesytem created on a block device using compilebench tool.
 *
 * @param initial_dirs  Number of directories initially created.
 * @param runs          Number of random operations to be performed.
 * @param makej         Simulate a make -j on the initial dirs and exit.
 */

#define TE_TEST_NAME "blk-proxy/fs/compilebench"

#include "tsapi.h"
#include "tapi_job_factory_rpc.h"
#include "tsapi_compilebench.h"
#include "tsapi-fs.h"

/** Timeout to wait for compilebench to complete */
#define COMPILEBENCH_TIMEOUT_S 1200

int
main(int argc, char *argv[])
{
    const char *ta = "Agt_host";

    rcf_rpc_server          *rpcs = NULL;
    tapi_job_factory_t      *factory = NULL;
    tsapi_compilebench_app  *app = NULL;
    tsapi_compilebench_opt   opt = tsapi_compilebench_default_opt;
    char                    *mount_dir = NULL;

    char         **blocks;
    unsigned int   blocks_count;

    unsigned int i;

    TEST_START;

    TEST_GET_RPCS(ta, "pco", rpcs);

    mount_dir = tsapi_get_mount_dir_name(ta, 0);
    RING("Compilebench working directory is %s", mount_dir);
    opt.working_dir = mount_dir;

    TEST_STEP("Read compilebench options");
    opt.initial_dirs = TEST_UINT_PARAM(initial_dirs);
    opt.runs = TEST_UINT_PARAM(runs);
    opt.makej = TEST_ENUM_PARAM(makej, BOOL_MAPPING_LIST);

    TEST_STEP("Initialize compilebench app");
    CHECK_RC(tapi_job_factory_rpc_create(rpcs, &factory));
    CHECK_RC(tsapi_compilebench_create(factory, &opt, &app));

    TEST_STEP("Start compilebench");
    CHECK_RC(tsapi_compilebench_start(app));

    TEST_STEP("Wait for compilebench to complete");
    CHECK_RC(tsapi_compilebench_wait(app, TE_SEC2MS(COMPILEBENCH_TIMEOUT_S)));

    TEST_STEP("Print compilebench report");
    CHECK_RC(tsapi_compilebench_print_results(app));

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tsapi_compilebench_destroy(app));
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
