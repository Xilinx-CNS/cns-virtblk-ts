/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for work with NVME
 *
 * @defgroup blk-proxy-nvme Helper for work backends
*  @ingroup virtblk-blk-proxy-lib
 * @{
 *
 * @author Artemii Morozov <Artemii.Morozov@arknetworks.am>
 */

#include "tsapi-nvme.h"
#include "tsapi-inventory.h"

#define TSAPI_NVME_SERVER_CREATION_TIMEOUT 5 * 60 * 1000

#define TSAPI_NVME_SERVER_REMOVAL_TIMEOUT 1 * 60 * 1000

/** Values of actions to preparate NVME server */
typedef enum {
    TSAPI_NVME_CREATE,         /*< Create NVME server */
    TSAPI_NVME_REMOVE,         /*< Remove NVME server */
} tsapi_nvme_server_act;

/* Script names that are used to preparate NVME server. */
static const char *nvme_prepare_scripts[] = {
    [TSAPI_NVME_CREATE] = "create_ansible_nvme_server.sh",
    [TSAPI_NVME_REMOVE] = "remove_ansible_nvme_server.sh"
};

/* See description in tsapi-nvme.h */
te_errno
tsapi_nvme_prepare_params(const struct sockaddr *addr,
                          unsigned int block_count)
{
    te_errno rc;
    char *host_name;
    int stor_size_gb;
    int port;
    int disk_size_gb;
    const char *nvme = "nvme";

    host_name = tsapi_get_backend_param_str(nvme, "host_name");
    if (host_name == NULL)
        return TE_EFAIL;

    rc = tsapi_get_backend_param_int(nvme, "port", &port);
    if (rc != 0)
        return rc;

    rc = tsapi_get_backend_param_int(nvme, "stor_size_gb", &stor_size_gb);
    if (rc != 0)
        return rc;

    if (stor_size_gb <= 0)
    {
        ERROR("Storage size should be positive value");
        return TE_EINVAL;
    }

    disk_size_gb = stor_size_gb / block_count;

    rc = tsapi_inventory_local_add_str(nvme, "NVME_SERVER_HOST", host_name);
    if (rc != 0)
        return rc;

    rc = tsapi_inventory_local_add_str(nvme, "NVME_ADDR",
                                       te_sockaddr_get_ipstr(addr));
    if (rc != 0)
        return rc;

    rc = tsapi_inventory_local_add_int(nvme, "NVME_PORT", port);
    if (rc != 0)
        return rc;

    rc = tsapi_inventory_local_add_int(nvme, "NVME_DISK_NUM", block_count);
    if (rc != 0)
        return rc;

    rc = tsapi_inventory_local_add_int(nvme, "NVME_DISK_SIZE", disk_size_gb);
    if (rc != 0)
        return rc;

    return 0;
}

/* See description in tsapi-nvme.h */
te_errno
tsapi_nvme_start_server(rcf_rpc_server *rpcs, const char *helpers_path,
                        const char *virtblk_ci_ansible_path,
                        const char *inventory_path)
{
    rpc_wait_status status;
    te_errno rc = 0;

    rpcs->timeout = TSAPI_NVME_SERVER_CREATION_TIMEOUT;
    status = rpc_system_ex(rpcs, "%s/%s %s %s",
                           helpers_path,
                           nvme_prepare_scripts[TSAPI_NVME_CREATE],
                           virtblk_ci_ansible_path, inventory_path);
    if (status.value != 0)
    {
        ERROR("Failed to execute %s: %s", nvme_prepare_scripts[TSAPI_NVME_CREATE],
              wait_status_flag_rpc2str(status.flag));
        rc = TE_EFAIL;
    }

    return rc;
}

/* See description in tsapi-nvme.h */
te_errno
tsapi_nvme_stop_server(rcf_rpc_server *rpcs, const char *helpers_path)
{
    rpc_wait_status status;
    te_errno rc = 0;

    rpcs->timeout = TSAPI_NVME_SERVER_REMOVAL_TIMEOUT;
    status = rpc_system_ex(rpcs, "%s/%s", helpers_path,
                           nvme_prepare_scripts[TSAPI_NVME_REMOVE]);
    if (status.value != 0)
    {
        ERROR("Failed to execute %s: %s",
              nvme_prepare_scripts[TSAPI_NVME_REMOVE],
              wait_status_flag_rpc2str(status.flag));
        rc = TE_EFAIL;
    }

    return rc;
}
