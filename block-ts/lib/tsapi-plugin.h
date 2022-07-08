/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for work with plugins
 *
 * @defgroup blk-proxy-plugin-helper Helpers for work with plugins
 * @ingroup virtblk-blk-proxy-lib
 * @{
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */

#ifndef __TSAPI_PLUGIN_H__
#define __TSAPI_PLUGIN_H__

#include "tsapi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the plugin name from /local/plugin: subtree.
 *
 * @note Returned string should be freed after usage.
 *
 * @return String value of the plugin name.
 */
extern char *tsapi_plugin_get_plugin_name(void);

/**
 * Load plugin to @p ifname interface using xnpradmin tool.
 *
 * @param rpcs        RPC server handle.
 * @param ifname      Name of the interface for loading the plugin.
 * @param plugin_path Path to plugin.
 *
 * @return Status code.
 */
extern te_errno tsapi_plugin_load(rcf_rpc_server *rpcs, const char *ifname,
                                  const char *plugin_path);

/**
 * Enable plugin on @p ifname interface using xnpradmin tool.
 *
 * @param rpcs   RPC server handle.
 * @param ifname Name of the interface for enabling the plugin.
 *
 * @return Status code.
 */
extern te_errno tsapi_plugin_enable(rcf_rpc_server *rpcs, const char *ifname);

/**
 * Load and enbale plugin on @p ifname.
 *
 * @note Only 1 plugin is supported. The plugin name must be
 *       specified in /local/plugin/name.
 *
 * @param rpcs   RPC server handle.
 * @param ifname Name of the interface for loading and enabling the plugin.
 *
 * @return Status code.
 */
extern te_errno tsapi_plugin_start(rcf_rpc_server *rpcs, const char *ifname);

/**
 * Disable plugin on @p ifname interface using xnpradmin tool.
 *
 * @param rpcs   RPC server handle.
 * @param ifname Name of the interface for diabling the plugin.
 *
 * @return Status code.
 */
extern te_errno tsapi_plugin_disable(rcf_rpc_server *rpcs, const char *ifname);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TSAPI_PLUGIN_H__ */

/** @} <!-- END blk-proxy-plugin-helper --> */
