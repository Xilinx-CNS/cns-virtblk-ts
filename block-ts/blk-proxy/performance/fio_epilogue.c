/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Prepare blk-proxy to test performance.
 *
 * @defgroup blk-proxy-performance-fio-epilogue Session epilogue for the performance tests
 * @grouporder{4}
 * @ingroup virtblk-blk-proxy-performance
 *
 * Stop the blk-proxy and clean all up
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

#define TE_TEST_NAME "blk-proxy/performance/fio_epilogue"

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

    TEST_START;
    TEST_GET_RPCS(arm_ta, "pco", rpcs_arm);
    TEST_GET_RPCS(host_ta, "pco", rpcs_host);

    TEST_STEP("Initialize blk-proxy process handle");
    CHECK_RC(tsapi_blkproxy_handle_init(&bp_handle, rpcs_arm,
                                        PERFORMANCE_LIB_BLK_PROXY_PS_NAME,
                                        PERFORMANCE_LIB_BLK_PROXY_CONF_NAME));

    TEST_STEP("Unbind the driver on the all block devices");
    tsapi_virtio_blocks_bind_driver(rpcs_host, "");

    TEST_STEP("Stop already running blk-proxy");
    CHECK_RC(tsapi_blkproxy_stop(bp_handle));

    TEST_STEP("Umount directory for hugepage");
    CHECK_RC(tsapi_blkproxy_umount_hugepage_dir(bp_handle));

    TEST_SUCCESS;

cleanup:
    tsapi_blkproxy_handle_free(bp_handle);
    TEST_END;
}
