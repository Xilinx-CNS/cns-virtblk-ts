/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for work with backends
 *
 * @defgroup blk-proxy-backend-helper Helper for work backends
*  @ingroup virtblk-blk-proxy-lib
 * @{
 *
 * @author Artemii Morozov <Artemii.Morozov@arknetworks.am>
 */

#include "tsapi.h"
#include "tsapi-backend.h"
#include "tsapi-inventory.h"

const backend_callbacks_t backend_callbacks[] = {
    [TSAPI_BACKEND_TYPES_CEPH_CONTAINER] = {
        .start = tsapi_backend_start_server,
        .stop = tsapi_backend_stop_server,
        .prepare_params = tsapi_ceph_prepare_params,
        .prepare_clients = tsapi_ceph_prepare_client,
        .hook = block_ts_ceph_enable_logger
    },
    [TSAPI_BACKEND_TYPES_CEPH_CONNECT] = {
        .start = NULL,
        .stop = NULL,
        .prepare_params = NULL,
        .prepare_clients = tsapi_ceph_prepare_client,
        .hook = block_ts_ceph_enable_logger
    },
    [TSAPI_BACKEND_TYPES_CEPH_QEMU] = {
        .start = NULL,
        .stop = NULL,
        .prepare_params = NULL,
        .prepare_clients = NULL,
        .hook = NULL
    },
    [TSAPI_BACKEND_TYPES_NVME] = {
        .start = tsapi_backend_start_server,
        .stop = tsapi_backend_stop_server,
        .prepare_params = tsapi_nvme_prepare_params,
        .prepare_clients = NULL,
        .hook = NULL
    },
    [TSAPI_BACKEND_TYPES_NVME_CONNECT] = {
        .start = NULL,
        .stop = NULL,
        .prepare_params = tsapi_nvme_prepare_params,
        .prepare_clients = NULL,
        .hook = NULL
    },
    [TSAPI_BACKEND_TYPES_MALLOC] = {
        .start = NULL,
        .stop = NULL,
        .prepare_params = NULL,
        .prepare_clients = NULL,
        .hook = NULL
    },
    [TSAPI_BACKEND_TYPES_NULLBLOCK] = {
        .start = NULL,
        .stop = NULL,
        .prepare_params = NULL,
        .prepare_clients = NULL,
        .hook = NULL
    }
};

/* See description in tsapi-backend.h */
char *
tsapi_backend_mode2str(tsapi_backend_mode mode)
{
    switch (mode)
    {
        case TSAPI_BACKEND_MODE_MALLOC:
            return "malloc";

        case TSAPI_BACKEND_MODE_CEPH:
            return "ceph";

        case TSAPI_BACKEND_MODE_NULLBLOCK:
            return "nullblock";

        case TSAPI_BACKEND_MODE_NVME:
            return "nvme";

        default:
            return NULL;
    }
}

/* See description in tsapi-backend.h */
tsapi_backend_mode
tsapi_backend_get_mode(void)
{
    te_errno rc;
    cfg_handle *instances;
    unsigned int n_instances = 0;
    cfg_oid *oid;
    tsapi_backend_mode mode;

    rc = cfg_find_pattern("/local:/backend:*", &n_instances, &instances);
    if (rc != 0)
    {
        ERROR("Backend is not specified");
        return TSAPI_BACKEND_MODE_UNKNOWN;
    }

    if (n_instances > 1)
    {
        ERROR("More than one backend is specified");
        return TSAPI_BACKEND_MODE_UNKNOWN;
    }

    rc = cfg_get_oid(instances[0], &oid);
    if (rc != 0)
    {
        ERROR("Failed to get backend");
        return TSAPI_BACKEND_MODE_UNKNOWN;
    }

    if (strcmp(CFG_OID_GET_INST_NAME(oid, 2), "malloc") == 0)
        mode = TSAPI_BACKEND_MODE_MALLOC;
    else if (strcmp(CFG_OID_GET_INST_NAME(oid, 2), "ceph") == 0)
        mode = TSAPI_BACKEND_MODE_CEPH;
    else if (strcmp(CFG_OID_GET_INST_NAME(oid, 2), "nullblock") == 0)
        mode = TSAPI_BACKEND_MODE_NULLBLOCK;
    else if (strcmp(CFG_OID_GET_INST_NAME(oid, 2), "nvme") == 0)
        mode = TSAPI_BACKEND_MODE_NVME;
    else
        mode = TSAPI_BACKEND_MODE_UNKNOWN;

    cfg_free_oid(oid);

    return mode;
}

/* See description in tsapi-backend.h */
tsapi_backend_types
tsapi_backend_get_type(void)
{
    tsapi_backend_mode mode = tsapi_backend_get_mode();
    cfg_val_type val_type = CVT_STRING;
    char *val = NULL;
    te_errno rc;
    unsigned i;
    struct {
        tsapi_backend_mode mode;
        char *type_str;
        tsapi_backend_types type;
    } maps[] = {
        {TSAPI_BACKEND_MODE_CEPH, "container",
         TSAPI_BACKEND_TYPES_CEPH_CONTAINER},
        {TSAPI_BACKEND_MODE_CEPH, "qemu", TSAPI_BACKEND_TYPES_CEPH_QEMU},
        {TSAPI_BACKEND_MODE_CEPH, "connect", TSAPI_BACKEND_TYPES_CEPH_CONNECT},
        {TSAPI_BACKEND_MODE_MALLOC, "none", TSAPI_BACKEND_TYPES_MALLOC},
        {TSAPI_BACKEND_MODE_NULLBLOCK, "none", TSAPI_BACKEND_TYPES_NULLBLOCK},
        {TSAPI_BACKEND_MODE_NVME, "none", TSAPI_BACKEND_TYPES_NVME},
        {TSAPI_BACKEND_MODE_NVME, "connect", TSAPI_BACKEND_TYPES_NVME_CONNECT}
    };

    rc = cfg_get_instance_fmt(&val_type, &val,
                              "/local:/backend:%s/type:",
                              tsapi_backend_mode2str(mode));
    if (rc != 0)
        return TSAPI_BACKEND_TYPES_NONE;

    for (i = 0; i < sizeof(maps) / sizeof(*maps); i++)
    {
        if (mode == maps[i].mode && strcmp(val, maps[i].type_str) == 0)
        {
            free(val);
            return maps[i].type;
        }
    }

    free(val);
    return TSAPI_BACKEND_TYPES_NONE;
}

/* See description in tsapi-backend.h */
te_bool
tsapi_backend_need_env(void)
{
    tsapi_backend_mode mode;

    mode = tsapi_backend_get_mode();

    if (mode == TSAPI_BACKEND_MODE_CEPH ||
        mode == TSAPI_BACKEND_MODE_NVME)
        return TRUE;

    return FALSE;
}

/* See description in tsapi-backend.h */
te_errno
tsapi_backend_start_server(rcf_rpc_server *rpcs)
{
    te_kvpair_h params;
    const char *inventory_path = NULL;
    char virtblk_ci_ansible_path[PATH_MAX];
    char helpers_path[PATH_MAX];
    tsapi_backend_mode mode;
    te_errno rc;

    mode = tsapi_backend_get_mode();
    if (mode == TSAPI_BACKEND_MODE_UNKNOWN)
    {
        ERROR("Unknown backend mode");
        return TE_EINVAL;
    }

    te_kvpair_init(&params);

    rc = tsapi_inventory_local_to_kv_pair(tsapi_backend_mode2str(mode),
                                          &params);
    if (rc != 0)
    {
        ERROR("Failed to build parameters for inventory file: %r", rc);
        te_kvpair_fini(&params);
        return rc;
    }

    rc = tsapi_inventory_prepare(rpcs, &params, mode, &inventory_path);
    te_kvpair_fini(&params);
    if (rc != 0)
    {
        ERROR("Failed to prepare invetory file: %r", rc);
        return rc;
    }

    TSAPI_APPEND_PATH_TO_AGT_DIR(virtblk_ci_ansible_path, rpcs->ta,
                                 TSAPI_VIRTBLK_CI_ANSIBLE_AGT_PATH);

    TSAPI_APPEND_PATH_TO_AGT_DIR(helpers_path, rpcs->ta,
                                 TSAPI_HELPERS_AGT_DIR_NAME);

    if (mode == TSAPI_BACKEND_MODE_CEPH)
    {
        rc = tsapi_ceph_start_server(rpcs, helpers_path,
                                     virtblk_ci_ansible_path, inventory_path);
    }
    else if (mode == TSAPI_BACKEND_MODE_NVME)
    {
        rc = tsapi_nvme_start_server(rpcs, helpers_path,
                                     virtblk_ci_ansible_path, inventory_path);
    }
    else
    {
        ERROR("There is no start method for %s", tsapi_backend_mode2str(mode));
        rc = TE_EINVAL;
    }

    if (rc != 0)
        ERROR("Failed to start backend server: %r", rc);

    return rc;
}

/* See description in tsapi-backend.h */
te_errno
tsapi_backend_stop_server(rcf_rpc_server *rpcs)
{
    char helpers_path[PATH_MAX];
    tsapi_backend_mode mode;
    te_errno rc;

    mode = tsapi_backend_get_mode();
    if (mode == TSAPI_BACKEND_MODE_UNKNOWN)
    {
        ERROR("Unknown backend mode");
        return TE_EINVAL;
    }

    TSAPI_APPEND_PATH_TO_AGT_DIR(helpers_path, rpcs->ta,
                                 TSAPI_HELPERS_AGT_DIR_NAME);

    if (mode == TSAPI_BACKEND_MODE_CEPH)
    {
        rc = tsapi_ceph_stop_server(rpcs, helpers_path);
    }
    else if (mode == TSAPI_BACKEND_MODE_NVME)
    {
        rc = tsapi_nvme_stop_server(rpcs, helpers_path);
    }
    else
    {
        ERROR("There is no stop method for %s", tsapi_backend_mode2str(mode));
        rc = TE_EINVAL;
    }

    if (rc != 0)
        ERROR("Failed to stop backend server: %r", rc);

    return rc;
}
