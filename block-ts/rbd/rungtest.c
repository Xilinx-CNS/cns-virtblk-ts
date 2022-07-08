/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Block Test Suite
 *
 * @defgroup rbd-rungtest Start RBD test over gtest
 * @ingroup virtblk-rbd
 * @{
 *
 * Block Test Suite
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

/** @page rbd-rbd GDB Test Suite Runner
 *
 * @objective Run a specific test for librdb CEPH self-test
 *
 * @par Scenario
 */

#define TE_TEST_NAME        "rungtest"
#define GTEST_RBD_MAX_TIME  TE_SEC2MS(30 * 60)

#include "block-ts.h"

int
main(int argc, char *argv[])
{
    rcf_rpc_server *rpcs = NULL;
    tapi_job_factory_t *factory = NULL;
    tapi_gtest gtest = TAPI_GTEST_DEFAULTS;

    TEST_START;
    TEST_GET_RPCS("Agt_ARM", "pco", rpcs);
    TEST_GET_GTEST_PARAM(gtest);

    TEST_STEP("Prepare test ceph_test_librbd");
    if (strcmp(gtest.bin, "ceph_test_librbd") == 0)
    {
        TEST_SKIP("Non-mocking testing turned off");
    }
    gtest.bin = block_ts_gtest_absbin(gtest.bin);

    TEST_STEP("Setup features requered for start ceph_test_librbd");
    CHECK_RC(block_ts_gtest_setenv(rpcs, argc, argv));

    CHECK_RC(tapi_job_factory_rpc_create(rpcs, &factory));
    CHECK_RC(tapi_gtest_init(&gtest, factory));
    CHECK_RC(block_ts_log_conv_add(gtest.impl.out[0], (block_ts_log_conv) {
        .re = "^SKIPPING",
        .action = BLOCK_TS_LOG_CONV_TEST_SKIP
    }));

    TEST_STEP("Start ceph_test_librbd");
    CHECK_RC(tapi_gtest_start(&gtest));
    CHECK_RC(tapi_gtest_wait(&gtest, GTEST_RBD_MAX_TIME));

    TEST_STEP("Convert gtest logs to TE logs");
    CHECK_RC(block_ts_log_conv_run());

    TEST_STEP("Stop ceph_test_librbd");
    CHECK_RC(tapi_gtest_stop(&gtest));

    TEST_SUCCESS;
cleanup:
    free((void *)gtest.bin);
    CLEANUP_CHECK_RC(tapi_gtest_fini(&gtest));
    tapi_job_factory_destroy(factory);
    TEST_END;
}

/**@} <!-- END rbd-rungtest --> */
