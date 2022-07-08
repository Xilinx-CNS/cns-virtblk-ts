/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Block Proxy tests prologue
 *
 * @defgroup blk-proxy-prologue Block proxy tests prologue
 * @grouporder{1}
 * @ingroup virtblk-blk-proxy-bootstrap
 * @{
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

/**
 * @objective Perform setup of test environment for all tests
 */

#define TE_TEST_NAME    "blk-proxy/prologue"

#include "tsapi.h"
#include "tsapi-fio.h"
#include "tapi_cfg_modules.h"
#include "tapi_cfg_pci.h"
#include "tapi_cfg_process.h"
#include "error.h"
#include "block-ts.h"
#include "tsapi-plugin.h"
#include "tsapi-blkproxy.h"
#include "tsapi-backend.h"

/**
 * Start logger to collect uart logs.
 *
 * @param rpcs      RPC server handle
 */
static void
start_uart_logger(rcf_rpc_server *rpcs)
{
    char xilinxtool_path[PATH_MAX];
    char *uart_dut_host = NULL;
    int i = 1;

    CHECK_NOT_NULL(uart_dut_host = getenv("TE_UART_DUT_HOST"));
    TSAPI_APPEND_PATH_TO_AGT_DIR(xilinxtool_path, rpcs->ta,
                                 "snapper/scripts/xilinxtool");

    CHECK_RC(tapi_cfg_ps_add(rpcs->ta, "xilinxtool",
             xilinxtool_path, FALSE));
    CHECK_RC(tapi_cfg_ps_add_arg(rpcs->ta, "xilinxtool", i++, "--silent"));
    CHECK_RC(tapi_cfg_ps_add_arg(rpcs->ta, "xilinxtool", i++, "--duthost"));
    CHECK_RC(tapi_cfg_ps_add_arg(rpcs->ta, "xilinxtool", i++, uart_dut_host));
    CHECK_RC(tapi_cfg_ps_add_arg(rpcs->ta, "xilinxtool", i++,
                                 "--force-uart-nmc"));
    CHECK_RC(tapi_cfg_ps_start(rpcs->ta, "xilinxtool"));
}

static te_errno
tsapi_sh_env_ta_path_prepend(const char *ta, const char *dir)
{
    cfg_val_type val_type = CVT_STRING;
    char new_path_str[PATH_MAX];
    char *path;
    te_errno rc;
    int res;

    rc = cfg_get_instance_fmt(&val_type, &path, "/agent:%s/env:PATH",
                              ta);
    if (rc != 0)
    {
        ERROR("Failed to get PATH env from agent %s", ta);
        return rc;
    }

    res = snprintf(new_path_str, sizeof(new_path_str), "%s:%s", dir, path);
    if (res >= (int)sizeof(new_path_str))
    {
        /* not that this free matters */
        free(path);
        ERROR("We're seen PATH that is longer then %z", sizeof(new_path_str));
        return TE_RC(TE_TAPI, TE_ENOBUFS);
    }

    free(path);

    rc = cfg_set_instance_fmt(CFG_VAL(STRING, new_path_str),
                              "/agent:%s/env:PATH", ta);

    return rc;
}

/**
 * Start CEPH server in QEMU.
 *
 * @param rpcs            RPC server
 * @param ceph_net_ip     IPv4 address of CEPH net
 * @param ceph_mon_addr   IPv4 address of CEPH monitor
 * @param ceph_net_ip_pfx IPV4 address prefix length
 */
static void
ceph_start_cluster_in_qemu(rcf_rpc_server *rpcs,
                           const struct sockaddr *ceph_net,
                           const struct sockaddr *ceph_mon_addr,
                           unsigned int ceph_net_ip_pfx)
{
    TEST_SUBSTEP("Create ceph config");
    CHECK_RC(block_ts_ceph_cluster_conf_create(rpcs, ceph_net, ceph_mon_addr,
                                               ceph_net_ip_pfx));

    TEST_SUBSTEP("Create ceph monmap");
    CHECK_RC(block_ts_ceph_cluster_monmap_create(rpcs, ceph_mon_addr));

    TEST_SUBSTEP("Start ceph-mon");
    CHECK_RC(block_ts_ceph_cluster_mon_start(rpcs));

    TEST_SUBSTEP("Start ceph-osd");
    CHECK_RC(block_ts_ceph_cluster_osd_start(rpcs));

    TEST_SUBSTEP("Start ceph-mgr");
    rpc_system(rpcs, "ceph-mgr --conf /etc/ceph/ceph.conf "
                     "--monmap /tmp/monmap");

    /* TODO: Move this code into tsapi_ceph */
    rpc_system(rpcs, "ceph --conf /etc/ceph/ceph.conf --monmap /tmp/monmap "
                     "osd pool create rampool 100 100");
    rpc_system(rpcs, "rbd --conf /etc/ceph/ceph.conf "
                     "pool init rampool");
    rpc_system(rpcs, "rbd --conf /etc/ceph/ceph.conf create --size 2048 "
                     "rampool/ramdev");
    rpc_system(rpcs, "rbd --conf /etc/ceph/ceph.conf feature disable "
                     "rampool/ramdev object-map fast-diff deep-flatten");
}

void
start_ceph_server_in_qemu(rcf_rpc_server *rpcs_disk, rcf_rpc_server *rpcs_arm,
                          tapi_env_net *net, const struct sockaddr *srv_addr)
{
    te_bool is_enabled_logging = FALSE;

    CHECK_RC(tsapi_get_backend_param_bool("ceph", "enable_logging",
                                          &is_enabled_logging));

    ceph_start_cluster_in_qemu(rpcs_disk, net->ip4addr,
                               srv_addr, net->ip4pfx);
    CHECK_RC(block_ts_ceph_cluster_conf_create(rpcs_arm, net->ip4addr, srv_addr,
                                               net->ip4pfx));
    if (is_enabled_logging)
        CHECK_RC(block_ts_ceph_start_logger(rpcs_disk));
}

int
main(int argc, char *argv[])
{
    const char     *arm_ta = "Agt_ARM";
    const char     *disk_ta = "Agt_disk";
    const char     *uart_ta = "Agt_UART";
    const char     *host_ta = "Agt_host";
    rcf_rpc_server *rpcs_arm = NULL;
    rcf_rpc_server *rpcs_disk = NULL;
    rcf_rpc_server *rpcs_host = NULL;
    rcf_rpc_server *rpcs_uart = NULL;
    tapi_env_net   *net = NULL;
    char           *use_uart_logger = getenv("TE_UART_LOGGER_EXEC");

    const struct sockaddr  *clt_addr;
    const struct sockaddr  *srv_addr;

    const struct if_nameindex *clt_if = NULL;
    tapi_env env;

    tsapi_blkproxy_handle *blk_prox_handle = NULL;
    tsapi_blkproxy_opts blk_prox_opts = tsapi_blkproxy_opts_default_opt;
    unsigned int virtio_block_count;
    tsapi_backend_types type;

    TEST_START;

    /* Workaround for FWRIVERHD-4487 */
    TEST_GET_RPCS(arm_ta, "pco", rpcs_arm);
    if (tsapi_backend_need_env())
    {
        const char *arm_if = getenv("TE_ARM_SFC_IF");

        rpc_system_ex(rpcs_arm, "ethtool -s %s autoneg off speed 100000"
                      " duplex full", arm_if);
        te_motivated_sleep(15, "Waiting for successfully disabling"
                           " autonegotiation");
    }

    /*
     * We need ENV only in ceph case, because only in this case
     * we have some network setup.
     */
    if (tsapi_backend_need_env())
    {
        tapi_env_init(&env);
        tapi_network_setup(FALSE);
    }
    TSAPI_TEST_START_ENV;
    TEST_GET_RPCS(host_ta, "pco", rpcs_host);
    if (tsapi_backend_need_env())
    {
        TEST_GET_RPCS(disk_ta, "pco", rpcs_disk);
        TEST_GET_NET(net);
        TEST_GET_ADDR_NO_PORT(clt_addr);
        TEST_GET_ADDR_NO_PORT(srv_addr);
        TEST_GET_IF(clt_if);
    }

    type = tsapi_backend_get_type();
    if (type == TSAPI_BACKEND_TYPES_NONE)
        TEST_FAIL("Unknown backend type");

    if (use_uart_logger)
    {
        TEST_GET_RPCS(uart_ta, "pco", rpcs_uart);

        TEST_STEP("Start UART logger");
        start_uart_logger(rpcs_uart);
    }

    if (tsapi_use_plugin())
    {
        TEST_STEP("Load and enable plugin");
        CHECK_RC(tsapi_plugin_start(rpcs_arm, clt_if->if_name));
    }

    virtio_block_count = tsapi_virtio_blocks_count(host_ta);

    if (backend_callbacks[type].prepare_params != NULL)
    {
        CHECK_RC(backend_callbacks[type].prepare_params(srv_addr,
                                                        virtio_block_count));
    }

    if (backend_callbacks[type].start != NULL)
        CHECK_RC(backend_callbacks[type].start(rpcs_disk));

    if (backend_callbacks[type].prepare_clients != NULL)
        CHECK_RC(backend_callbacks[type].prepare_clients(rpcs_disk, clt_addr));

    if (backend_callbacks[type].hook != NULL)
        CHECK_RC(backend_callbacks[type].hook(rpcs_disk));

    /* TODO: Add CEPH in qemu management via tsapi-backend lib. */
    if (type == TSAPI_BACKEND_TYPES_CEPH_QEMU)
        start_ceph_server_in_qemu(rpcs_disk, rpcs_arm, net, srv_addr);

    CHECK_RC(tsapi_blkproxy_handle_init(&blk_prox_handle, rpcs_arm, NULL,
                                        NULL));
    CHECK_RC(tsapi_blkproxy_create_conf(blk_prox_handle, argc, argv));

    TEST_STEP("Load uio_pci_generic driver");
    CHECK_RC(tapi_cfg_module_add(rpcs_arm->ta, "uio_pci_generic", 1));

    TEST_STEP("Binding uio_pci_generic driver");
    CHECK_RC(tapi_cfg_pci_bind_driver_by_vend_dev_inst(rpcs_arm->ta, "10ee",
                                                "00f0",
                                                TSAPI_BLKPROXY_PCI_NUMBER,
                                                "uio_pci_generic"));

    TEST_STEP("Start blk-proxy");

    TEST_SUBSTEP("Starting block proxy for %u devices", virtio_block_count);
    CHECK_RC(tsapi_blkproxy_init(blk_prox_handle, &blk_prox_opts));
    CHECK_RC(tsapi_blkproxy_start(blk_prox_handle));

    TEST_STEP("Adding virtio-blk module");
    /* On some systems modprobe is not in PATH */
    CHECK_RC(tsapi_sh_env_ta_path_prepend("Agt_host", "/usr/sbin"));
    CHECK_RC(tapi_cfg_module_add("Agt_host", "virtio-blk", TRUE));

    TEST_STEP("Binding virtio-pci driver");
    tsapi_virtio_blocks_bind_driver(
        rpcs_host, TSAPI_VIRTIO_BLOCK_DRIVER_NAME);

    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/:"));

    TEST_SUCCESS;
cleanup:
    tsapi_blkproxy_handle_free(blk_prox_handle);
    tsapi_blkproxy_options_free(&blk_prox_opts);
    TSAPI_TEST_END_ENV;
    TEST_END;
}

/**@} <!-- END blk-proxy-prologue --> */
