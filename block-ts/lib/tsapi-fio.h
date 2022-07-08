/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for fio manipulation
 *
 * @defgroup blk-proxy-fio-helper Helpers for work with FIO tool
 * @ingroup virtblk-blk-proxy-lib
 * @{
 * Helper for fio manipulation
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TS_TSAPI_FIO_TS_H__
#define __TS_TSAPI_FIO_TS_H__

#include "tapi_test.h"
#include "tapi_rpc.h"
#include "tapi_fio.h"
#include "tapi_job_factory_rpc.h"
#include "te_mi_log.h"

/* Max fio instace for multi tests */
#define TSAPI_FIO_MULTI_MAX_DEV    (8)

/**
 * Fill fio option form package.xml
 *
 * @param opts FIO options
 * @param argc Count of arguments
 * @param argv List of arguments
 * @param ta   Test agent for read agent specific options
 */
extern void tsapi_fio_opts_read(tapi_fio_opts *opts, int argc, char **argv,
                                const char *ta);

/**
 * Create FIO context based on options and TA name.
 *
 * @param options       FIO options
 * @param factory       Job factory
 * @param ta            Agent name to be used for getting
 *                      FIO tool.
 *                      May be NULL to use default FIO tool.
 * @param fio           Output FIO context
 *
 * @return Status code
 */
extern te_errno tsapi_fio_create(const tapi_fio_opts *options,
                                 tapi_job_factory_t *factory,
                                 const char *ta,
                                 tapi_fio **fio);

/**
 * Fill fio option form package.xml with suffix
 *
 * @param opts      FIO options
 * @param argc      Count of arguments
 * @param argv      List of arguments
 * @param suffix    Suffix for arguments
 */
extern void tsapi_fio_opts_read_suf(tapi_fio_opts *opts, int argc, char **argv,
                                    const char *suffix);

/**
 * Waiting for fio completion based on the startup type
 *
 * @param fio           FIO handler
 *
 * @return Status code
 */
extern te_errno tsapi_fio_flex_wait(tapi_fio *fio);

/* Context of one fio instance */
typedef struct tsapi_fio_context {
    tapi_fio *fio;                  /**< FIO intance */
    tapi_fio_opts opts;             /**< FIO options */
    rcf_rpc_server *forked_rpcs;    /**< RPC server that forked */
    tapi_job_factory_t *forked_factory; /**< Job factory on the forked server */
} tsapi_fio_context;

/**
 * Prepare RPC servers before run fio in multi process mode
 *
 * @param rpcs      RPC server needed for fork
 * @param ctxs      FIO context objects
 * @param n         Count of fio context
 *
 * @return Status code
 */
extern te_errno tsapi_fio_multi_init(rcf_rpc_server *rcps,
                                     tsapi_fio_context *ctxs, int n);

/**
 * Cleanup forked RPC servers
 *
 * @param ctxs       FIO context objects
 * @param n          Count of fio context
 *
 * @return Status code
 */
extern te_errno tsapi_fio_multi_fini(tsapi_fio_context *ctxs, int n);

/** List of the FIO argumets tu put to the log */
#define TSAPI_FIO_ARGS_FOR_MI_LOG \
    "iodepth",   \
    "blocksize", \
    "rwmixread", \
    "readwrite", \
    "numjobs",

/**
 * Add FIO arguments to keys section in the MI log
 *
 * @param logger MI logger instance
 * @param fio    FIO instance
 *
 * @return Status code
 */
extern te_errno tsapi_fio_args_to_mi_log(te_mi_logger *logger,
                                         tapi_fio *fio);

/**
 * Add MI log for fio tool
 *
 * @param fio             FIO intance
 * @param report          FIO test tool report
 * @param blk_proxy_conf  Block proxy config name
 * @param ta              Test agent name where config is placed
 * @param multipath       Use multipath or not
 *
 * @return Status code
 */
extern te_errno tsapi_fio_mi_report(tapi_fio *fio,
                                    const tapi_fio_report *report,
                                    const char *blk_proxy_conf,
                                    const char *ta,
                                    te_bool multipath);

#endif /* __TS_TSAPI_FIO_TS_H__ */

/** @} <!-- END blk-proxy-fio-helper --> */
