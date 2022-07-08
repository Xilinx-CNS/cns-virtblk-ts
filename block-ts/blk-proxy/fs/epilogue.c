/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief FS package epilogue
 *
 * @defgroup blk-proxy-fs-epilogue Epilogue for FS tests
 * @grouporder{6}
 * @ingroup virtblk-blk-proxy-fs
 *
 * Unmount the filesystem and remove the mount directory.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_TEST_NAME "blk-proxy/fs/epilogue"

#include "tsapi.h"
#include "tsapi-fs.h"
#include "tsapi-fio.h"

#define DEV_COUNT 1

int
main(int argc, char *argv[])
{
    const char *ta = "Agt_host";

    rcf_rpc_server *rpcs = NULL;
    char           *mount_dir = NULL;

    TEST_START;

    TEST_GET_RPCS(ta, "pco", rpcs);

    mount_dir = tsapi_get_mount_dir_name(ta, 0);

    TEST_STEP("Unmount the filesystem");
    CHECK_RC(fs_ts_umount_filesystem(rpcs, mount_dir, TRUE));

    TEST_STEP("Remove the mount directory");
    CHECK_RC(tsapi_remove_mount_dir(rpcs, 0));

    TEST_SUCCESS;

cleanup:
    free(mount_dir);

    TEST_END;
}
