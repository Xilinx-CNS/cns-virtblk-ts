/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Restart block proxy
 *
 * @defgroup blk-proxy-config-restart-blk-proxy Block proxy restarting
 * @ingroup virtblk-blk-proxy-config
 *
 * Restart the block proxy a several times with or without
 * rebinding of a driver
 *
 * @param vendor_id    Vendor's PCI id
 * @param device_id    Devices's PCI id
 * @param with_rebind  If @c TRUE, the driver will be unbinded before
 *                     restarting and bound after restarting
 * @param iter_num     Number of test iterations
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */

#define TE_TEST_NAME  "blk-proxy/config/restart_blk_proxy"

#include "tsapi.h"
#include "te_vector.h"
#include "tapi_cfg_process.h"
#include "tsapi-blkproxy.h"

static void
bind_all_drivers(const char *ta, const char *vendor_id, const char *device_id,
                 te_vec *drivers)
{
    size_t i;
    size_t drivers_size;

    drivers_size = te_vec_size(drivers);

    for (i = 0; i < drivers_size; i++)
    {
        CHECK_RC(tapi_cfg_pci_bind_driver_by_vend_dev_inst(ta, vendor_id,
                                            device_id, i,
                                            TE_VEC_GET(char *, drivers, i)));
    }
}

static void
unbind_all_drivers(const char *ta, const char *vendor_id, const char *device_id,
                   te_vec *drivers)
{
    size_t i;
    size_t drivers_size;

    drivers_size = te_vec_size(drivers);

    for (i = 0; i < drivers_size; i++)
    {
        CHECK_RC(tapi_cfg_pci_unbind_driver_by_vend_dev_inst(ta, vendor_id,
                                                             device_id, i));
    }
}

int
main(int argc, char *argv[])
{
    const char *arm_ta = "Agt_ARM";
    const char *host_ta = "Agt_host";
    rcf_rpc_server *rpcs_arm = NULL;
    rcf_rpc_server *rpcs_host = NULL;

    const char *vendor_id;
    const char *device_id;
    te_bool with_rebind;
    unsigned int iter_num;

    tsapi_blkproxy_handle *bp_handle = NULL;

    unsigned int block_dev_count;
    unsigned int i;
    unsigned int j;
    te_vec drivers = TE_VEC_INIT(char *);
    char *driver = NULL;
    te_bool status;
    unsigned int autorestart_value;

    TEST_START;
    TEST_GET_RPCS(arm_ta, "pco", rpcs_arm);
    TEST_GET_RPCS(host_ta, "pco", rpcs_host);
    TEST_GET_STRING_PARAM(vendor_id);
    TEST_GET_STRING_PARAM(device_id);
    TEST_GET_BOOL_PARAM(with_rebind);
    TEST_GET_UINT_PARAM(iter_num);

    TEST_STEP("Initialize default block proxy handler");
    CHECK_RC(tsapi_blkproxy_handle_init(&bp_handle, rpcs_arm, NULL, NULL));

    for (i = 0; i < iter_num; i++)
    {
        TEST_STEP("Check that block proxy process is started");
        CHECK_RC(tsapi_blkproxy_get_status(bp_handle, &status));
        if (!status)
        {
            TEST_FAIL("%s process must be started before running the test",
                      bp_handle->ps_name);
        }
        block_dev_count = tsapi_virtio_blocks_count(rpcs_host->ta);

        if (with_rebind)
        {
            TEST_STEP("Unbind the driver on the all block devices");
            for (j = 0; j < block_dev_count; j++)
            {
                CHECK_RC(tapi_cfg_pci_get_driver_by_vend_dev_inst(
                                                                rpcs_host->ta,
                                                                vendor_id,
                                                                device_id,
                                                                j, &driver));
                if (*driver == '\0')
                    TEST_FAIL("Could not get current driver of a device");

                CHECK_RC(TE_VEC_APPEND(&drivers, driver));
            }

            unbind_all_drivers(rpcs_host->ta, vendor_id, device_id, &drivers);
        }

        TEST_STEP("Check that auto-restart for block proxy is disabled");
        CHECK_RC(tapi_cfg_ps_get_autorestart(bp_handle->rpcs->ta,
                                             bp_handle->ps_name,
                                             &autorestart_value));
        if (autorestart_value != 0)
        {
            TEST_SUBSTEP("Disable autorestart feature for block proxy"
                         " if it was enabled");
            CHECK_RC(tapi_cfg_ps_set_autorestart(bp_handle->rpcs->ta,
                                                 bp_handle->ps_name,
                                                 0));
        }

        TEST_STEP("Stop block proxy");
        CHECK_RC(tsapi_blkproxy_stop(bp_handle));

        TEST_STEP("Check that blk-proxy process is stopped");
        CHECK_RC(tsapi_blkproxy_get_status(bp_handle, &status));
        if (status)
            TEST_FAIL("%s process is not stopped", bp_handle->ps_name);

        TEST_STEP("Restart block proxy");
        CHECK_RC(tsapi_blkproxy_start(bp_handle));

        if (with_rebind)
        {
            TEST_STEP("Bind the driver on the all block devices"
                      " if @p with_rebind is @c TRUE");
            bind_all_drivers(rpcs_host->ta, vendor_id, device_id, &drivers);
            te_motivated_sleep(1, "Waiting to successfully bind");
            te_vec_reset(&drivers);
        }
    }

    TEST_SUCCESS;

cleanup:
    if (autorestart_value != 0)
    {
        CHECK_RC(tapi_cfg_ps_set_autorestart(bp_handle->rpcs->ta,
                                             bp_handle->ps_name,
                                             autorestart_value));
    }

    te_vec_deep_free(&drivers);
    tsapi_blkproxy_handle_free(bp_handle);
    TEST_END;
}
