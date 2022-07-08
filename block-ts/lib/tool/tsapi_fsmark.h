/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief fsmark tool TAPI
 *
 * TAPI to handle fsmark tool.
 *
 * All stdout is logged automatically.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TSAPI_FSMARK_H__
#define __TSAPI_FSMARK_H__

#include "tapi_job.h"

#ifdef __cplusplus
extern "C" {
#endif

/** fsmark specific command line options. */
typedef struct tsapi_fsmark_opt {
    /** Number of files to be used, @c 1000 by default. */
    unsigned int numfiles;
    /** Size of each file in bytes, @c 51200 by default. */
    unsigned int file_size;
    /** Working directory to run the filesystem operations in. */
    char *working_dir;
} tsapi_fsmark_opt;

/** Default options initializer. */
extern const tsapi_fsmark_opt tsapi_fsmark_default_opt;

/** FSMark handle. */
typedef struct tsapi_fsmark_app tsapi_fsmark_app;

/**
 * Create fsmark app.
 *
 * @param[in]  factory         Job factory.
 * @param[in]  opt             fsmark options.
 * @param[out] app             fsmark app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_fsmark_destroy
 */
extern te_errno tsapi_fsmark_create(tapi_job_factory_t *factory,
                                    const tsapi_fsmark_opt *opt,
                                    tsapi_fsmark_app **app);

/**
 * Start fsmark.
 *
 * @param      app             fsmark app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_fsmark_stop
 */
extern te_errno tsapi_fsmark_start(tsapi_fsmark_app *app);

/**
 * Wait for fsmark completion.
 *
 * @param      app             fsmark app handle.
 * @param      timeout_ms      Wait timeout in milliseconds.
 *
 * @return     Status code.
 * @retval     TE_EINPROGRESS  fsmark is still running.
 * @retval     TE_ESHCMD       fsmark was never started or returned
 *                             non-zero exit status.
 */
extern te_errno tsapi_fsmark_wait(tsapi_fsmark_app *app, int timeout_ms);

/**
 * Send a signal to fsmark.
 *
 * @param      app             fsmark app handle.
 * @param      signum          Signal to send.
 *
 * @return     Status code.
 */
extern te_errno tsapi_fsmark_kill(tsapi_fsmark_app *app, int signum);

/**
 * Stop fsmark. It can be started over with tsapi_fsmark_start().
 *
 * @param      app             fsmark app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_fsmark_start
 */
extern te_errno tsapi_fsmark_stop(tsapi_fsmark_app *app);

/**
 * Destroy fsmark app. The app cannot be used after calling this function.
 *
 * @param      app             fsmark app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_fsmark_create
 */
extern te_errno tsapi_fsmark_destroy(tsapi_fsmark_app *app);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TSAPI_FSMARK_H__ */
