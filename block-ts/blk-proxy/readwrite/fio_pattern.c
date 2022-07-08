/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Read-write test validating data pattern written to the device
 *
 * @defgroup blk-proxy-fio-pattern Read-write test validating data pattern written to the device
 * @ingroup virtblk-blk-proxy-readwrite
 *
 * Run fio under block device with pattern check mode, i.e data validation
 *
 * @param ioengine       FIO parameter: I/O Engine type
 * @param rwtype         FIO parameter: Read or write type
 * @param rwmixread      FIO parameter: Percentage of mixed workload that is
 *                                      reads
 * @param numjobs        FIO parameter: Duplicate this job many times
 * @param iodepth        FIO parameter: Number of IO buffers to keep in flight
 * @param blocksize      FIO parameter: Block size unit
 * @param runtime_sec    FIO parameter: Stop workload when the time expires
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

/**
 * @objective Run fio under block device with pattern check mode, i.e data
 *            pattern is written to the device and is validated afterwards
 */

#define TE_TEST_NAME  "blk-proxy/readwrite/fio_pattern"

#include "tsapi.h"
#include "tsapi-fio.h"
#include "tapi_job_factory_rpc.h"

static void
get_common_verify_opts(te_string *vopts, tapi_fio_opts *opts)
{
    te_string_reset(vopts);
    CHECK_RC(te_string_append(vopts, " --verify=md5"));
    CHECK_RC(te_string_append(vopts, " --verify_interval=%d", opts->blocksize));
}

static void
get_writter_verify_opts(te_string *vopts, tapi_fio_opts *opts)
{
    get_common_verify_opts(vopts, opts);
    CHECK_RC(te_string_append(vopts, " --do_verify=0"));
    CHECK_RC(te_string_append(vopts, " --verify_state_save=1"));
    CHECK_RC(te_string_append(vopts, " --rw=randwrite"));

    if (opts->iodepth > 1)
    {
        CHECK_RC(te_string_append(vopts, " --serialize_overlap=1"));
        CHECK_RC(te_string_append(vopts, " --overwrite=1"));
    }

    opts->user = vopts->ptr;
}

static void
get_verification_opts(te_string *vopts, tapi_fio_opts *opts)
{
    get_common_verify_opts(vopts, opts);
    CHECK_RC(te_string_append(vopts, " --verify_state_load=1"));
    CHECK_RC(te_string_append(vopts, " --do_verify=1"));
    CHECK_RC(te_string_append(vopts, " --verify_fatal=1"));
    CHECK_RC(te_string_append(vopts, " --verify_dump=1"));
    CHECK_RC(te_string_append(vopts, " --rw=randread"));
    opts->rwmixread = 100;
    opts->user = vopts->ptr;
}

int
main(int argc, char *argv[])
{
    tapi_job_factory_t *fio_factory = NULL;
    rcf_rpc_server *host_rpcs = NULL;
    tapi_fio *fio = NULL;
    tapi_fio_opts opts = TAPI_FIO_OPTS_DEFAULTS;
    tapi_fio_report report;
    te_string verify_opts = TE_STRING_INIT;
    const char *host_ta = "Agt_host";

    TEST_START;
    TEST_GET_RPCS(host_ta, "pco", host_rpcs);

    TEST_STEP("Prepare FIO run");
    tsapi_fio_opts_read(&opts, argc, argv, host_rpcs->ta);
    CHECK_RC(tapi_job_factory_rpc_create(host_rpcs, &fio_factory));
    CHECK_RC(tapi_job_factory_set_path(fio_factory));
    CHECK_RC(tsapi_fio_create(&opts, fio_factory, host_ta, &fio));

    TEST_STEP("Start FIO as writter");
    get_writter_verify_opts(&verify_opts, &(fio->app.opts));
    CHECK_RC(tapi_fio_start(fio));
    CHECK_RC(tsapi_fio_flex_wait(fio));
    CHECK_RC(tapi_fio_stop(fio));

    TEST_STEP("Get FIO writter report and log it");
    CHECK_RC_VERDICT(tapi_fio_get_report(fio, &report),
                     "Failed to parse FIO output");

    TEST_STEP("Start FIO for verificaion");
    get_verification_opts(&verify_opts, &(fio->app.opts));
    CHECK_RC(tapi_fio_start(fio));
    CHECK_RC(tsapi_fio_flex_wait(fio));
    CHECK_RC(tapi_fio_stop(fio));

    TEST_STEP("Get FIO report and log it");
    CHECK_RC_VERDICT(tapi_fio_get_report(fio, &report),
                     "Failed to parse FIO output");

    tapi_fio_log_report(&report);

    TEST_SUCCESS;

cleanup:
    tapi_fio_destroy(fio);
    te_string_free(&verify_opts);
    tapi_job_factory_destroy(fio_factory);

    TEST_END;
}

