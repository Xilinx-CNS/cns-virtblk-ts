/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief FS package prologue
 *
 * @defgroup blk-proxy-fs-prologue Prologue for FS tests
 * @grouporder{1}
 * @ingroup virtblk-blk-proxy-fs
 *
 * Create a filesystem on a virtio block device and mount it.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_TEST_NAME "blk-proxy/fs/prologue"

#include "tsapi.h"
#include "tsapi-fs.h"

int
main(int argc, char *argv[])
{
    const char *ta = "Agt_host";

    rcf_rpc_server *rpcs = NULL;
    char           *mount_dir = NULL;

    char         **blocks;
    unsigned int   blocks_count;

    unsigned int i;

    TEST_START;

    TEST_GET_RPCS(ta, "pco", rpcs);

    TEST_STEP("Get a block device that will used to create a filesystem");
    CHECK_RC(tsapi_get_devices(ta, &blocks, &blocks_count));
    RING("Filesystem will be created on '%s'", blocks[0]);

    CHECK_RC(fs_ts_create_filesystem(rpcs, blocks[0], TRUE));

    mount_dir = tsapi_get_mount_dir_name(ta, 0);
    TEST_STEP("Create a directory to mount the filesystem at");
    CHECK_RC(tsapi_create_mount_dir(rpcs, 0));

    CHECK_RC(fs_ts_mount_filesystem(rpcs, blocks[0], mount_dir, TRUE));

    TEST_SUCCESS;

cleanup:
    free(mount_dir);

    if (blocks != NULL)
    {
        for (i = 0; i < blocks_count; i++)
            free(blocks[i]);
        free(blocks);
    }

    TEST_END;
}
