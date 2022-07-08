/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Performance package prologue.
 *
 * @defgroup blk-proxy-performance-prologue The main prologue for the performance tests
 * @grouporder{1}
 * @ingroup virtblk-blk-proxy-performance
 *
 * General preparation for the performance testing
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

#define TE_TEST_NAME "blk-proxy/performance/prologue"

#include "tsapi.h"
#include "tsapi-blkproxy.h"
#include "tsapi-ceph.h"
#include "lib/performance_lib.h"
#include "tsapi-backend.h"

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

    if (tsapi_backend_get_mode() == TSAPI_BACKEND_MODE_CEPH)
    {
        TEST_STEP("Completely fill block devices");
        CHECK_RC(tsapi_ceph_fill_devices(rpcs_host,
                                         PERFORMANCE_LIB_DEFAULT_FIO_OPT_SIZE));
    }

    TEST_STEP("Get default blk-proxy handle");
    CHECK_RC(tsapi_blkproxy_handle_init(&bp_handle, rpcs_arm,
                                        NULL, NULL));

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
