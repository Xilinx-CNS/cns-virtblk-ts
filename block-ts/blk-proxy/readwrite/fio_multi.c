/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Multi-device FIO testing
 *
 * @defgroup blk-proxy-fio-multi Multi-device FIO tests
 * @ingroup virtblk-blk-proxy-readwrite
 *
 * Run fio under block device in multi mode
 *
 * @param ioengine            FIO parameter: I/O Engine type
 * @param rwtype              FIO parameter: Read or write type
 * @param rwmixread           FIO parameter: Percentage of mixed workload that is
 *                                           reads
 * @param numjobs             FIO parameter: Duplicate this job many times
 * @param iodepth             FIO parameter: Number of IO buffers to keep
 *                                           in flight
 * @param blocksize           FIO parameter: Block size unit
 * @param runtime_sec         FIO parameter: Stop workload when the time
 *                                           expires
 * @param ioengine_second     FIO parameter: I/O Engine type for the second
 *                                           device
 * @param rwtype_second       FIO parameter: Read or write typefor the second
 *                                           device
 * @param rwmixread_second    FIO parameter: Percentage of mixed workload that
 *                                           is reads for the second device
 * @param numjobs_second      FIO parameter: Duplicate this job many times for
 *                                           the second device
 * @param iodepth_second      FIO parameter: Number of IO buffers to keep
 *                                           in flight for the second device
 * @param blocksize_second    FIO parameter: Block size unit for the second
 *                                           device
 * @param runtime_sec_second  FIO parameter: Stop workload when the time
 *                                           expires for the second device
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

/**
 * @objective Run multiple instances of FIO on each of the block devices
 *            exposed to the host
 */

#define TE_TEST_NAME  "blk-proxy/readwrite/fio_mutli"

#include "tsapi.h"
#include "tsapi-fio.h"

static te_errno
fio_blocks_to_str(const char **blocks, unsigned int count, te_string *str)
{
    te_errno rc;
    te_string result = TE_STRING_INIT;
    unsigned int i;

    for (i = 0; i < count; i++)
    {
        if ((rc = te_string_append(&result, "%s, ", blocks[i])))
        {
            te_string_free(&result);
            return rc;
        }
    }

    *str = result;

    return 0;
}

int
main(int argc, char *argv[])
{
    const char *host_ta = "Agt_host";
    rcf_rpc_server *host_rpcs = NULL;

    te_string str;
    tsapi_fio_context ctx[TSAPI_FIO_MULTI_MAX_DEV];
    char **blocks;
    unsigned int blocks_count = 0;
    unsigned int size;
    unsigned int i;

    TEST_START;
    TEST_GET_RPCS(host_ta, "pco", host_rpcs);

    CHECK_RC(tsapi_get_devices(host_rpcs->ta, &blocks, &size));
    blocks_count = size;
    if (blocks_count < 2)
        TEST_SKIP("At least 2 virtio block devices are required for the test");

    CHECK_RC(fio_blocks_to_str((const char **)blocks, blocks_count, &str));
    RING("Founded devices: %s", str.ptr);

    CHECK_RC(tsapi_fio_multi_init(host_rpcs, ctx, blocks_count));

    TEST_STEP("Read fio options");
    for (i = 0; i < blocks_count; i++) {
        const char *suffix= i == 0 ? "": "_second";
        tsapi_fio_opts_read_suf(&ctx[i].opts, argc, argv, suffix);
        ctx[i].opts.filename = blocks[i];
    }

    TEST_STEP("Prepare FIO run");
    for (i = 0; i < blocks_count; i++) {
        CHECK_RC(tsapi_fio_create(&ctx[i].opts, ctx[i].forked_factory,
                                  host_ta, &ctx[i].fio));
    }

    TEST_STEP("Start FIO for each device");
    for (i = 0; i < blocks_count; i++) {
        CHECK_RC(tapi_fio_start(ctx[i].fio));
    }

    TEST_STEP("Waiting FIO instances");
    for (i = 0; i < blocks_count; i++) {
        CHECK_RC(tsapi_fio_flex_wait(ctx[i].fio));
        CHECK_RC(tapi_fio_stop(ctx[i].fio));
    }

    TEST_STEP("Prepare report of FIO");
    for (i = 0; i < blocks_count; i++) {
        tapi_fio_report report;

        CHECK_RC_VERDICT(tapi_fio_get_report(ctx[i].fio, &report),
                         "Failed to parse FIO output");
        tapi_fio_log_report(&report);
    }

    TEST_SUCCESS;
cleanup:
    if (blocks_count >= 2)
    {
        for (i = 0; i < blocks_count; i++) {
            tapi_fio_destroy(ctx[i].fio);
            if (blocks != NULL)
                free(blocks[i]);
        }
        if (blocks != NULL)
            free(blocks);
        CLEANUP_CHECK_RC(tsapi_fio_multi_fini(ctx, blocks_count));
    }
    TEST_END;
}
