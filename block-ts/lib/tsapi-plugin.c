/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for work with plugins
 *
 * @defgroup blk-proxy-plugin-helper Helper for work with plugins
 * @{
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */

#include "tsapi-plugin.h"
#include "conf_api.h"
#include "te_string.h"
#include "tapi_cfg_process.h"

/** Default timeout for loading the plugin */
#define TSAPI_PLUGIN_LOAD_TIMEOUT_S 15
/** Plugin tool */
#define TSAPI_PLUGIN_TOOL "xnpradmin"

char *
tsapi_plugin_get_plugin_name(void)
{
    te_errno rc;
    cfg_val_type val_type = CVT_STRING;
    char *plugin_name = NULL;

    rc = cfg_get_instance_fmt(&val_type, &plugin_name,
                              "/local:/plugin:/name:");
    if (rc != 0)
        return NULL;

    return plugin_name;
}

te_errno
tsapi_plugin_load(rcf_rpc_server *rpcs, const char *ifname,
                  const char *plugin_path)
{
    const char *name = "load_plugin";
    te_string buf = TE_STRING_INIT_STATIC(RCF_MAX_VAL);
    te_errno rc;
    int i = 1;

    rc = tapi_cfg_ps_add(rpcs->ta, name, TSAPI_PLUGIN_TOOL, FALSE);
    if (rc != 0)
        return rc;

    rc = te_string_append(&buf, "ioctl=%s", ifname);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_ps_add_arg(rpcs->ta, name, i++, buf.ptr);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_ps_add_arg(rpcs->ta, name, i++, "program");
    if (rc != 0)
        return rc;

    rc = tapi_cfg_ps_add_arg(rpcs->ta, name, i++, plugin_path);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_ps_start(rpcs->ta, name);
    if (rc != 0)
        return rc;

    te_motivated_sleep(TSAPI_PLUGIN_LOAD_TIMEOUT_S,
                       "Sleep to ensure that plugin is loaded");

    return rc;
}

te_errno
tsapi_plugin_enable(rcf_rpc_server *rpcs, const char *ifname)
{
    const char *name = "enable_plugin";
    te_string buf = TE_STRING_INIT_STATIC(RCF_MAX_VAL);
    te_errno rc;
    int i = 1;

    rc = tapi_cfg_ps_add(rpcs->ta, name, TSAPI_PLUGIN_TOOL, FALSE);
    if (rc != 0)
        return rc;

    rc = te_string_append(&buf, "ioctl=%s", ifname);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_ps_add_arg(rpcs->ta, name, i++, buf.ptr);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_ps_add_arg(rpcs->ta, name, i++, "enable");
    if (rc != 0)
        return rc;

    rc = tapi_cfg_ps_add_arg(rpcs->ta, name, i++, "all");
    if (rc != 0)
        return rc;

    rc = tapi_cfg_ps_start(rpcs->ta, name);

    return rc;
}

te_errno
tsapi_plugin_start(rcf_rpc_server *rpcs, const char *ifname)
{
    te_errno rc;
    char plugin_path[PATH_MAX];
    char *plugin_name = NULL;

    plugin_name = tsapi_plugin_get_plugin_name();
    if (plugin_name == NULL)
    {
        ERROR("The plugin name is not specified");
        rc = TE_ENOENT;
        goto out;
    }

    TSAPI_APPEND_PATH_TO_AGT_DIR(plugin_path, rpcs->ta, "%s", plugin_name);

    rc = tsapi_plugin_load(rpcs, ifname, plugin_path);
    if (rc != 0)
    {
        ERROR("Failed to load plugin: %r", rc);
        goto out;
    }

    rc = tsapi_plugin_enable(rpcs, ifname);
    if (rc != 0)
    {
        ERROR("Failed to enable plugin: %r", rc);
        goto out;
    }

out:
    if (rc != 0)
        free(plugin_name);

    return rc;
}

te_errno
tsapi_plugin_disable(rcf_rpc_server *rpcs, const char *ifname)
{
    rpc_wait_status rc;

    rc = rpc_system_ex(rpcs, "xnpradmin ioctl=%s disable all", ifname);
    if (rc.value != 0)
    {
        ERROR("Failed to disable plugin: %s", wait_status_flag_rpc2str(rc.flag));
        return TE_EFAIL;
    }

    return 0;
}
