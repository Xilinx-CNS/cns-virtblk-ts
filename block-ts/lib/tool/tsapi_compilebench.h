/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief compilebench tool TAPI
 *
 * TAPI to handle compilebench tool.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TSAPI_COMPILEBENCH_H__
#define __TSAPI_COMPILEBENCH_H__

#include "tapi_job.h"

#ifdef __cplusplus
extern "C" {
#endif

/** compilebench specific command line options */
typedef struct tsapi_compilebench_opt {
    /** Buffer size in bytes, 262144 by default */
    unsigned int buf_size;
    /** Number of directories initially created, 30 by default */
    unsigned int initial_dirs;
    /** Number of random operations to be performed, 100 by default */
    unsigned int runs;
    /** Simulate a make -j on the initial dirs and exit */
    te_bool makej;
    /** Working directory */
    char *working_dir;
    /** Directory containing source files required for compilebench */
    const char *sources_dir;
} tsapi_compilebench_opt;

/** Default options initializer */
extern const tsapi_compilebench_opt tsapi_compilebench_default_opt;

/** compilebench handle */
typedef struct tsapi_compilebench_app tsapi_compilebench_app;

/**
 * Create compilebench app.
 *
 * @param[in]  factory         Job factory.
 * @param[in]  opt             compilebench options.
 * @param[out] app             compilebench app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_compilebench_destroy
 */
extern te_errno tsapi_compilebench_create(tapi_job_factory_t *factory,
                                          tsapi_compilebench_opt *opt,
                                          tsapi_compilebench_app **app);

/**
 * Start compilebench.
 *
 * @param      app             compilebench app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_compilebench_stop
 */
extern te_errno tsapi_compilebench_start(tsapi_compilebench_app *app);


/**
 * Wait for compilebench completion.
 *
 * @param      app             compilebench app handle.
 * @param      timeout_ms      Wait timeout in milliseconds.
 *
 * @return     Status code.
 * @retval     TE_EINPROGRESS  compilebench is still running.
 * @retval     TE_ESHCMD       compilebench was never started or returned
 *                             non-zero exit status.
 */
extern te_errno tsapi_compilebench_wait(tsapi_compilebench_app *app,
                                        int timeout_ms);

/**
 * Send a signal to compilebench.
 *
 * @param      app             compilebench app handle.
 * @param      signum          Signal to send.
 *
 * @return     Status code.
 */
extern te_errno tsapi_compilebench_kill(tsapi_compilebench_app *app,
                                        int signum);

/**
 * Stop compilebench. It can be started over with tsapi_compilebench_start().
 *
 * @param      app             compilebench app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_compilebench_start
 */
extern te_errno tsapi_compilebench_stop(tsapi_compilebench_app *app);

/**
 * Destroy compilebench app. The app cannot be used after calling this function.
 *
 * @param      app             compilebench app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_compilebench_create
 */
extern te_errno tsapi_compilebench_destroy(tsapi_compilebench_app *app);

/**
 * Print compilebench results. This function should be called after
 * a successfull call of tsapi_compilebench_wait().
 *
 * @param      app             compilebench app handle.
 *
 * @return     Status code.
 */
extern te_errno tsapi_compilebench_print_results(tsapi_compilebench_app *app);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TSAPI_COMPILEBENCH_H__ */
