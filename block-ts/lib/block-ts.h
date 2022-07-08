/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Block Test Suite
 *
 * @defgroup blk-proxy-ceph-helper Helper for work with Block TS
 * @ingroup virtblk-blk-proxy-lib
 * @{
*
 * Header file containing useful helpers
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TS_BLOCK_TS_H__
#define __TS_BLOCK_TS_H__

#include "te_config.h"
#include "tapi_test.h"
#include "tapi_rpc.h"
#include "tapi_sh_env.h"
#include "tapi_test_log.h"
#include "tapi_gtest.h"
#include "tapi_job_factory_rpc.h"
#include "tsapi-ceph.h"

/**
 * Add needs for gtest run environment
 *
 * @param rpcs      RPC Server
 * @param argc      Count of arguments
 * @param argv      Argument values
 * @return Status code
 */
extern te_errno block_ts_gtest_setenv(rcf_rpc_server *rpcs,
                                      int argc, char *argv[]);

/** Log conversation action */
typedef enum block_ts_log_conv_action {
    BLOCK_TS_LOG_CONV_TEST_SKIP      /**< Do TEST_SKIP(verdict) */
} block_ts_log_conv_action;

/** Main log conversation struct */
typedef struct block_ts_log_conv {
    const char *re;                     /**< Regual expression */
    block_ts_log_conv_action action;    /**< Action when match */
    const char *verdict;                /**< Verdict when match */
} block_ts_log_conv;

/**
 * Add log conversation
 *
 * @param conv      Main log conversation struct
 * @param in        Input channel
 * @return Status code
 */
extern te_errno block_ts_log_conv_add(tapi_job_channel_t *in,
                                      block_ts_log_conv conv);

/**
 * Generate absolute path of gtest binary
 *
 * @param bin       Binary name
 * @return Absolute path of binary
 */
extern const char *block_ts_gtest_absbin(const char *bin);

/**
 * Run log conversion
 *
 * @return Status code
 */
extern te_errno block_ts_log_conv_run(void);

/** RBD Features list */
typedef enum block_ts_rbd_feature {
    BLOCK_TS_RBD_FEATURE_LAYERING        = (1 << 0), /**<  Layering */
    BLOCK_TS_RBD_FEATURE_STRIPINGV2      = (1 << 1), /**<  Stripingv2 */
    BLOCK_TS_RBD_FEATURE_EXCLUSIVE_LOCK  = (1 << 2), /**<  Exclusive lock */
    BLOCK_TS_RBD_FEATURE_OBJECT_MAP      = (1 << 3), /**<  Object_map */
    BLOCK_TS_RBD_FEATURE_FAST_DIFF       = (1 << 4), /**<  Fast_diff */
    BLOCK_TS_RBD_FEATURE_DEEP_FLATTEN    = (1 << 5), /**<  Deep_flatten */
    BLOCK_TS_RBD_FEATURE_JOURNALING      = (1 << 6), /**<  Journaling */
    BLOCK_TS_RBD_FEATURE_DATA_POOL       = (1 << 7), /**<  Data_pool */
    BLOCK_TS_RBD_FEATURE_OPERATIONS      = (1 << 8), /**<  Operations */
    BLOCK_TS_RBD_FEATURE_MIGRATING       = (1 << 9), /**<  Migrating */
} block_ts_rbd_feature;

/** Set of RBD features */
typedef unsigned block_ts_rbd_features;

/**
 * Convert RBD features set to string
 *
 * @param features      RBD features set
 * @param str           Result string
 *
 * @return Status code
 */
extern te_errno block_ts_rbd_features_string(block_ts_rbd_features features,
                                             te_string *str);

/**
 * Convert name string to RBD feature value
 *
 * @param feature       String feature name
 * @param feature_val   Result RBD feature
 *
 * @return Status code
 */
extern te_errno block_ts_rbd_feature_str2feat(const char *feature,
                                         block_ts_rbd_feature *feature_val);

/**
 * Create ceph conf on TA
 *
 * @param rpcs            RPC server
 * @param ceph_net_ip     IPv4 address of CEPH net
 * @param ceph_mon_addr   IPv4 address of CEPH monitor
 * @param ceph_net_ip_pfx IPV4 address prefix length

 *
 * @return Status code
 */
extern te_errno block_ts_ceph_cluster_conf_create(rcf_rpc_server *rpcs,
                                        const struct sockaddr *ceph_net_ip,
                                        const struct sockaddr *ceph_mon_addr,
                                        unsigned int ceph_net_ip_pfx);

/**
 * Create monmap on TA
 *
 * @param rpcs          RPC server
 * @param ceph_mon_addr IP address of CEPH monitor
 *
 * @return Status code
 */
extern te_errno block_ts_ceph_cluster_monmap_create(rcf_rpc_server *rpcs,
                                        const struct sockaddr *ceph_mon_addr);

/**
 * Start monitor process
 *
 * @param rpcs          RPC server
 *
 * @return Status code
 */
extern te_errno block_ts_ceph_cluster_mon_start(rcf_rpc_server *rpcs);

/**
 * Start osd process
 *
 * @param rpcs          RPC server
 *
 * @return Status code
 */
extern te_errno block_ts_ceph_cluster_osd_start(rcf_rpc_server *rpcs);

/**
 * Prepare directory for CEPH logs.
 *
 * @note This function has sence for ceph created on
 *       QEMU only, because in the case of container
 *       all necessary directories will be created
 *       by cephadm tool.
 *       Should be called before running CEPH server.
 *
 * @param rpcs      RPC server handle
 *
 * @return Status code
 */
extern te_errno block_ts_ceph_prepare_log_dir(rcf_rpc_server *rpcs);

/**
 * Start logger to collect ceph logs.
 *
 * @note tail tool is using to read and print ceph logs.
 *
 * @param rpcs      RPC server handle
 *
 * @return Status code
 */
extern te_errno block_ts_ceph_start_logger(rcf_rpc_server *rpcs);

/**
 * Get ceph fsid
 *
 * @param rpcs     RPC server
 * @param fsid     Out parameter for fsid string
 *
 * @return Status code
 */
extern te_errno block_ts_ceph_cluster_get_fsid(rcf_rpc_server *rpcs,
                                               te_string *fsid);

/**
 * Start logger to collect CEPH logs,
 * if /local/backend/enable_logging is @c TRUE.
 *
 * @param rpcs     RPC server
 *
 * @return Status code.
 */
extern te_errno block_ts_ceph_enable_logger(void *data);

#endif

/**@} <!-- END blk-proxy-ceph-helper --> */
