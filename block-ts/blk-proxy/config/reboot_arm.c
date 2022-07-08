/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Reboot ARM host
 *
 * @defgroup blk-proxy-config-reboot-arm ARM host reboot
 * @ingroup virtblk-blk-proxy-config
 *
 * Reboot ARM host
 *
 * @param reboot_after_fio If @c TRUE: Do ARM reboot after FIO completion
 *                         If @c FALSE: Do ARM reboot between FIO start and FIO end
 * @param count_iter       Number of test iterations
 *
 * @author Artemii Morozov <Artemii.Morozov@arknetworks.am>
 */

#define TE_TEST_NAME  "blk-proxy/config/reboot_arm"

#include "tsapi.h"
#include "tsapi-fio.h"

int
main(int argc, char *argv[])
{
    const char *host_ta = "Agt_host";
    const char *arm_ta = "Agt_ARM";
    rcf_rpc_server *host_rpcs = NULL;

    tapi_job_factory_t *fio_factory = NULL;
    tapi_fio *fio = NULL;
    tapi_fio_opts fio_opts = TAPI_FIO_OPTS_DEFAULTS;
    tapi_fio_report report;

    te_bool reboot_after_fio;
    int count_iter;

    int i;

    TEST_START;
    TEST_GET_RPCS(host_ta, "pco", host_rpcs);
    TEST_GET_BOOL_PARAM(reboot_after_fio);
    TEST_GET_INT_PARAM(count_iter);

    tsapi_fio_opts_read(&fio_opts, argc, argv, host_rpcs->ta);

    CHECK_RC(tapi_job_factory_rpc_create(host_rpcs, &fio_factory));
    CHECK_RC(tsapi_fio_create(&fio_opts, fio_factory, host_ta, &fio));

    for (i = 0; i < count_iter; i++)
    {
        TEST_STEP("Start fio");
        CHECK_RC(tapi_fio_start(fio));

        if (!reboot_after_fio)
        {
            TEST_STEP("Reboot ARM host");
            CHECK_RC(cfg_reboot_ta(arm_ta, TRUE, RCF_REBOOT_TYPE_WARM));
        }

        TEST_STEP("Wait and stop fio");
        CHECK_RC(tsapi_fio_flex_wait(fio));
        CHECK_RC(tapi_fio_stop(fio));

        if (reboot_after_fio)
        {
            TEST_STEP("Reboot ARM host");
            CHECK_RC(cfg_reboot_ta(arm_ta, TRUE, RCF_REBOOT_TYPE_WARM));
        }

        tapi_fio_get_report(fio, &report);
        tapi_fio_log_report(&report);
    }

    TEST_SUCCESS;

cleanup:
    tapi_fio_destroy(fio);
    tapi_job_factory_destroy(fio_factory);
    TEST_END;
}
