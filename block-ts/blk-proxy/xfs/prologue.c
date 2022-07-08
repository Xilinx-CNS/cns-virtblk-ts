/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Prepare blk-proxy disk for able start XFS test
 *
 * @defgroup blk-proxy-xfs-prologue Prepare blk-proxy disk to start XFS tests
 * @grouporder{1}
 * @ingroup virtblk-blk-proxy-xfs
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

/**
 * @objective Prepare blk-proxy disk for able start XFS test
 */

#define TE_TEST_NAME    "blk-proxy/xfs/prologue"

#include "tsapi.h"
#include "tsapi-xfs.h"

int
main(int argc, char *argv[])
{
    static const char *ta = "Agt_host";

    rcf_rpc_server *disk_rpcs = NULL;
    char **blocks;
    unsigned int blocks_count;
    unsigned int i;

    TEST_START;
    TEST_GET_RPCS(ta, "pco", disk_rpcs);

    TEST_STEP("Check that XFS_TS_DIR environment variable is set.");
    if (getenv("XFS_TS_DIR") == NULL)
    {
        TEST_FAIL("XFS_TS_DIR should be set for start this test");
    }

    TEST_STEP("Get number of block devices on the Agt_host ta.");
    CHECK_RC(tsapi_get_devices(ta, &blocks, &blocks_count));

    TEST_STEP("For each block device:");
    for (i = 0; i < blocks_count; i++)
    {
        TEST_SUBSTEP("Prepare XFS file system.");
        CHECK_RC(xfs_test_suite_make_xfs(disk_rpcs, blocks[i]));
        TEST_SUBSTEP("Create mount directory");
        CHECK_RC(tsapi_create_mount_dir(disk_rpcs, i));
    }

    TEST_SUCCESS;
cleanup:
    if (blocks != NULL)
    {
        for (i = 0; i < blocks_count; i++)
            free(blocks[i]);
        free(blocks);
    }
    TEST_END;
}
