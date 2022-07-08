/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Block Proxy test epilogue
 *
 * @defgroup blk-proxy-epilogue Block Proxy tests epilogue
 * @grouporder{2}
 * @ingroup virtblk-blk-proxy-bootstrap
 * @{
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_TEST_NAME "epilogue"

#include "tsapi.h"
#include "tapi_cfg_modules.h"
#include "tapi_rpc.h"
#include "tapi_sh_env.h"
#include "tapi_cfg_process.h"
#include "block-ts.h"
#include "tsapi-plugin.h"
#include "tsapi-blkproxy.h"
#include "tsapi-ceph.h"
#include "tsapi-backend.h"

/**
 * Finish uart logger process.
 *
 * @param ta      Test agent name
 */
static void
finish_uart_logger(const char *ta)
{
    te_errno rc;
    te_bool status;

    rc = tapi_cfg_ps_get_status(ta, "xilinxtool", &status);
    if (rc == TE_RC(TE_CS, TE_ENOENT) || status == FALSE)
        return;
    else if(rc != 0)
        TEST_FAIL("Failed to finish UART logger: %r", rc);

    CHECK_RC(tapi_cfg_ps_stop(ta, "xilinxtool"));
}

int
main(int argc, char *argv[])
{
    const char     *arm_ta = "Agt_ARM";
    const char     *disk_ta = "Agt_disk";
    const char     *host_ta = "Agt_host";
    const char     *uart_ta = "Agt_UART";
    rcf_rpc_server *disk_rpcs = NULL;
    rcf_rpc_server *rpcs_arm = NULL;
    rcf_rpc_server *rpcs_host = NULL;
    char           *use_uart_logger = getenv("TE_UART_LOGGER_EXEC");

    tsapi_blkproxy_handle *bp_handle = NULL;

    const struct if_nameindex *clt_if = NULL;
    tapi_env env;

    tsapi_backend_types type;

    TEST_START;
    TSAPI_TEST_START_ENV;

    TEST_GET_RPCS(arm_ta, "pco", rpcs_arm);
    TEST_GET_RPCS(host_ta, "pco", rpcs_host);

    TEST_STEP("Unbind the driver on the all block devices");
    tsapi_virtio_blocks_bind_driver(rpcs_host, "");

    TEST_STEP("Unload virtio-blk module");
    CHECK_RC(tapi_cfg_module_unload(host_ta, "virtio-blk"));

    TEST_STEP("Stop block proxy");
    CHECK_RC(tsapi_blkproxy_handle_init(&bp_handle, rpcs_arm, NULL, NULL));
    CHECK_RC(tsapi_blkproxy_stop(bp_handle));

    TEST_STEP("Kill Onload orphaned stacks");
    CHECK_RC(tsapi_blkproxy_kill_orphaned_stacks(rpcs_arm));

    TEST_STEP("Unbind uio_pci_generic driver");
    CHECK_RC(tapi_cfg_pci_unbind_driver_by_vend_dev_inst(
                 arm_ta, "10ee", "00f0", TSAPI_BLKPROXY_PCI_NUMBER));

    type = tsapi_backend_get_type();
    if (type == TSAPI_BACKEND_TYPES_NONE)
        TEST_FAIL("Unknown backend type");

    if (backend_callbacks[type].stop != NULL)
    {
        TEST_GET_RPCS(disk_ta, "pco", disk_rpcs);

        CHECK_RC(backend_callbacks[type].stop(disk_rpcs));
    }

    if (tsapi_use_plugin())
    {
        TEST_GET_IF(clt_if);

        TEST_STEP("Disable plugin");
        CHECK_RC(tsapi_plugin_disable(rpcs_arm, clt_if->if_name));
    }

    if (use_uart_logger)
    {
        TEST_STEP("Finish UART logger");
        finish_uart_logger(uart_ta);
    }

    TEST_SUCCESS;
cleanup:
    tsapi_blkproxy_handle_free(bp_handle);
    TEST_END;
}

/**@} <!-- END blk-proxy-epilogue --> */
