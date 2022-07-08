/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Stress testing
 *
 * @defgroup blk-proxy-stress-stress
 * @ingroup virtblk-blk-proxy-stress
 *
 * Run FIO tool and check behaviour with varios stress actions.
 *
 * @p action Action to be tested with data infligth:
 *           - @c unbind     Unbind virtio-pci driver
 *           - @c drop_link  Drop link between ARM and partner host
 *           - @c unload_sfc Unload SFC module on the ARM
 * @p reverse If @c TRUE do opposite action to @p action
 * @p env          Network environment configuration (only for @c drop_link)
 *
 * @author Artemii Morozov <Artemii.Morozov@arknetworks.am>
 */

#define TE_TEST_NAME  "blk-proxy/stress/stress"

#include "tsapi.h"
#include "tsapi-fio.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_modules.h"

typedef enum {
    STRESS_ACTION_UNBIND,
    STRESS_ACTION_DROP_LINK_ARM,
    STRESS_ACTION_DROP_LINK_PARTNER,
    STRESS_ACTION_UNLOAD_SFC,
} stress_action;

#define STRESSS_ACTION_MAPPING_LIST \
    {"unbind", STRESS_ACTION_UNBIND}, \
    {"drop_link_arm", STRESS_ACTION_DROP_LINK_ARM}, \
    {"drop_link_partner", STRESS_ACTION_DROP_LINK_PARTNER}, \
    {"unload_sfc", STRESS_ACTION_UNLOAD_SFC}

/* Default timeout between opposite actions */
#define TIMEOUT_BETWEEN_REVERSE_ACTION_S 3

int
main(int argc, char *argv[])
{
    const char *host_ta = "Agt_host";
    const char *arm_ta = "Agt_ARM";
    const char *disk_ta = "Agt_disk";
    rcf_rpc_server *host_rpcs = NULL;

    char **blocks;
    unsigned int blocks_count = 0;
    tsapi_fio_context ctx[TSAPI_FIO_MULTI_MAX_DEV];

    stress_action action;
    const struct if_nameindex *clt_if = NULL;
    const struct if_nameindex *srv_if = NULL;
    tapi_env env;
    te_bool reverse;

    unsigned int i;

    TEST_START;
    TEST_GET_RPCS(host_ta, "pco", host_rpcs);
    TEST_GET_ENUM_PARAM(action, STRESSS_ACTION_MAPPING_LIST);
    TEST_GET_BOOL_PARAM(reverse);

    CHECK_RC(tsapi_get_devices(host_rpcs->ta, &blocks, &blocks_count));
    CHECK_RC(tsapi_fio_multi_init(host_rpcs, ctx, blocks_count));

    for (i = 0; i < blocks_count; i++)
    {
        tsapi_fio_opts_read_suf(&ctx[i].opts, argc, argv, "");
        ctx[i].opts.filename = blocks[i];
    }

    for (i = 0; i < blocks_count; i++)
    {
        CHECK_RC(tsapi_fio_create(&ctx[i].opts, ctx[i].forked_factory,
                                  host_ta, &ctx[i].fio));
    }

    TEST_STEP("Start fio");
    for (i = 0; i < blocks_count; i++)
        CHECK_RC(tapi_fio_start(ctx[i].fio));

    te_motivated_sleep(3, "Waiting to successfully start fio");

    switch (action)
    {
        case STRESS_ACTION_UNBIND:
            TEST_STEP("Unbind the driver on the all block devices");
            tsapi_virtio_blocks_bind_driver(host_rpcs, "");
            if (reverse)
            {
                te_motivated_sleep(TIMEOUT_BETWEEN_REVERSE_ACTION_S,
                                   "Sleep between bind driver");
                TEST_STEP("Bind the driver on the all block devices");
                tsapi_virtio_blocks_bind_driver(host_rpcs,
                                                TSAPI_VIRTIO_BLOCK_DRIVER_NAME);
            }
            break;

        case STRESS_ACTION_DROP_LINK_ARM:
        case STRESS_ACTION_DROP_LINK_PARTNER:
        {
            TEST_START_ENV;
            const char *ifname = NULL;
            const char *ta = NULL;
            te_bool is_arm = (action == STRESS_ACTION_DROP_LINK_ARM) ?
                             TRUE : FALSE;

            if (is_arm)
            {
                TEST_GET_IF(clt_if);
                ifname = clt_if->if_name;
                ta = arm_ta;
                TEST_STEP("Down the interface on the ARM");
            }
            else
            {
                TEST_GET_IF(srv_if);
                ifname = srv_if->if_name;
                ta = disk_ta;
                TEST_STEP("Down the interface on the partner");
            }

            CHECK_RC(tapi_cfg_base_if_down(ta, ifname));
            if (reverse)
            {
                te_motivated_sleep(TIMEOUT_BETWEEN_REVERSE_ACTION_S,
                                   "Sleep between bind driver");
                TEST_STEP("Up the interface that was previously down");
                CHECK_RC(tapi_cfg_base_if_up(ta, ifname));
            }
            break;
        }

        case STRESS_ACTION_UNLOAD_SFC:
            TEST_STEP("Unload and load sfc module");
            CHECK_RC(tapi_cfg_module_unload(arm_ta, "sfc"));
            if (reverse)
            {
                te_motivated_sleep(TIMEOUT_BETWEEN_REVERSE_ACTION_S,
                                   "Sleep between bind driver");
                TEST_STEP("Load sfc module");
                CHECK_RC(tapi_cfg_module_load(arm_ta, "sfc"));
            }
            break;

        default:
            TEST_FAIL("Unsupported action");
    }

    TEST_STEP("Wait and stop fio");
    for (i = 0; i < blocks_count; i++)
    {
        CHECK_RC(tsapi_fio_flex_wait(ctx[i].fio));
        CHECK_RC(tapi_fio_stop(ctx[i].fio));
    }

    for (i = 0; i < blocks_count; i++)
    {
        tapi_fio_report report;

        CHECK_RC_VERDICT(tapi_fio_get_report(ctx[i].fio, &report),
                         "Failed to parse FIO output");
        tapi_fio_log_report(&report);
    }

    if (reverse)
    {
        TEST_STEP("Start FIO again and make sure that it's successful"
                  "after all the actions");
        for (i = 0; i < blocks_count; i++)
            CHECK_RC(tapi_fio_start(ctx[i].fio));

        for (i = 0; i < blocks_count; i++)
        {
            CHECK_RC(tsapi_fio_flex_wait(ctx[i].fio));
            CHECK_RC(tapi_fio_stop(ctx[i].fio));
        }

        for (i = 0; i < blocks_count; i++)
        {
            tapi_fio_report report;

            CHECK_RC_VERDICT(tapi_fio_get_report(ctx[i].fio, &report),
                             "Failed to parse FIO output");
            tapi_fio_log_report(&report);
        }
    }

    TEST_SUCCESS;

cleanup:
    for (i = 0; i < blocks_count; i++)
    {
        tapi_fio_destroy(ctx[i].fio);
        if (blocks != NULL)
            free(blocks[i]);
    }
    if (blocks != NULL)
        free(blocks);
    tsapi_fio_multi_fini(ctx, blocks_count);
    TEST_END;
}
