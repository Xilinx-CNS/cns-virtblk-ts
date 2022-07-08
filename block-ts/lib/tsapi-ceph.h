/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief  Block Test Suite
 *
 * Main header file containing useful helpers
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TSAPI_CEPH_H__
#define __TSAPI_CEPH_H__

#include "te_config.h"
#include "tapi_test.h"
#include "tapi_rpc.h"
#include "tapi_sh_env.h"
#include "tapi_test_log.h"
#include "tapi_gtest.h"
#include "tapi_job_factory_rpc.h"
#include "te_vector.h"


/* Name of the process which collect ceph logs */
#define TSAPI_CEPH_LOGGER_NAME "ceph_logger"

/** Details of add option for monmaptool tool */
typedef struct tsapi_ceph_mmt_add_opt {
    const char *name;           /**< Name of host */
    const char *ip;             /**< IP address */
    int port;                   /**< Port of ceph-mon */
} tsapi_ceph_mmt_add_opt;


/** Values of actions to preparate Ceph in the container */
typedef enum {
    TSAPI_CEPH_CREATE,         /*< Create CEPH server */
    TSAPI_CEPH_REMOVE,         /*< Remove CEPH server */
    TSAPI_CEPH_PREPARE_CLIENT, /*< Prepare CEPH client */
} tsapi_ceph_container_act;

/** For support the consistency of the API */
typedef const char * tsapi_ceph_mmt_rm_opt;

/** Options for monmaptool */
typedef struct tsapi_ceph_mmt_opts {
    const char *config;                 /**< Path to ceph config */
    const char *monmap;                 /**< Path to monmap file */
    const char *fsid;                   /**< FSID if ceph instance */
    te_bool need_create;                /**< Add --create option or not */
    te_bool need_print;                 /**< Add --print option or not */
    tsapi_ceph_mmt_add_opt *add;        /**< Add --add option with context */
    tsapi_ceph_mmt_rm_opt *rm;          /**< Add --rm option with context */
} tsapi_ceph_mmt_opts;

/** Options for monmaptool defaults */
#define TSAPI_CEPH_MMT_OPTS_INIT (tsapi_ceph_mmt_opts) {        \
    .config = NULL,                                             \
    .monmap = NULL,                                             \
    .fsid = NULL,                                               \
    .need_create = FALSE,                                       \
    .need_print = FALSE,                                        \
    .add = NULL,                                                \
    .rm = NULL,                                                 \
}

/** Common option accross all ceph-* commands */
typedef struct tsapi_ceph_common_opts {
    const char *config;                /**< Path to ceph config */
    const char *monmap;                /**< Path to monmap file */
    const char *id;                    /**< ID of ceph cluser */
    te_bool mkfs;                      /**< Run in mkfs mode */
} tsapi_ceph_common_opts;

/** Options for common option defaults */
#define TSAPI_CEPH_COMMON_OPTS_INIT (tsapi_ceph_common_opts) {  \
    .config = NULL,                                             \
    .monmap = NULL,                                             \
    .id = NULL,                                                 \
    .mkfs = FALSE,                                              \
}

/** Option for osd subcommand of ceph tool */
typedef struct tsapi_ceph_opts_osd_subcommand {
    te_bool create;                     /** Add 'create' option */
} tsapi_ceph_opts_osd_subcommand;

/** Option for fsid subcommand of ceph tool */
typedef struct tsapi_ceph_opts_fsid_subcommand {
    te_bool get_fsid;                     /** Whether fsid shoud be got */
} tsapi_ceph_opts_fsid_subcommand;

/** Ceph specific options */
typedef struct tsapi_ceph_opts {
    tsapi_ceph_common_opts common_opts;               /**< Common ceph
                                                           options */
    tsapi_ceph_opts_osd_subcommand *osd_subcommand;   /**< osd subcommand
                                                           options */
    tsapi_ceph_opts_fsid_subcommand *fsid_subcommand; /**< fsid subcommand
                                                           options */
    te_bool status;                                   /**< Add --status
                                                           options */
} tsapi_ceph_opts;

/** Options for ceph defaults */
#define TSAPI_CEPH_OPTS_INIT (tsapi_ceph_opts) {                \
    .common_opts = TSAPI_CEPH_COMMON_OPTS_INIT,                 \
    .osd_subcommand = NULL,                                     \
    .fsid_subcommand = NULL,                                    \
}

/** ceph-osd specific options */
typedef struct tsapi_ceph_osd_opts {
    tsapi_ceph_common_opts common_opts; /**< Common ceph options */
} tsapi_ceph_osd_opts;

/** Options for ceph-osd defaults */
#define TSAPI_CEPH_OSD_OPTS_INIT (tsapi_ceph_osd_opts) {        \
    .common_opts = TSAPI_CEPH_COMMON_OPTS_INIT,                 \
}

/** ceph-mon specific options */
typedef struct tsapi_ceph_mon_opts {
    tsapi_ceph_common_opts common_opts; /**< Common ceph options */
} tsapi_ceph_mon_opts;

/** Options for ceph-mon defaults */
#define TSAPI_CEPH_MON_OPTS_INIT (tsapi_ceph_mon_opts) {        \
    .common_opts = TSAPI_CEPH_COMMON_OPTS_INIT,                 \
}

/** Shared part for ceph-* commands */
typedef struct tsapi_ceph_common_t {
    tapi_job_t *job;                    /**< Job handler */
    tapi_job_factory_t *factory;        /**< Job factory */
    tapi_job_channel_t *out[2];         /**< stdin/out handler */
    tapi_job_channel_t *fsid_filter;    /**< fsid filter */
} tsapi_ceph_common_t;

/** Shared part for ceph-* defaults */
#define TSAPI_CEPH_COMMON_T_INIT (tsapi_ceph_common_t) {        \
    .job = NULL,                                                \
    .factory = NULL,                                            \
    .out = {NULL},                                              \
}

/** Handler for manmontool tool */
typedef struct tsapi_ceph_mmt_t {
    tsapi_ceph_common_t common;         /**< Common ceph options */
    tsapi_ceph_mmt_opts opts;           /**< monmaptool options */
} tsapi_ceph_mmt_t;

/** Handler for monmaptool defaults */
#define TSAPI_CEPH_MMT_T_INIT (tsapi_ceph_mmt_t) {              \
    .common = TSAPI_CEPH_COMMON_T_INIT,                         \
    .opts = TSAPI_CEPH_MMT_OPTS_INIT,                           \
}

/** Handler for ceph tool */
typedef struct tsapi_ceph_t {
    tsapi_ceph_common_t common;         /**< Common ceph options */
    tsapi_ceph_opts opts;               /**< ceph options */
} tsapi_ceph_t;

/** Handler for monmaptool defaults */
#define TSAPI_CEPH_T_INIT (tsapi_ceph_t) {                      \
    .common = TSAPI_CEPH_COMMON_T_INIT,                         \
    .opts = TSAPI_CEPH_OPTS_INIT,                               \
}

/** Handler for ceph osd */
typedef struct tsapi_ceph_osd_t {
    tsapi_ceph_common_t common;         /**< Common ceph options */
    tsapi_ceph_osd_opts opts;           /**< ceph-osd options */
} tsapi_ceph_osd_t;

/** Handler for ceph osd defaults */
#define TSAPI_CEPH_OSD_T_INIT (tsapi_ceph_osd_t) {              \
    .common = TSAPI_CEPH_COMMON_T_INIT,                         \
    .opts = TSAPI_CEPH_OSD_OPTS_INIT,                           \
}

/** Handler for ceph mon */
typedef struct tsapi_ceph_mon_t {
    tsapi_ceph_common_t common;         /**< Common ceph options */
    tsapi_ceph_mon_opts opts;           /**< ceph-mon options */
} tsapi_ceph_mon_t;

/** Handler for ceph mon defaults */
#define TSAPI_CEPH_MON_T_INIT (tsapi_ceph_mon_t) {              \
    .common = TSAPI_CEPH_COMMON_T_INIT,                         \
    .opts = TSAPI_CEPH_MON_OPTS_INIT,                           \
}

/**
 * ceph-osd tool initialize
 *
 * @param rpcs              RPC server
 * @param osd               ceph-osd handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_osd_init(rcf_rpc_server *rpcs,
                                    tsapi_ceph_osd_t *osd);

/**
 * ceph-mon tool initialize
 *
 * @param rpcs              RPC server
 * @param osd               ceph-mon handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_mon_init(rcf_rpc_server *rpcs,
                                    tsapi_ceph_mon_t *mon);

/**
 * ceph tool initialize
 *
 * @param rpcs              RPC server
 * @param ceph              ceph handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_init(rcf_rpc_server *rpcs, tsapi_ceph_t *ceph);

/**
 * monmaptool tool initialize
 *
 * @param rpcs              RPC server
 * @param ceph              monmaptool handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_mmt_init(rcf_rpc_server *rpcs,
                                    tsapi_ceph_mmt_t *monmaptool);

/**
 * ceph-osd tool start
 *
 * @param rpcs              RPC server
 * @param osd               ceph-osd handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_osd_start(tsapi_ceph_osd_t *osd);

/**
 * ceph-mon tool start
 *
 * @param rpcs              RPC server
 * @param osd               ceph-mon handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_mon_start(tsapi_ceph_mon_t *mon);

/**
 * ceph tool start
 *
 * @param rpcs              RPC server
 * @param osd               ceph handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_start(tsapi_ceph_t *ceph);

/**
 * monmaptool tool start
 *
 * @param rpcs              RPC server
 * @param osd               monmaptool handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_mmt_start(tsapi_ceph_mmt_t *monmaptool);

/**
 * ceph-osd tool wait
 *
 * @param rpcs              RPC server
 * @param osd               ceph-osd handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_osd_wait(tsapi_ceph_osd_t *osd);

/**
 * ceph-mon tool wait
 *
 * @param rpcs              RPC server
 * @param mon               ceph-mon handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_mon_wait(tsapi_ceph_mon_t *mon);

/**
 * ceph tool wait
 *
 * @param rpcs              RPC server
 * @param ceph              ceph handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_wait(tsapi_ceph_t *ceph);

/**
 * monmaptool tool wait
 *
 * @param rpcs              RPC server
 * @param ceph              monmaptool handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_mmt_wait(tsapi_ceph_mmt_t *monmaptool);

/**
 * ceph-osd tool cleanup
 *
 * @param rpcs              RPC server
 * @param ceph              ceph-osd handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_osd_cleanup(tsapi_ceph_osd_t *osd);

/**
 * ceph-mon tool cleanup
 *
 * @param rpcs              RPC server
 * @param ceph              ceph-mon handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_mon_cleanup(tsapi_ceph_mon_t *mon);

/**
 * ceph tool cleanup
 *
 * @param rpcs              RPC server
 * @param ceph              ceph handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_cleanup(tsapi_ceph_t *ceph);

/**
 * monmaptool tool cleanup
 *
 * @param rpcs              RPC server
 * @param ceph              monmaptool handler
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_mmt_cleanup(tsapi_ceph_mmt_t *monmaptool);

/**
 * Get resulting fsid. This function should be called after
 * a successfull call of tsapi_ceph_wait().
 *
 * @param ceph         ceph app handle
 * @param fsid         Out parameter for fsid string
 *
 * @return     Status code.
 */
extern te_errno tsapi_ceph_get_fsid_result(tsapi_ceph_t *ceph,
                                           te_string *fsid);

/**
 * Fill all block devices by given size
 *
 * @param rpcs        RPC server
 * @param size        The total size of file I/O to fill devices (see man fio)
 *
 * @return Status code
 */
extern te_errno tsapi_ceph_fill_devices(rcf_rpc_server *rpcs,
                                        const char *size);

/**
 * Get human readable name of the action
 *
 * @param action Action to perform with CEPH server in a container
 *
 * @return String with human readable name of the action
 */
static inline const char *
tsapi_ceph_container_act2str(const tsapi_ceph_container_act action)
{
    switch (action)
    {
        case TSAPI_CEPH_CREATE:
            return "create";

        case TSAPI_CEPH_REMOVE:
            return "remove";

        case TSAPI_CEPH_PREPARE_CLIENT:
            return "prepare client for";

        default:
            return "<unknown>";
    }
}

/**
 * Prepare parameters for inventory file.
 *
 * @param mon_addr    IP address of the CEPH monitor
 * @param block_count Number of ceph devices to create
 *
 * @return Status code.
 */
extern te_errno tsapi_ceph_prepare_params(const struct sockaddr *mon_addr,
                                          unsigned int block_count);

/**
 * Start CEPH server.
 *
 * @param rpcs                    RPC server
 * @param helper_path             Path to backend-helpers directory
 * @param virtblk_ci_ansible_path Path to virtblk-ci/ansible directory
 * @param inventory_path          Path to inventory file
 *
 * @return Status code.
 */
extern te_errno tsapi_ceph_start_server(rcf_rpc_server *rpcs,
                                        const char *helper_path,
                                        const char *virtblk_ci_ansible_path,
                                        const char *inventory_path);

/**
 * Stop CEPH server.
 *
 * @param rpcs        RPC server
 * @param helper_path Path to backend-helpers directory
 *
 * @return Status code.
 */
extern te_errno tsapi_ceph_stop_server(rcf_rpc_server *rpcs,
                                       const char *helpers_path);

/**
 * Prepare CEPH client.
 *
 * @param rpcs RPC server
 * @param data IP address of the CEPH client
 *
 * @return Status code.
 */
extern te_errno tsapi_ceph_prepare_client(rcf_rpc_server *rpcs,
                                          const void *data);

#endif /* __TSAPI_CEPH_H__ */
