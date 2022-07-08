/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for work with blk-proxy
 *
 * @defgroup blk-proxy-blk-proxy-helper Helpers for work with blk-proxy
*  @ingroup virtblk-blk-proxy-lib
 * @{
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */

#ifndef __TSAPI_BLKPROXY_H__
#define __TSAPI_BLKPROXY_H__

#include "tsapi.h"
#include "te_mi_log.h"
#include "tsapi-conf.h"
#include "te_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Number of PCI address in the Congifurator tree for blk_proxy */
#define TSAPI_BLKPROXY_PCI_NUMBER 0

/* PCI addres to use for blk_proxy */
#define TSAPI_BLKPROXY_PCI_ADDRESS "01:00.1"

/** Default name of the blk-proxy process */
#define TSAPI_BLOCK_PROXY_PS_NAME "blk-proxy"

/** Default blk-proxy config name */
#define TSAPI_BLKPROXY_CONFIG_NAME "bp_map"

typedef struct tsapi_blkproxy_handle {
    /** RPC server handle */
    rcf_rpc_server *rpcs;
    /** Name of blk-proxy process */
    const char *ps_name;
    /** Name of blk-proxy config */
    const char *config_name;
    /**
     * Path to the agent build directory on the
     * engine side where bp config should be store.
     */
    te_string config_local_dir;
    /**
     * Path to the directory on the agent side
     * where bp config should be store.
     */
    te_string config_agent_dir;
    /** Is SPDK older then 21.07 */
    te_bool is_old_spdk;
} tsapi_blkproxy_handle;

/**
 * Initialize blk-proxy process handle.
 *
 * @param handle             blk-proxy handle
 * @param rpcs               RPC server handle
 * @param ps_name            Name of blk-proxy process
 * @param config_name        Name of blk-proxy config
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_handle_init(tsapi_blkproxy_handle **handle,
                                           rcf_rpc_server *rpcs,
                                           const char *ps_name,
                                           const char *config_name);

/**
 * Free blk-proxy process handle
 *
 * @param handle blk-proxy handle
 */
extern void tsapi_blkproxy_handle_free(tsapi_blkproxy_handle *handle);

/**
 * Get list of flags for blk-proxy from /local tree
 *
 * @return String value or @c NULL if the path is not presented.
 */
extern char *tsapi_blkproxy_get_logflags(void);

typedef struct tsapi_blkproxy_opts {
    /** Memory size in MB for DPDK */
    unsigned int mem_size;
    /** Core mask for DPDK */
    const char *cpu_mask;
    /** Path to config file */
    char *config;
    /** Pci add to whitelist */
    const char *pci_whitelist;
    /** Debug log flag */
    char *logflags;
} tsapi_blkproxy_opts;

/** Default options initializer */
extern const tsapi_blkproxy_opts tsapi_blkproxy_opts_default_opt;

/**
 * Initialize blk-proxy process context.
 *
 * @param handle             blk-porxy handle
 * @param opts               blk-proxy options
 * @param virtio_block_count Number of block devices
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_init(const tsapi_blkproxy_handle *handle,
                                    tsapi_blkproxy_opts *opts);

/**
 * Free blk-proxy options
 *
 * @param opts blk-proxy options
 */
extern void tsapi_blkproxy_options_free(tsapi_blkproxy_opts *opts);

/**
 * Kill Onload orphaned stacks created by the blk-proxy
 *
 * @param rpcs  RPC server handle
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_kill_orphaned_stacks(rcf_rpc_server *rpcs);

/**
 * Start blk-proxy process.
 *
 * @param handle             blk-porxy handle
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_start(const tsapi_blkproxy_handle *handle);

/**
 * Stop blk-proxy process.
 *
 * @param handle             blk-porxy handle
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_stop(const tsapi_blkproxy_handle *handle);

/**
 * Get the status of blk-proxy process.
 *
 * @param[in]  handle             blk-porxy handle
 * @param[out] status             Status blk-proxy process
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_get_status(const tsapi_blkproxy_handle *handle,
                                          te_bool *status);

/**
 * Generate and send to TA blk-proxy config
 *
 * @param handle blk-porxy handle
 * @param argc   Count of arguments
 * @param argv   List of arguments
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_create_conf(const tsapi_blkproxy_handle *handle,
                                           int argc, char **argv);
/**
 * Get blk_proxy application config path
 *
 * @param handle       blk-porxy handle
 * @param json_config  get config in JSON format
 *
 * @return blk_proxy application config path
 */
extern const char * tsapi_blkproxy_get_config_path(
                        const tsapi_blkproxy_handle *handle,
                        te_bool json_config);

/**
 * Allocate hugepages for the blk-proxy
 *
 * @param handle  blk-porxy config handle
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_alloc_hugepages(
                    const tsapi_blkproxy_handle *handle);

/**
 * Free hugepages that were allocated for the blk-proxy
 *
 * @param handle  blk-porxy config handle
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_free_hugepages(
                    const tsapi_blkproxy_handle *handle);

/**
 * Add blk-proxy config to MI log
 *
 * @param logger MI logger intance
 * @param conf   Blk-proxy config handle
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_add_conf_to_mi_log(te_mi_logger *logger,
                                                  tsapi_conf_t *conf,
                                                  te_bool multipath);

/**
 * Mount directory for hugepages as hugetlbfs filesystem
 *
 * @param handle  blk-porxy config handle
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_mount_hugepage_dir(
                    const tsapi_blkproxy_handle *handle);

/**
 * Umount directory for hugepages. All the files will be deleted from
 * the directory.
 *
 * @param handle  blk-porxy config handle
 *
 * @return Status code
 */
extern te_errno tsapi_blkproxy_umount_hugepage_dir(
                    const tsapi_blkproxy_handle *handle);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TSAPI_BLKPROXY_H__ */

/** @} <!-- END blk-proxy-blk-proxy-helper --> */
