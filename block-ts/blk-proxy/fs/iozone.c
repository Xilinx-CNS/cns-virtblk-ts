/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief FS iozone test.
 *
 * @defgroup blk-proxy-fs-iozone FS iozone test
 * @grouporder{5}
 * @ingroup virtblk-blk-proxy-fs
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 *
 * Check a filesytem created on a block device using iozone tool.
 *
 * @param record_size  Size of the chunks that iozone breaks a file into
 *                     before performing an I/O operation (R/W) on the disk.
 * @param file_size    Size of the file under test.
 */

#define TE_TEST_NAME "blk-proxy/fs/iozone"

#include "tsapi.h"
#include "tapi_job_factory_rpc.h"
#include "tsapi_iozone.h"

/**
 * Default timeout to  wait for iozone to complete.
 */
#define IOZONE_TIMEOUT_DEFAULT_S 6000

/**
 * Maximal timeout to wait for heaviest iterations of iozone to complete.
 * Used only when record_size is 4k.
 */
#define IOZONE_TIMEOUT_MAX_S 60000

int
main(int argc, char *argv[])
{
    const char *ta = "Agt_host";

    rcf_rpc_server      *rpcs = NULL;
    tapi_job_factory_t  *factory = NULL;
    tsapi_iozone_app    *app = NULL;
    tsapi_iozone_opt     opt = tsapi_iozone_default_opt;
    char                *mount_dir = NULL;
    int                  iozone_timeout_s;

    TEST_START;

    TEST_GET_RPCS(ta, "pco", rpcs);

    mount_dir = tsapi_get_mount_dir_name(ta, 0);
    RING("IOzone working directory is %s", mount_dir);

    TEST_STEP("Read iozone options");
    opt.record_size = TEST_STRING_PARAM(record_size);
    opt.file_size = TEST_STRING_PARAM(file_size);
    RING("Options: record_size=%s, file_size=%s",
         opt.record_size, opt.file_size);

    TEST_STEP("Initialize iozone app");
    CHECK_RC(tapi_job_factory_rpc_create(rpcs, &factory));
    CHECK_RC(tsapi_iozone_create(factory, &opt, mount_dir, &app));

    TEST_STEP("Start iozone");
    CHECK_RC(tsapi_iozone_start(app));

    TEST_STEP("Wait for iozone to complete");
    if (strcmp(opt.record_size, "4k") != 0)
        iozone_timeout_s = TE_SEC2MS(IOZONE_TIMEOUT_DEFAULT_S);
    else
        iozone_timeout_s = TE_SEC2MS(IOZONE_TIMEOUT_MAX_S);

    CHECK_RC(tsapi_iozone_wait(app, iozone_timeout_s));

    TEST_STEP("Print iozone report");
    CHECK_RC(tsapi_iozone_print_results(app));

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tsapi_iozone_destroy(app));
    tapi_job_factory_destroy(factory);
    free(mount_dir);

    TEST_END;
}
