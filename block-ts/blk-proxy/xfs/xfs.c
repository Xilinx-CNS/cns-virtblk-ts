/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Blk Proxy xfs test
 *
 * @defgroup blk-proxy-xfs Block proxy xfs test
 * @grouporder{2}
 * @ingroup virtblk-blk-proxy-xfs
 *
 * @param group    Group of XFS tests
 * @param name     Name of XFS test
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

/**
 * @objective Wrapper for XFS test suite
 */

#define TE_TEST_NAME    "blk-proxy/xfs/xfs"

#include "tsapi.h"
#include "tsapi-xfs.h"

static te_errno
prepare_xfs(xfs_test_suite *xfs, const char *block, int index)
{
    te_errno rc;
    te_string str = TE_STRING_INIT;
    char *blk;

    rc = te_string_append(&str, "/tmp/disk%d", index);
    if (rc != 0)
    {
        return rc;
    }

    blk = strdup(block);
    if (blk == NULL)
    {
        te_string_free(&str);
        return TE_ENOMEM;
    }

    if (index % 2 == 0)
    {
        xfs->test_dir = str.ptr;
        xfs->test_dev = blk;
    } else {
        xfs->scratch_dir = str.ptr;
        xfs->scratch_dev = blk;
    }

    return 0;
}

#define XFS_TEST_TIMEOUT_S 300

int
main(int argc, char *argv[])
{
    static const char *ta = "Agt_host";

    const char *group = NULL;
    const char *name = NULL;
    char *xfs_dir = NULL;
    cfg_val_type val_type = CVT_STRING;

    rcf_rpc_server *disk_rpcs = NULL;
    xfs_test_suite xfs = XFS_TEST_SUITE_DEFAULT;
    char **blocks;
    unsigned int blocks_count;
    unsigned int i;

    TEST_START;
    TEST_GET_RPCS(ta, "pco", disk_rpcs);
    TEST_GET_STRING_PARAM(group);
    TEST_GET_STRING_PARAM(name);

    CHECK_RC(cfg_get_instance_fmt(&val_type, &xfs_dir, "/agent:%s/dir:", ta));

    TEST_STEP("Get the number of block devices");
    CHECK_RC(tsapi_get_devices(ta, &blocks, &blocks_count));

    if (blocks_count < 2) {
        TEST_SKIP("At least 2 block devices are required for this test");
    }

    TEST_STEP("Prepare XFS file system on each block device");
    for (i = 0; i < blocks_count || i < 2; i++)
    {
        CHECK_RC(prepare_xfs(&xfs, blocks[i], i));
    }

    TEST_STEP("Prepare environment and command line for"
              " the XFS test execution");
    CHECK_RC(xfs_test_suite_init(&xfs, disk_rpcs, xfs_dir, group, name));

    TEST_STEP("Start XFS test");
    CHECK_RC(xfs_test_suite_start(&xfs));

    TEST_STEP("Wait a report of XFS test suite");
    CHECK_RC(xfs_test_suite_wait(&xfs, XFS_TEST_TIMEOUT_S));

    TEST_SUCCESS;
cleanup:
    CLEANUP_CHECK_RC(xfs_test_suite_destroy(&xfs));
    if (blocks != NULL)
    {
        for (i = 0; i < blocks_count; i++)
            free(blocks[i]);
        free(blocks);
    }
    TEST_END;
}
