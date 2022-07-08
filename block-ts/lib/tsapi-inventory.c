/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper functions for build configuration files
 *
 * @author Denis Prazhennikov <Denis.Pryazhennikov@arknetworks.am>
 */

#include "tsapi-inventory.h"
#include "te_expand.h"
#include "te_kvpair.h"

/* Name of the directory that contains templates for inventory files */
#define TSAPI_INVENTORY_LOCAL_DIR "backend_tmpls"

/** Size of buffer to read inventory template line-by-line */
#define TSAPI_INVENTORY_INPUT_STR_BUF_SIZE 256

/* Names of inventory files for each mode of backend */
static const char *inventory_names[] = {
    [TSAPI_BACKEND_MODE_CEPH] = "ceph_inventory",
    [TSAPI_BACKEND_MODE_NVME] = "nvme_inventory",
    [TSAPI_BACKEND_MODE_MALLOC] = NULL,
    [TSAPI_BACKEND_MODE_NULLBLOCK] = NULL,
    [TSAPI_BACKEND_MODE_UNKNOWN] = NULL,
};

/**
 * Build the path to the inventory template for the given backend
 *
 * @param tmpl_path   Resulting path to the inventory template
 * @param mode        Backend mode
 *
 * @return Status code
 */
static te_errno
tsapi_inventory_get_tmpl_path(te_string *tmpl_path, tsapi_backend_mode mode)
{
    te_errno rc;
    const char *ts_dir = getenv("TE_TS_DIR");
    const char *name = inventory_names[mode];

    if (ts_dir == NULL)
    {
        ERROR("Failed to get path to TS directory");
        return TE_EFAIL;
    }

    if (name == NULL)
    {
        ERROR("There is no inventory template for the given backend mode");
        return TE_EINVAL;
    }

    rc = te_string_append(tmpl_path, "%s/%s/%s_tmpl.yml", ts_dir,
                          TSAPI_INVENTORY_LOCAL_DIR, name);

    return rc;
}

/**
 * Get the path to ceph inventory file on the agent
 *
 * @param ta   Agent name
 * @param mode Backend mode
 *
 * @return Path to ceph inventory file
 */
static const char *
tsapi_inventory_get_ta_path(const char *ta, tsapi_backend_mode mode)
{
    static char inventory_path[PATH_MAX];
    const char *name = inventory_names[mode];

    if (name == NULL)
        return NULL;

    TSAPI_APPEND_PATH_TO_AGT_DIR(inventory_path, ta, "%s.yml", name);

    return inventory_path;
}

/**
 * Expand inventory file from the template
 *
 * @param tmpl_path   Path to the inventory template
 * @param kv_params   List of kv-pair parameters
 * @param inventory   Resulting string that contains inventory
 *
 * @return Status code
 */
static te_errno
tsapi_inventory_expand_template(te_string *tmpl_path,
                                te_kvpair_h *kv_params,
                                te_string *inventory)
{
    FILE *source = NULL;
    te_errno rc = 0;

    char source_buf[TSAPI_INVENTORY_INPUT_STR_BUF_SIZE];
    char *target_buf = NULL;
    te_bool newline_not_found = FALSE;

    source = fopen(tmpl_path->ptr, "r");
    if (source == NULL)
    {
        ERROR("Failed to open inventory template to expand");
        rc = te_rc_os2te(errno);
        goto out;
    }

    while (fgets(source_buf, sizeof(source_buf), source) != NULL)
    {
        /* Check that the line except last one of the file wasn't too long */
        if (newline_not_found)
        {
            ERROR("Line in inventory template is too long");
            rc = TE_EFAIL;
            goto out;
        }

        rc = te_expand_kvpairs(source_buf, NULL, kv_params, &target_buf);
        if (rc != 0)
        {
            ERROR("Failed to expand inventory template");
            goto out;
        }

        newline_not_found = (target_buf[strlen(target_buf) - 1] != '\n');

        rc = te_string_append(inventory, target_buf);
        if (rc != 0)
        {
            ERROR("Failed to create inventory in string format: %r", rc);
            goto out;
        }

        free(target_buf);
    }

    if (ferror(source))
    {
        ERROR("Failed to parse inventory template");
        rc = te_rc_os2te(errno);
        goto out;
    }

out:
    if (source != NULL)
        fclose(source);

    return rc;
}

/* See description in tsapi-inventory.h */
te_errno
tsapi_inventory_prepare(rcf_rpc_server *rpcs, te_kvpair_h *kv_params,
                        tsapi_backend_mode mode, const char **inventory_path)
{
    te_errno  rc;
    te_string inventory = TE_STRING_INIT;
    te_string tmpl_path = TE_STRING_INIT_STATIC(PATH_MAX);
    const char *dst_path = NULL;

    rc = tsapi_inventory_get_tmpl_path(&tmpl_path, mode);
    if (rc != 0)
    {
        ERROR("Failed to construct path to inventory template");
        return rc;
    }
    rc = tsapi_inventory_expand_template(&tmpl_path, kv_params, &inventory);
    if (rc != 0)
    {
        ERROR("Failed to create inventory file by template: %r", rc);
        goto out;
    }

    dst_path = tsapi_inventory_get_ta_path(rpcs->ta, mode);
    if (dst_path == NULL)
    {
        ERROR("Failed to get path to inventory file");
        goto out;
    }

    rc = tsapi_put_str_to_agt_file(rpcs, &inventory, dst_path);
    if (rc != 0)
    {
        ERROR("Failed to put inventory file to %s agent: %r",
              rpcs->ta, rc);
        goto out;
    }

    *inventory_path = dst_path;

out:
    te_string_free(&inventory);

    return rc;
}

/* See description in tsapi-inventory.h */
te_errno
tsapi_inventory_local_add_str(const char *backend, const char *name,
                              const char *value)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, value),
                              "/local:/backend:%s/inventory:%s",
                              backend, name);
    if (rc != 0)
        ERROR("Failed to add %s with %s to local tree: %r", name, value, rc);

    return rc;
}

/* See description in tsapi-inventory.h */
te_errno
tsapi_inventory_local_add_int(const char *backend, const char *name,
                              int value)
{
    te_string val = TE_STRING_INIT;
    te_errno rc;

    rc = te_string_append(&val, "%d", value);
    if (rc != 0)
        return rc;

    rc = tsapi_inventory_local_add_str(backend, name, val.ptr);

    te_string_free(&val);
    return rc;
}

/* See description in tsapi-inventory.h */
te_errno
tsapi_inventory_local_to_kv_pair(const char *backend, te_kvpair_h *params)
{
    te_errno rc;
    cfg_handle *instances;
    unsigned int n_instances = 0;
    unsigned int i;
    cfg_oid *oid = NULL;
    char *val = NULL;

    rc = cfg_find_pattern_fmt(&n_instances, &instances,
                              "/local:/backend:%s/inventory:*", backend);
    if (rc != 0)
    {
        ERROR("Failed to find /local:/backend:%s/inventory subtree: %r",
              backend, rc);
        return rc;
    }

    for (i = 0; i < n_instances; i++)
    {
        rc = cfg_get_oid(instances[i], &oid);
        if (rc != 0)
        {
            ERROR("Failed to get inventory OID: %r", rc);
            break;
        }

        rc = cfg_get_instance(instances[i], NULL, &val);
        if (rc != 0)
        {
            ERROR("Failed to get /local/backend/inventory instance: %r", rc);
            break;
        }

        rc = te_kvpair_add(params, CFG_OID_GET_INST_NAME(oid, 3), "%s", val);
        if (rc != 0)
            break;

        cfg_free_oid(oid);
        free(val);
    }

    if (rc != 0)
    {
        cfg_free_oid(oid);
        free(val);
        te_kvpair_fini(params);
    }

    return rc;
}
