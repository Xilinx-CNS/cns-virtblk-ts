/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Run fio under block devices to check performance
 *
 * @defgroup blk-proxy-performance-perf-fio Performance testing using FIO tool
 * @grouporder{3}
 * @ingroup virtblk-blk-proxy-performance
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 *
 * Run FIO tool to finger out the performance on block devices
 */

#define TE_TEST_NAME  "blk-proxy/performace/fio_perf"

#include "tsapi.h"
#include "tsapi-fio.h"
#include "tapi_job_factory_rpc.h"
#include "lib/performance_lib.h"

int
main(int argc, char *argv[])
{
    tapi_job_factory_t *fio_factory = NULL;
    rcf_rpc_server *host_rpcs = NULL;
    const char *host_ta = "Agt_host";
    const char *arm_ta = "Agt_ARM";

    tapi_fio *fio = NULL;
    tapi_fio_report report;
    tapi_fio_opts fio_opts = TAPI_FIO_OPTS_DEFAULTS;

    TEST_START;
    TEST_GET_RPCS(host_ta, "pco", host_rpcs);

    TEST_STEP("Read fio options");
    tsapi_fio_opts_read(&fio_opts, argc, argv, host_rpcs->ta);

    TEST_STEP("Prepare FIO run");
    CHECK_RC(tapi_job_factory_rpc_create(host_rpcs, &fio_factory));
    CHECK_RC(tsapi_fio_create(&fio_opts, fio_factory, host_ta, &fio));

    TEST_STEP("Start FIO and wait for completion");
    CHECK_RC(tapi_fio_start(fio));
    CHECK_RC(tsapi_fio_flex_wait(fio));
    CHECK_RC(tapi_fio_stop(fio));

    TEST_STEP("Get FIO report and log it");
    CHECK_RC_VERDICT(tapi_fio_get_report(fio, &report),
                     "Failed to parse FIO output");
    tapi_fio_log_report(&report);

    CHECK_RC(tsapi_fio_mi_report(fio, &report,
                                 PERFORMANCE_LIB_BLK_PROXY_CONF_NAME,
                                 arm_ta, FALSE));

    TEST_SUCCESS;
cleanup:
    tapi_fio_destroy(fio);
    tapi_job_factory_destroy(fio_factory);
    TEST_END;
}
