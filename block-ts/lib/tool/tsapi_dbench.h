/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief dbench tool TAPI
 *
 * TAPI to handle dbench tool.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TSAPI_DBENCH_H__
#define __TSAPI_DBENCH_H__

#include "tapi_job.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Default runtime of dbench in seconds */
#define TSAPI_DBENCH_DEFAULT_RUNTIME 600

/** dbench specific command line options */
typedef struct tsapi_dbench_opt {
    /**
     * Location of a @path{client.txt} file required for dbench to run.
     * If the option is set to @c NULL, it will be handled automatically
     * by tsapi_dbench_create().
     */
    const char *client_txt_location;
    /** Number of seconds for which to run dbench, @c 600 by default. */
    unsigned int runtime;
    /** Working directory to run the filesystem operations in. */
    char *working_dir;
    /** Number of clients to run simultaneously. */
    unsigned int numclients;
} tsapi_dbench_opt;

/** Default options initializer. */
extern const tsapi_dbench_opt tsapi_dbench_default_opt;

/** Dbench handle. */
typedef struct tsapi_dbench_app tsapi_dbench_app;

/**
 * Create dbench app.
 *
 * @param[in]  factory         Job factory.
 * @param[in]  opt             dbench options.
 * @param[out] app             dbench app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_dbench_destroy
 */
extern te_errno tsapi_dbench_create(tapi_job_factory_t *factory,
                                    tsapi_dbench_opt *opt,
                                    tsapi_dbench_app **app);

/**
 * Start dbench.
 *
 * @param      app             dbench app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_dbench_stop
 */
extern te_errno tsapi_dbench_start(tsapi_dbench_app *app);

/**
 * Wait for dbench completion.
 *
 * @param      app             dbench app handle.
 * @param      timeout_ms      Wait timeout in milliseconds.
 *
 * @return     Status code.
 * @retval     TE_EINPROGRESS  dbench is still running.
 * @retval     TE_ESHCMD       dbench was never started or returned
 *                             non-zero exit status.
 */
extern te_errno tsapi_dbench_wait(tsapi_dbench_app *app, int timeout_ms);

/**
 * Send a signal to dbench.
 *
 * @param      app             dbench app handle.
 * @param      signum          Signal to send.
 *
 * @return     Status code.
 */
extern te_errno tsapi_dbench_kill(tsapi_dbench_app *app, int signum);

/**
 * Stop dbench. It can be started over with tsapi_dbench_start().
 *
 * @param      app             dbench app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_dbench_start
 */
extern te_errno tsapi_dbench_stop(tsapi_dbench_app *app);

/**
 * Destroy dbench app. The app cannot be used after calling this function.
 *
 * @param      app             dbench app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_dbench_create
 */
extern te_errno tsapi_dbench_destroy(tsapi_dbench_app *app);

/**
 * Print dbench results. This function should be called after
 * a successfull call of tsapi_dbench_wait().
 *
 * @param      app             dbench app handle.
 *
 * @return     Status code.
 */
extern te_errno tsapi_dbench_print_results(tsapi_dbench_app *app);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TSAPI_DBENCH_H__ */
