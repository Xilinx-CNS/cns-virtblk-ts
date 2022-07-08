/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief FIO in parallel with network traffic running on the ARM
 *
 * @defgroup blk-proxy-fio-stability-perf FIO in parallel with network traffic running on the ARM
 * @ingroup virtblk-blk-proxy-readwrite
 *
 * Run fio under block device with parallel net traffic
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
 * @objective The test is trying to make sure network traffic that is not
 *            related to CEPH or other subsystems working on the ARM.
 */

#define TE_TEST_NAME  "blk-proxy/readwrite/fio_stability_perf"

#include "tsapi.h"
#include "tsapi-fio.h"
#include "tapi_performance.h"
#include "tapi_job_factory_rpc.h"

int
main(int argc, char *argv[])
{
    tapi_job_factory_t *fio_factory = NULL;
    const char *host_ta = "Agt_host";
    const char *arm_ta = "Agt_ARM";
    rcf_rpc_server *host_rpcs = NULL;
    rcf_rpc_server *arm_rpcs = NULL;

    tapi_fio *fio = NULL;
    tapi_fio_report report;
    tapi_fio_opts fio_opts = TAPI_FIO_OPTS_DEFAULTS;

    tapi_perf_client *perf_client = NULL;
    tapi_perf_server *perf_server = NULL;
    tapi_perf_opts perf_client_opts;
    tapi_perf_opts perf_server_opts;
    tapi_perf_report perf_client_report;
    tapi_perf_report perf_server_report;

    tapi_job_factory_t *perf_client_factory;
    tapi_job_factory_t *perf_server_factory;

    TEST_START;
    TEST_GET_RPCS(host_ta, "pco", host_rpcs);
    TEST_GET_RPCS(arm_ta, "pco", arm_rpcs);

    TEST_STEP("Read fio options");
    tsapi_fio_opts_read(&fio_opts, argc, argv, host_rpcs->ta);

    TEST_STEP("Prepare iperf tool");
    CHECK_RC(tapi_job_factory_rpc_create(host_rpcs, &perf_client_factory));
    CHECK_RC(tapi_job_factory_rpc_create(arm_rpcs, &perf_server_factory));

    tapi_perf_opts_init(&perf_client_opts);
    tapi_perf_opts_init(&perf_server_opts);

    perf_client_opts.host = "127.0.0.1";
    perf_client_opts.duration_sec = fio_opts.runtime_sec;

    perf_server_opts.protocol = perf_client_opts.protocol = RPC_IPPROTO_TCP;
    perf_server_opts.streams = perf_client_opts.streams = 2;
    perf_server_opts.port = perf_client_opts.port = 5201;

    perf_client = tapi_perf_client_create(TAPI_PERF_IPERF3, &perf_client_opts,
                                          perf_client_factory);
    perf_server = tapi_perf_server_create(TAPI_PERF_IPERF3, &perf_server_opts,
                                          perf_server_factory);

    TEST_STEP("Prepare FIO run");
    CHECK_RC(tapi_job_factory_rpc_create(host_rpcs, &fio_factory));
    CHECK_RC(tsapi_fio_create(&fio_opts, fio_factory, host_ta, &fio));

    TEST_STEP("Start iperf tool");
    CHECK_RC(tapi_perf_server_start(perf_server));
    CHECK_RC(tapi_perf_client_start(perf_client));

    TEST_STEP("Start FIO and wait for completion");
    CHECK_RC(tapi_fio_start(fio));
    CHECK_RC(tsapi_fio_flex_wait(fio));
    CHECK_RC(tapi_fio_stop(fio));

    CHECK_RC(tapi_perf_client_wait(perf_client, TAPI_PERF_TIMEOUT_DEFAULT));

    CHECK_RC(tapi_perf_client_get_report(perf_client, &perf_client_report));
    CHECK_RC(tapi_perf_server_get_report(perf_server, &perf_server_report));

    TEST_STEP("Get FIO report and log it");
    CHECK_RC_VERDICT(tapi_fio_get_report(fio, &report),
                     "Failed to parse FIO output");
    tapi_fio_log_report(&report);

    TEST_SUCCESS;
cleanup:
    tapi_fio_destroy(fio);
    tapi_perf_client_destroy(perf_client);
    tapi_perf_server_destroy(perf_server);
    tapi_job_factory_destroy(perf_client_factory);
    tapi_job_factory_destroy(perf_server_factory);
    tapi_job_factory_destroy(fio_factory);
    TEST_END;
}
