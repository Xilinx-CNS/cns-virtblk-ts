/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helpers for test suite
 *
 * Helpers for test suite
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "tsapi.h"
#include "te_str.h"
#include "conf_api.h"
#include "tapi_test.h"
#include "tapi_cfg_pci.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_misc.h"
#include "te_vector.h"

#define TSAPI_MOUNT_DIR_MODE \
    (RPC_S_IRWXU | RPC_S_IRGRP | RPC_S_IXGRP | RPC_S_IROTH | RPC_S_IXOTH)

/* See description in tsapi.h */
char *
tsapi_get_backend_param_str(const char *backend, const char *param)
{
    te_errno rc = 0;
    cfg_val_type val_type = CVT_STRING;
    char *val = NULL;

    rc = cfg_get_instance_fmt(&val_type, &val,
                              "/local:/backend:%s/params:%s", backend, param);
    if (rc != 0)
        return NULL;

    return val;
}

/* See description in tsapi.h */
te_errno
tsapi_get_backend_param_int(const char *backend, const char *param_name,
                            int *val)
{
    te_errno rc = 0;
    char *val_str = NULL;

    val_str = tsapi_get_backend_param_str(backend, param_name);
    if (val_str == NULL)
        return TE_EINVAL;

    rc = te_strtoi(val_str, 10, val);
    if (rc != 0)
        ERROR("Failed to convert %s to integer: %r", param_name, rc);

    free(val_str);
    return rc;
}

/* See description in tsapi.h */
te_errno
tsapi_get_backend_param_bool(const char *backend, const char *param_name,
                             te_bool *val)
{
    te_errno rc = 0;
    char *val_str = NULL;

    val_str = tsapi_get_backend_param_str(backend, param_name);
    if (val_str == NULL)
        return TE_EINVAL;

    if ((strcmp(val_str, "true") == 0) || (strcmp(val_str, "yes") == 0))
        *val = TRUE;
    else
        *val = FALSE;

    free(val_str);
    return rc;
}

te_bool
tsapi_use_onload(void)
{
    char *val;
    cfg_val_type val_type = CVT_STRING;
    te_bool res;

    CHECK_RC(cfg_get_instance_fmt(&val_type, &val,
                                  "/local:/use_onload:"));

    if ((strcmp(val, "true") == 0) || (strcmp(val, "yes") == 0))
        res = TRUE;
    else
        res = FALSE;

    free(val);
    return res;
}

char *
tsapi_get_onload_path(void)
{
    char *val;
    cfg_val_type val_type = CVT_STRING;

    CHECK_RC(cfg_get_instance_fmt(&val_type, &val,
                                  "/local:/onload_path:"));

    if (strlen(val) == 0)
        return NULL;
    else
        return val;
}

te_bool
tsapi_use_plugin(void)
{
    char *val;
    cfg_val_type val_type = CVT_STRING;
    te_bool res;

    CHECK_RC(cfg_get_instance_fmt(&val_type, &val,
                                  "/local:/plugin:/use_plugin:"));

    if ((strcmp(val, "true") == 0) || (strcmp(val, "yes") == 0))
        res = TRUE;
    else
        res = FALSE;

    free(val);
    return res;
}

const char *
tsapi_blkproxy_bin(const char *ta_name)
{
    static char blkproxy_bin_path[PATH_MAX];

    TSAPI_APPEND_PATH_TO_AGT_DIR(blkproxy_bin_path, ta_name, "blk_proxy");

    return blkproxy_bin_path;
}

te_errno
tsapi_get_virtio_blocks(const char *ta, unsigned int *count,
                        char ***pci_devs)
{
    te_errno rc;
    const char *device_id = TSAPI_VIRTIO_BLOCK_MODERN_DEVICE_ID;
    const char *pciid_type = getenv("MC_FW_PCIID_TYPE");

    if (pciid_type != NULL && strcmp(pciid_type, "transitional") == 0) {
        device_id = TSAPI_VIRTIO_BLOCK_TRANSITIONAL_DEVICE_ID;
    }

    rc = tapi_cfg_pci_devices_by_vendor_device(ta,
                                               TSAPI_VIRTIO_BLOCK_VENDOR_ID,
                                               device_id, count, pci_devs);
    if (rc != 0)
        ERROR("Failed to get Virtio block PCI devices");

    return rc;
}

unsigned int
tsapi_virtio_blocks_count(const char *ta)
{
    unsigned int count;

    CHECK_RC(tsapi_get_virtio_blocks(ta, &count, NULL));

    return count;
}

void
tsapi_virtio_blocks_bind_driver(rcf_rpc_server *rpcs, const char *driver)
{
    unsigned int n_virtio;
    char **virtio_pci_dev;
    unsigned int i;

    CHECK_RC(tsapi_get_virtio_blocks(rpcs->ta, &n_virtio, &virtio_pci_dev));
    for (i = 0; i < n_virtio; i++)
        CHECK_RC(tapi_cfg_pci_bind_driver(virtio_pci_dev[i], driver));

    /* Disable block devices scheduler after drivers binding */
    if (strcmp(driver, "") != 0)
        rpc_system(rpcs, "echo none > /sys/block/vda/queue/scheduler");
}

char *
tsapi_get_mount_dir_name(const char *ta, int index)
{
    char *mount_dir;
    char *mount_dir_prefix;
    cfg_val_type val_type = CVT_STRING;

    CHECK_RC(cfg_get_instance_fmt(&val_type, &mount_dir_prefix,
                                  "/local:/agent:%s/mount_dir_prefix:", ta));
    mount_dir = te_string_fmt("%s%d", mount_dir_prefix, index);
    free(mount_dir_prefix);
    if (mount_dir == NULL)
        TEST_FAIL("Failed to allocate memory for mount directory name");

    return mount_dir;
}

te_errno
tsapi_create_mount_dir(rcf_rpc_server *rpcs, int index)
{
    int rpc_rc;
    te_errno rc = 0;
    char *mount_dir = tsapi_get_mount_dir_name(rpcs->ta, index);

    RPC_AWAIT_IUT_ERROR(rpcs);
    rpc_rc = rpc_mkdirp(rpcs, mount_dir, TSAPI_MOUNT_DIR_MODE);
    if (rpc_rc != 0)
        rc = RPC_ERRNO(rpcs);

    free(mount_dir);

    return rc;
}

te_errno
tsapi_remove_mount_dir(rcf_rpc_server *rpcs, int index)
{
    int rpc_rc;
    te_errno rc = 0;
    char *mount_dir = tsapi_get_mount_dir_name(rpcs->ta, index);

    RPC_AWAIT_IUT_ERROR(rpcs);
    rpc_rc = rpc_remove_dir_with_files(rpcs, mount_dir);
    if (rpc_rc != 0)
        rc = RPC_ERRNO(rpcs);

    free(mount_dir);

    return rc;
}

/* See description in tsapi.h */
te_errno
tsapi_get_devices(const char *ta, char ***blocks_name, unsigned int *count)
{
    unsigned int n_blocks = 0;
    unsigned int n_virtio;
    char **virtio_pci;
    te_vec blks_name = TE_VEC_INIT(char *);
    unsigned int i;
    te_errno rc;

    rc = tsapi_get_virtio_blocks(ta, &n_virtio, &virtio_pci);
    if (rc != 0)
        goto out;

    for (i = 0; i < n_virtio; i++)
    {
        unsigned int n_devices;
        char **device_names;
        unsigned int j;

        rc = tapi_cfg_pci_get_devices(virtio_pci[i], &n_devices,
                                      &device_names);
        if (rc != 0)
        {
            ERROR("Failed to get devices of a Virtio block device");
            goto out;
        }

        for (j = 0; rc == 0 && j < n_devices; j++)
        {
            rc = te_vec_append_str_fmt(&blks_name, "/dev/%s", device_names[j]);

            n_blocks++;
        }

        for (j = 0; j < n_devices; j++)
            free(device_names[j]);

        free(device_names);

        if (rc != 0)
            goto out;
    }

    if (n_blocks == 0)
    {
        ERROR("Failed to find at least one virtio block device");
        rc = TE_ENOENT;
        goto out;
    }

    *blocks_name = (char **)blks_name.data.ptr;
    *count = n_blocks;

out:
    if (rc != 0)
    {
        te_vec_deep_free(&blks_name);
        *blocks_name = NULL;
    }

    return rc;
}

/* See description in block-ts.h */
char *
tsapi_append_path_to_agt_dir(const char *id, char *dst, size_t size,
                             const char *ta, const char *fmt, ...)
{
    te_errno rc = 0;
    char *agent_dir = NULL;
    cfg_val_type val_type = CVT_STRING;
    va_list ap;
    size_t len;

    rc = cfg_get_instance_fmt(&val_type, &agent_dir, "/agent:%s/dir:", ta);
    if (rc != 0 || agent_dir == NULL)
    {
       ERROR("%s: failed to get agent directory: %r", id, rc);
       goto out;
    }

    te_snprintf_verbose(id, dst, size, "%s/", agent_dir);
    len = strlen(agent_dir) + 1;

    va_start(ap, fmt);
    rc = te_vsnprintf(dst + len, size - len, fmt, ap);
    va_end(ap);

    if (rc == TE_ESMALLBUF)
        ERROR("%s: string \"%s\" is truncated by snprintf()", id, dst);
    else if (rc != 0)
        ERROR("%s: output error is encountered: %r", id, rc);

out:
    free(agent_dir);
    return dst;
}

/* See description in tsapi.h */
te_errno
tsapi_put_str_to_agt_file(rcf_rpc_server *rpcs, const te_string *str,
                          const char *path)
{
    int fd;
    rpc_fcntl_flags fcntl_flags = RPC_O_CREAT | RPC_O_WRONLY | RPC_O_TRUNC;
    rpc_file_mode_flags mode_flags = RPC_S_IRUSR | RPC_S_IWUSR;
    int rv;

    fd = rpc_open(rpcs, path, fcntl_flags, mode_flags);
    if (fd < 0)
    {
        ERROR("Cannot open file '%s': %r",  path, RPC_ERRNO(rpcs));
        return RPC_ERRNO(rpcs);
    }

    rv = rpc_write_and_close(rpcs, fd, str->ptr, str->len);
    if (((size_t)rv) != str->len)
    {
        ERROR("Failed write to file '%s': %r", path, RPC_ERRNO(rpcs));
        return RPC_ERRNO(rpcs);
    }

    return 0;
}

/* See description in tsapi.h */
te_errno
tsapi_put_str_to_local_file(const te_string *str, const char *path)
{
    FILE *f;
    te_errno rc;

    f = fopen(path, "w+");
    if (f == NULL)
    {
        rc = te_rc_os2te(errno);
        ERROR("Failed to open '%s': %r", path, rc);
        return rc;
    }

    if (fwrite(str->ptr, 1, str->len, f) != str->len)
    {
        rc = te_rc_os2te(errno);
        fclose(f);
        ERROR("Failed to write the data to %s': %r", path, rc);
        return rc;
    }

    fclose(f);

    return 0;
}
