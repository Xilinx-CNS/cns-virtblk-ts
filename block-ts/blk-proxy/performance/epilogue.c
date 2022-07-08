/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Performance package epilogue.
 *
 * @defgroup blk-proxy-performance-epilogue The main epilogue for the performance tests
 * @grouporder{5}
 * @ingroup virtblk-blk-proxy-performance
 *
 * Roll all changes back after the performance testing
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

#define TE_TEST_NAME    "blk-proxy/performance/epilogue"

#include "tsapi.h"
#include "tsapi-blkproxy.h"

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

    TEST_STEP("Get default blk-proxy handle");
    CHECK_RC(tsapi_blkproxy_handle_init(&bp_handle, rpcs_arm,
                                        NULL, NULL));

    TEST_STEP("Start blk-proxy");
    CHECK_RC(tsapi_blkproxy_mount_hugepage_dir(bp_handle));
    CHECK_RC(tsapi_blkproxy_start(bp_handle));

    TEST_STEP("Binding virtio-pci driver");
    tsapi_virtio_blocks_bind_driver(rpcs_host, TSAPI_VIRTIO_BLOCK_DRIVER_NAME);

    TEST_SUCCESS;
cleanup:
    tsapi_blkproxy_handle_free(bp_handle);
    TEST_END;
}
