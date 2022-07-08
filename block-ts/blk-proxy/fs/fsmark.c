/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief FS fsmark test.
 *
 * @defgroup blk-proxy-fs-fsmark FS fsmark test
 * @grouporder{4}
 * @ingroup virtblk-blk-proxy-fs
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 *
 * Check a filesytem created on a block device using fsmark tool.
 *
 * @param numfiles   Number of files to be used by fsmark.
 * @param file_size  Size of each file in bites.
 */

#define TE_TEST_NAME "blk-proxy/fs/fsmark"

#include "tsapi.h"
#include "tapi_job_factory_rpc.h"
#include "tsapi_fsmark.h"
#include "tsapi-fs.h"

/** Timeout to wait for fsmark to complete */
#define FSMARK_TIMEOUT_S 120

int
main(int argc, char *argv[])
{
    const char *ta = "Agt_host";

    rcf_rpc_server      *rpcs = NULL;
    tapi_job_factory_t  *factory = NULL;
    tsapi_fsmark_app    *app = NULL;
    tsapi_fsmark_opt     opt = tsapi_fsmark_default_opt;
    char                *mount_dir = NULL;

    char         **blocks;
    unsigned int   blocks_count;

    unsigned int i;

    TEST_START;

    TEST_GET_RPCS(ta, "pco", rpcs);

    mount_dir = tsapi_get_mount_dir_name(ta, 0);
    RING("FSMark working directory is %s", mount_dir);
    opt.working_dir = mount_dir;

    TEST_STEP("Read fsmark options");
    opt.numfiles = TEST_UINT_PARAM(numfiles);
    opt.file_size = TEST_UINT_PARAM(file_size);
    RING("Options: numfiles=%u, file_size=%u", opt.numfiles, opt.file_size);

    TEST_STEP("Initialize fsmark app");
    CHECK_RC(tapi_job_factory_rpc_create(rpcs, &factory));
    CHECK_RC(tsapi_fsmark_create(factory, &opt, &app));

    TEST_STEP("Start fsmark");
    CHECK_RC(tsapi_fsmark_start(app));

    TEST_STEP("Wait for fsmark to complete");
    CHECK_RC(tsapi_fsmark_wait(app, TE_SEC2MS(FSMARK_TIMEOUT_S)));

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tsapi_fsmark_destroy(app));
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
