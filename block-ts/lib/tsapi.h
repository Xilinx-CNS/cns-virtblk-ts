/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helpers for test suite
 *
 * @defgroup blk-proxy-tsapi Common helpers for work with TS
 * @ingroup virtblk-blk-proxy-lib
 * @{
 *
 * Main header file containing useful helpers
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TSAPI_H__
#define __TSAPI_H__

#include "te_config.h"
#include "te_errno.h"
#include "te_defs.h"
#include "rcf_rpc.h"

/**
 * Helper macro to use env in tests
 */
#define TSAPI_TEST_START_ENV \
do {                           \
    if (tsapi_backend_need_env())  \
        TEST_START_ENV;        \
} while (0)

/**
 * Helper macro to free env in tests
 */
#define TSAPI_TEST_END_ENV \
do {                           \
    if (tsapi_backend_need_env())  \
        TEST_END_ENV;          \
} while (0)

#undef TEST_END_SPECIFIC
#define TEST_END_SPECIFIC \
do {                                                                        \
    if (te_sigusr2_caught() && getenv("TSAPI_SIGUSR2_COLD_REBOOT") != NULL) \
    {                                                                       \
        char *_ta = getenv("TE_DUT_TST2_TA_NAME");                          \
        te_errno _rc;                                                       \
                                                                            \
        _rc = cfg_reboot_ta(_ta, TRUE, RCF_REBOOT_TYPE_COLD);               \
        if (_rc != 0)                                                       \
        {                                                                   \
            ERROR("Cold reboot failed: %r", _rc);                           \
            tapi_test_run_status_set(TE_TEST_RUN_STATUS_FAIL);              \
        }                                                                   \
    }                                                                       \
} while(0)

#include "tapi_test.h"
#include "tapi_rpc.h"
#include "tapi_sh_env.h"
#include "tapi_test_log.h"
#include "tapi_env.h"
#include "tapi_network.h"
#include "tapi_sockaddr.h"

#define TSAPI_VIRTIO_BLOCK_VENDOR_ID "1bf4"
#define TSAPI_VIRTIO_BLOCK_MODERN_DEVICE_ID "1042"
#define TSAPI_VIRTIO_BLOCK_TRANSITIONAL_DEVICE_ID "1001"
#define TSAPI_VIRTIO_BLOCK_DRIVER_NAME "virtio-pci"

/* Path to ansible scripts in the agent directory */
#define TSAPI_VIRTBLK_CI_ANSIBLE_AGT_PATH "tmp/virtblk-ci-ansible"

/* Name of the directory that contains backend-helpers */
#define TSAPI_HELPERS_AGT_DIR_NAME "backend-helpers"

/** Backend mode. */
typedef enum tsapi_backend_mode {
    /** Backend mode is CEPH */
    TSAPI_BACKEND_MODE_CEPH,
    /** Backend mode is NVME */
    TSAPI_BACKEND_MODE_NVME,
    /** Backend mode is Malloc */
    TSAPI_BACKEND_MODE_MALLOC,
    /** Backend mode is NullBlock */
    TSAPI_BACKEND_MODE_NULLBLOCK,
    /** Backend mode is unknown */
    TSAPI_BACKEND_MODE_UNKNOWN
} tsapi_backend_mode;

/**
 * Get a string parameter from /local/backend/params subtree.
 *
 * @param backend Backend name
 * @param param   Name of the parameter to get
 *
 * @note Returned string should be freed after usage.
 *
 * @return String value of the parameter or @c NULL
 */
extern char *tsapi_get_backend_param_str(const char *backend, const char *param);

/**
 * Get an int parameter from /local/backend/params subtree.
 *
 * @param[in]  backend      Backend name
 * @param[in]  param_name   Name of the parameter to get
 * @param[out] val          Value of requested parameter
 *
 * @return Status code.
 */
extern te_errno tsapi_get_backend_param_int(const char *backend,
                                            const char *param_name,
                                            int *val);

/**
 * Get an bool parameter from /local/backend/params subtree.
 *
 * @param[in]  backend      Backend name
 * @param[in]  param_name   Name of the parameter to get
 * @param[out] val          Value of requested parameter
 *
 * @return Status code.
 */
extern te_errno tsapi_get_backend_param_bool(const char *backend,
                                             const char *param_name,
                                             te_bool *val);

/**
 * Determine whether CEPH is used.
 *
 * @return @c TRUE if onload should be used
 */
extern te_bool tsapi_use_onload(void);

/**
 * Get path to onload.
 *
 * @return String value or @c NULL if the path is not presented.
 */
extern char *tsapi_get_onload_path(void);

/**
 * Determine whether plugin is used.
 *
 * @return @c TRUE if onload should be used
 */
extern te_bool tsapi_use_plugin(void);

/**
 * Get blk_proxy application path
 *
 * @param ta_name   Test agent name
 *
 * @return blk_proxy application path
 */
extern const char * tsapi_blkproxy_bin(const char *ta_name);

/**
 * Bind a driver on all virtio-block PCI devices.
 *
 * @param rpcs      RPC Server
 * @param driver    Driver to bind
 */
extern void tsapi_virtio_blocks_bind_driver(rcf_rpc_server *rpcs,
                                            const char *driver);

/**
 * Get virtio block PCI devices OID list.
 *
 * @param ta        Test agent name
 * @param count     Count of @p pci_devs
 * @param pci_devs  Virtio block PCI device OIDs
 *
 * @return          Status code
 */
extern te_errno tsapi_get_virtio_blocks(const char *ta, unsigned int *count,
                                        char ***pci_devs);
/**
 * Get count of virtio block PCI devices on a test agent;
 *
 * @param ta        Test agent name
 *
 * @return Count of the PCI devices
 */
extern unsigned int tsapi_virtio_blocks_count(const char *ta);

/**
 * Get a name of the mount directory by index.
 * The name consists of a prefix (locates in the congifurator /local tree)
 * and the index. For example, if the prefix is "/tmp/disk" and the index
 * is 0, the function will return "/tmp/disk0".
 *
 * @param ta        Test agent name
 * @param index     Index to be concatenated with the prefix
 *
 * @exception TEST_FAIL  Failed to get the prefix from the configurator
 *                       or failed to allocate memory for mount directory name
 *
 * @return Mount directory name that (cannot be @c NULL) on which
 *         you can call free()
 */
extern char *tsapi_get_mount_dir_name(const char *ta, int index);

/**
 * Create mount directory by index.
 *
 * @param rpcs      RPC Server
 * @param index     Index of the directory.
 *
 * @exception TEST_FAIL  tsapi_get_mount_dir_name() failed
 * @return Status code
 *
 * @sa tsapi_get_mount_dir_name()
 */
extern te_errno tsapi_create_mount_dir(rcf_rpc_server *rpcs, int index);

/**
 * Remove mount directory by index.
 *
 * @param rpcs      RPC Server
 * @param index     Index of the directory.
 *
 * @exception TEST_FAIL  tsapi_get_mount_dir_name() failed
 * @return Status code
 *
 * @sa tsapi_get_mount_dir_name()
 */
extern te_errno tsapi_remove_mount_dir(rcf_rpc_server *rpcs, int index);

/**
 * Get all blocks under test
 *
 * @param ta[in]             Test Agent
 * @param blocks_name[out]   List of block names
 * @param count[out]         Count of names
 *
 * @return Status code
 */
extern te_errno tsapi_get_devices(const char *ta, char ***blocks_name,
                                  unsigned int *count);

/**
 * Append the path to the agent directory
 *
 * @param id      Prefix for error message, usually __func__
 * @param dst     Pointer to destination buffer
 * @param size    Size of buffer
 * @param ta      Agent name
 * @param fmt     Format of the path to append
 * @param ...     Format string arguments
 *
 * @note This function doesn't return error, but prints error messages
 *
 * @return Status code
 */
extern char *tsapi_append_path_to_agt_dir(const char *id, char *dst,
                                          size_t size, const char *ta,
                                          const char *fmt, ...)
             __attribute__((format(printf, 5, 6)));
/**
 * Append the path to the agent directory. It is a wrapper
 * for tsapi_append_path_to_agt_dir()
 *
 * @param _dst      Pointer to destination buffer
 * @param _ta       Agent name
 * @param _fmt      Format string with arguments
 */
#define TSAPI_APPEND_PATH_TO_AGT_DIR(_dst, _ta, _fmt...) \
    tsapi_append_path_to_agt_dir(__func__, _dst, sizeof(_dst), _ta, _fmt)

/**
 * Put a string to a file on TA via RPC server
 *
 * @param rpcs RPC server
 * @param str  String to put
 * @param path Path to file on TA side
 *
 * @return Status code
 */
extern te_errno tsapi_put_str_to_agt_file(rcf_rpc_server *rpcs,
                                          const te_string *str,
                                          const char *path);

/**
 * Put a string to a local file on Engine
 *
 * @param str  String to put
 * @param path Path to file on Engine side
 *
 * @return Status code
 */
extern te_errno tsapi_put_str_to_local_file(const te_string *str,
                                            const char *path);

#endif /* __TSAPI_H__ */

/**@} <!-- END blk-proxy-tsapi --> */
