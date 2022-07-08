/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Network stimuli test
 *
 * @defgroup blk-proxy-fio_netem I/O with network problems.
 * @grouporder{2}
 * @ingroup virtblk-blk-proxy-network-stimuli
 *
 * Run FIO tool and check its behaviour with various network corruption.
 *
 * @param env            Network environment configuration:
 *                       - @c network
 * @param ioengine       FIO parameter: I/O Engine type
 * @param rwtype         FIO parameter: Read or write type
 * @param rwmixread      FIO parameter: Percentage of mixed workload that is
 *                                      reads
 * @param numjobs        FIO parameter: Duplicate this job many times
 * @param iodepth        FIO parameter: Number of IO buffers to keep in flight
 * @param blocksize      FIO parameter: Block size unit
 * @param runtime_sec    FIO parameter: Stop workload when the time expires
 * @param action1        Netem action:
 *                       - @c delay - @p param1 ms delay
 *                       - @c loss - loss @p param1 percent of packets
 *                       - @c duplicate - duplicate @p param1 percent of packets
 *                       - @c limit @p param1 number of packets the qdisc may
 *                                  hold queued at a time
 *                       - @c gap - reorder every @p param1 packet
 *                       - @c reorder - reorder @p param1 percent of packets
 *                       - @c corruption - corrupt  @p param1 percent of packets
 *                       - @c none - Do not do @p action1
 * @param param1         Parameter for the @c action1
 * @param correlation1   Value of the correlation in percent for @c action1
 * @param action2        Netem action:
 *                       - @c delay - @p param2 ms delay
 *                       - @c loss - loss @p param2 percent of packets
 *                       - @c duplicate - duplicate @p param2 percent of packets
 *                       - @c limit @p param2 number of packets the qdisc may
 *                                  hold queued at a time
 *                       - @c gap - reorder every @p param2 packet
 *                       - @c reorder - reorder @p param2 percent of packets
 *                       - @c corruption - corrupt  @p param2 percent of packets
 *                       - @c none - Do not do @p param2
 * @param param2         Parameter for the @c action2
 * @param correlation2   Value of the correlation in percent for @c action2
 *
 * @author Artemii Morozov <Artemii.Morozov@arknetworks.am>
 */

#define TE_TEST_NAME "blk-proxy/network_stimuli/fio_netem"

#include "tsapi.h"
#include "tapi_cfg_qdisc.h"
#include "tapi_cfg_netem.h"
#include "tsapi-fio.h"
#include "tapi_job_factory_rpc.h"

typedef enum {
    NETEM_ACTION_DELAY,
    NETEM_ACTION_LOSS,
    NETEM_ACTION_DUPLICATE,
    NETEM_ACTION_LIMIT,
    NETEM_ACTION_GAP,
    NETEM_ACTION_REORDER,
    NETEM_ACTION_CORRUPTION,
    NETEM_ACTION_NONE
} netem_action;

#define NETEM_ACTION_MAP \
    { "delay", NETEM_ACTION_DELAY }, \
    { "loss", NETEM_ACTION_LOSS }, \
    { "duplicate", NETEM_ACTION_DUPLICATE }, \
    { "limit", NETEM_ACTION_LIMIT }, \
    { "gap", NETEM_ACTION_GAP }, \
    { "reorder", NETEM_ACTION_REORDER }, \
    { "corruption", NETEM_ACTION_CORRUPTION }, \
    { "none", NETEM_ACTION_NONE }

te_errno (* netem_correlation_func[])(const char *, const char *, double) = {
    [NETEM_ACTION_DELAY] = tapi_cfg_netem_set_delay_correlation,
    [NETEM_ACTION_LOSS] = tapi_cfg_netem_set_loss_correlation,
    [NETEM_ACTION_DUPLICATE] = tapi_cfg_netem_set_duplicate_correlation,
    [NETEM_ACTION_LIMIT] = NULL,
    [NETEM_ACTION_GAP] = NULL,
    [NETEM_ACTION_REORDER] = tapi_cfg_netem_set_reorder_correlation,
    [NETEM_ACTION_CORRUPTION] = tapi_cfg_netem_set_corruption_correlation
};

static void
set_netem_action_param(const char *ta, const char *if_name,
                       netem_action action, double param, double correlation)
{
    if (correlation != 0)
    {
        if (netem_correlation_func[action] == NULL)
            TEST_FAIL("netem does not support correlation with this action");

        CHECK_RC((*netem_correlation_func[action])(ta, if_name, correlation));
    }

    switch (action)
    {
        case NETEM_ACTION_DELAY:
            CHECK_RC(tapi_cfg_netem_set_delay(ta, if_name,
                                              TE_MS2US((unsigned int)param)));
            break;

        case NETEM_ACTION_LOSS:
            CHECK_RC(tapi_cfg_netem_set_loss(ta, if_name, param));
            break;

        case NETEM_ACTION_DUPLICATE:
            CHECK_RC(tapi_cfg_netem_set_duplicate(ta, if_name, param));
            break;

        case NETEM_ACTION_LIMIT:
            CHECK_RC(tapi_cfg_netem_set_limit(ta, if_name,
                                              (unsigned int)param));
            break;

        case NETEM_ACTION_GAP:
            CHECK_RC(tapi_cfg_netem_set_gap(ta, if_name, (unsigned int)param));
            break;

        case NETEM_ACTION_REORDER:
            CHECK_RC(tapi_cfg_netem_set_reorder_probability(ta, if_name,
                                                            param));
            break;

        case NETEM_ACTION_CORRUPTION:
            CHECK_RC(tapi_cfg_netem_set_corruption_probability(ta, if_name,
                                                               param));
            break;

        default:
            TEST_FAIL("Unsupported netem action");
    }
}

int
main(int argc, char *argv[])
{
    rcf_rpc_server *rpcs_disk;
    rcf_rpc_server *rpcs_host;
    const char *disk_ta = "Agt_disk";
    const char *host_ta = "Agt_host";
    const struct if_nameindex *srv_if = NULL;

    double param1;
    double correlation1;
    netem_action action1;
    double param2;
    double correlation2;
    netem_action action2;
    tapi_env env;
    tapi_job_factory_t *fio_factory = NULL;
    tapi_fio *fio = NULL;
    tapi_fio_report report;
    tapi_fio_opts fio_opts = TAPI_FIO_OPTS_DEFAULTS;

    TEST_START;
    TEST_START_ENV;
    TEST_GET_RPCS(host_ta, "pco", rpcs_host);
    TEST_GET_RPCS(disk_ta, "pco", rpcs_disk);
    TEST_GET_IF(srv_if);
    TEST_GET_ENUM_PARAM(action1, NETEM_ACTION_MAP);
    TEST_GET_DOUBLE_PARAM(correlation1);
    TEST_GET_DOUBLE_PARAM(param1);
    TEST_GET_ENUM_PARAM(action2, NETEM_ACTION_MAP);
    TEST_GET_DOUBLE_PARAM(correlation2);
    TEST_GET_DOUBLE_PARAM(param2);

    if (action1 == NETEM_ACTION_NONE)
        TEST_FAIL("The first action cannot be none");

    if (param1 < 0 || param2 < 0)
        TEST_FAIL("'param1' and 'param2' should be non-negative value");

    CHECK_RC(tapi_cfg_qdisc_set_kind(disk_ta, srv_if->if_name,
                                     TAPI_CFG_QDISC_KIND_NETEM));

    TEST_STEP("Set @p param1 for @p action1 and @p correlation1 if needed");
    set_netem_action_param(disk_ta, srv_if->if_name, action1,
                           param1, correlation1);

    if (action2 != NETEM_ACTION_NONE)
    {
        TEST_STEP("Set @p param2 for @p action2 and @p correlation2 if needed");
        set_netem_action_param(disk_ta, srv_if->if_name, action2, param2,
                               correlation2);
    }

    CHECK_RC(tapi_cfg_qdisc_enable(disk_ta, srv_if->if_name));

    TEST_STEP("Read fio options");
    tsapi_fio_opts_read(&fio_opts, argc, argv, rpcs_host->ta);

    TEST_STEP("Prepare FIO run");
    CHECK_RC(tapi_job_factory_rpc_create(rpcs_host, &fio_factory));
    CHECK_RC(tsapi_fio_create(&fio_opts, fio_factory, host_ta, &fio));

    TEST_STEP("Start FIO and wait for completion");
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
    tapi_job_factory_destroy(fio_factory);
    TEST_END;
}
