/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Prepare blk-proxy disk for able start XFS test
 *
 * @defgroup blk-proxy-xfs-epilogue Cleanup after XFS tests execution
 * @grouporder{3}
 * @ingroup virtblk-blk-proxy-xfs
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

/**
 * @objective Cleanup after blk-proxy disk for able start XFS test
 */

#define TE_TEST_NAME    "blk-proxy/xfs/epilogue"

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


    TEST_STEP("Get number of block devices on Agt_host.");
    CHECK_RC(tsapi_get_devices(ta, &blocks, &blocks_count));

    TEST_STEP("For each block device:");
    for (i = 0; i < blocks_count; i++)
    {
        TEST_SUBSTEP("Remove mount directory created in prologue.");
        CHECK_RC(tsapi_remove_mount_dir(disk_rpcs, i));
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
