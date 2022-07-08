/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Run fio under block device with stress test in parallel
 *
 * @defgroup blk-proxy-fio-stability FIO in parallel with stressful actions on the CPU
 * @ingroup virtblk-blk-proxy-readwrite
 *
 * Run fio under block device with stress test in parallel
 *
 * @param ioengine       FIO parameter: I/O Engine type
 * @param rwtype         FIO parameter: Read or write type
 * @param rwmixread      FIO parameter: Percentage of mixed workload that is
 *                                      reads
 * @param numjobs        FIO parameter: Duplicate this job many times
 * @param iodepth        FIO parameter: Number of IO buffers to keep in flight
 * @param blocksize      FIO parameter: Block size unit
 * @param runtime_sec    FIO parameter: Stop workload when the time expires
 * @param stress_ta      Test Agent name to start stress tool:
 *                       - @c Agt_host
 *                       - @c Agt_ARM
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

/**
 * @objective Run fio under block device with bad link
 */

#define TE_TEST_NAME  "blk-proxy/readwrite/fio_stability"

#include "tsapi.h"
#include "tapi_stress.h"
#include "tsapi-fio.h"
#include "tapi_job_factory_rpc.h"

int
main(int argc, char *argv[])
{
    tapi_job_factory_t *fio_factory = NULL;
    rcf_rpc_server *host_rpcs = NULL;
    rcf_rpc_server *stress_rpcs = NULL;
    tapi_job_factory_t *stress_factory = NULL;
    const char *stress_ta;
    const char *host_ta = "Agt_host";
    const char *arm_ta = "Agt_ARM";

    tapi_fio *fio = NULL;
    tapi_fio_report report;
    tapi_fio_opts fio_opts = TAPI_FIO_OPTS_DEFAULTS;

    struct tapi_stress_app *stress_app = NULL;
    tapi_stress_opt stress_opt = tapi_stress_default_opt;

    TEST_START;
    TEST_GET_STRING_PARAM(stress_ta);
    TEST_GET_RPCS(host_ta, "pco", host_rpcs);

    if (strcmp(stress_ta, host_rpcs->ta) == 0)
    {
        stress_rpcs = host_rpcs;
    }
    else if (strcmp(stress_ta, arm_ta) == 0)
    {
        TEST_GET_RPCS(arm_ta, "pco", stress_rpcs);
    }
    else
    {
        TEST_FAIL("Unknown Test Agent name: %s", stress_ta);
    }

    stress_opt.cpu = 0;

    CHECK_RC(tapi_job_factory_rpc_create(stress_rpcs, &stress_factory));
    CHECK_RC(tapi_stress_create(stress_factory, &stress_opt, &stress_app));

    TEST_STEP("Read fio options");
    tsapi_fio_opts_read(&fio_opts, argc, argv, host_rpcs->ta);

    TEST_STEP("Prepare FIO run");
    CHECK_RC(tapi_job_factory_rpc_create(host_rpcs, &fio_factory));
    CHECK_RC(tsapi_fio_create(&fio_opts, fio_factory, host_ta, &fio));

    TEST_STEP("Start stress test for CPU");
    CHECK_RC(tapi_stress_start(stress_app));

    TEST_STEP("Start FIO and wait for completion");
    CHECK_RC(tapi_fio_start(fio));
    CHECK_RC(tsapi_fio_flex_wait(fio));
    CHECK_RC(tapi_fio_stop(fio));
    CHECK_RC(tapi_stress_stop(stress_app, TAPI_STRESS_DEFAULT_TERM_TIMEOUT_MS));

    TEST_STEP("Get FIO report and log it");
    CHECK_RC_VERDICT(tapi_fio_get_report(fio, &report),
                     "Failed to parse FIO output");
    tapi_fio_log_report(&report);

    TEST_SUCCESS;
cleanup:
    tapi_fio_destroy(fio);
    tapi_stress_destroy(stress_app);
    tapi_job_factory_destroy(fio_factory);
    TEST_END;
}
