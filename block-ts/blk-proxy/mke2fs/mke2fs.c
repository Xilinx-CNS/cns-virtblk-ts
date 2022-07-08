/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Mke2fs test
 *
 * @defgroup blk-proxy-mke2fs-mke2fs Mke2fs test
 * @ingroup virtblk-blk-proxy-mke2fs
 *
 * Test calling mke2fs on virtio block devices
 *
 * @param fs_type      Filesystem type that is to be created.
 * @param use_journal  Whether it is requested to use journal.
 * @param block_size   Size of blocks in bytes.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_TEST_NAME  "blk-proxy/mke2fs/mke2fs"

#include "tsapi.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_mke2fs.h"
#include "tsapi-mke2fs.h"

#define TSAPI_MKE2FS_DEV_COUNT 1

/** Timeout to wait for mke2fs to complete */
#define MKE2FS_TIMEOUT_S 600

int
main(int argc, char *argv[])
{
    const char *ta = "Agt_host";

    rcf_rpc_server      *rpcs = NULL;
    tapi_job_factory_t  *factory = NULL;
    tapi_mke2fs_app     *app = NULL;
    tapi_mke2fs_opt      opt = tapi_mke2fs_default_opt;

    char          **blocks;
    unsigned int    blocks_count;

    unsigned int i;

    TEST_START;

    TEST_GET_RPCS(ta, "pco", rpcs);

    TEST_STEP("Get a device for the test");
    CHECK_RC(tsapi_get_devices(ta, &blocks, &blocks_count));
    opt.device = blocks[0];
    RING("Filesystem will be created on '%s'", opt.device);

    TEST_STEP("Read mke2fs options");
    opt.block_size = TEST_UINT_PARAM(block_size);
    opt.use_journal = TEST_ENUM_PARAM(use_journal, BOOL_MAPPING_LIST);
    opt.fs_type = TEST_STRING_PARAM(fs_type);

    CHECK_RC(tapi_job_factory_rpc_create(rpcs, &factory));

    TEST_STEP("Call mke2fs");
    CHECK_RC(tapi_mke2fs_do(factory, &opt, &app, TE_SEC2MS(MKE2FS_TIMEOUT_S)));

    TEST_STEP("Check the filesystem");
    CHECK_RC(mke2fs_ts_check_fs(factory, app, opt.fs_type, opt.device,
                                TE_SEC2MS(2)));
    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_mke2fs_destroy(app));
    tapi_job_factory_destroy(factory);

    if (blocks != NULL)
    {
        for (i = 0; i < blocks_count; i++)
            free(blocks[i]);
        free(blocks);
    }

    TEST_END;
}
