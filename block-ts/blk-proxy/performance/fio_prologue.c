/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Prepare blk-proxy to test performance.
 *
 * @defgroup blk-proxy-performance-fio-prologue Session prologue for the performance tests
 * @grouporder{2}
 * @ingroup virtblk-blk-proxy-performance
 * @{
 *
 * Prepare the blk-proxy for the testing with session's parameters
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

#define TE_TEST_NAME "blk-proxy/performance/fio_prologue"

#include "tsapi.h"
#include "tsapi-blkproxy.h"
#include "lib/performance_lib.h"

int
main(int argc, char *argv[])
{
    const char     *arm_ta = "Agt_ARM";
    const char     *host_ta = "Agt_host";
    rcf_rpc_server *rpcs_arm = NULL;
    rcf_rpc_server *rpcs_host = NULL;

    tsapi_blkproxy_handle *bp_handle = NULL;
    tsapi_blkproxy_opts blk_prox_opts;

    TEST_START;
    TEST_GET_RPCS(arm_ta, "pco", rpcs_arm);
    TEST_GET_RPCS(host_ta, "pco", rpcs_host);

    TEST_STEP("Initialize blk-proxy process handle");
    CHECK_RC(tsapi_blkproxy_handle_init(&bp_handle, rpcs_arm,
                                        PERFORMANCE_LIB_BLK_PROXY_PS_NAME,
                                        PERFORMANCE_LIB_BLK_PROXY_CONF_NAME));

    TEST_STEP("Prepare blk-proxy options");
    blk_prox_opts = tsapi_blkproxy_opts_default_opt;

    TEST_STEP("Generate blk-proxy config");
    CHECK_RC(tsapi_blkproxy_create_conf(bp_handle, argc, argv));

    TEST_STEP("Start blk-proxy");
    CHECK_RC(tsapi_blkproxy_init(bp_handle, &blk_prox_opts));
    CHECK_RC(tsapi_blkproxy_mount_hugepage_dir(bp_handle));
    CHECK_RC(tsapi_blkproxy_start(bp_handle));

    TEST_STEP("Binding virtio-pci driver");
    tsapi_virtio_blocks_bind_driver(rpcs_host, TSAPI_VIRTIO_BLOCK_DRIVER_NAME);

    TEST_SUCCESS;

cleanup:
    tsapi_blkproxy_handle_free(bp_handle);
    tsapi_blkproxy_options_free(&blk_prox_opts);
    TEST_END;
}
