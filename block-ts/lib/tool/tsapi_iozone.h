/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief iozone tool TAPI
 *
 * TAPI to handle iozone tool.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TSAPI_IOZONE_H__
#define __TSAPI_IOZONE_H__

#include "tapi_job.h"

#ifdef __cplusplus
extern "C" {
#endif

/** iozone specific command line options */
typedef struct tsapi_iozone_opt {
    /**
     * Open the files with the O_SYNC flag, so all writes will be
     * synchronously written to disk. @c TRUE be default.
     */
    te_bool sync_writes;
    /**
     * Size of the chunks that iozone breaks a file into before performing
     * an I/O operation (R/W) on the disk. Should be formatted as a number and
     * a unit ('k' for Kbytes, 'm' for Mbytes, 'g' for Gbytes). For instance,
     * "64k" for 64 Kbytes.
     */
    const char *record_size;
    /**
     * Size of the file under test. The format is the same as for
     * @p record_size.
     */
    const char *file_size;
    /**
     * File to make read/write operations with. Useful to specify
     * a filesystem to test.
     */
    char *file_under_test;
} tsapi_iozone_opt;

/** Default options initializer. */
extern const tsapi_iozone_opt tsapi_iozone_default_opt;

/** IOzone handle. */
typedef struct tsapi_iozone_app tsapi_iozone_app;

/**
 * Create iozone app.
 *
 * @param[in]  factory         Job factory.
 * @param[in]  opt             iozone options.
 * @param[in]  working_dir     Directory (mountpoint) to start iozone in.
 *                             When @p working_dir is specified and
 *                             tsapi_iozone_opt::file_under_test is @c NULL,
 *                             file 'file_under_test' will be created in this
 *                             directory and used as
 *                             tsapi_iozone_opt::file_under_test.
 *                             Therefore, iozone will test a particular
 *                             filesystem to which this directory belongs.
 * @param[out] app             iozone app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_iozone_destroy
 */
extern te_errno tsapi_iozone_create(tapi_job_factory_t *factory,
                                    tsapi_iozone_opt *opt,
                                    const char *working_dir,
                                    tsapi_iozone_app **app);

/**
 * Start iozone.
 *
 * @param      app             iozone app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_iozone_stop
 */
extern te_errno tsapi_iozone_start(tsapi_iozone_app *app);

/**
 * Wait for iozone completion.
 *
 * @param      app             iozone app handle.
 * @param      timeout_ms      Wait timeout in milliseconds.
 *
 * @return     Status code.
 * @retval     TE_EINPROGRESS  iozone is still running.
 * @retval     TE_ESHCMD       iozone was never started or returned
 *                             non-zero exit status.
 */
extern te_errno tsapi_iozone_wait(tsapi_iozone_app *app, int timeout_ms);

/**
 * Send a signal to iozone.
 *
 * @param      app             iozone app handle.
 * @param      signum          Signal to send.
 *
 * @return     Status code.
 */
extern te_errno tsapi_iozone_kill(tsapi_iozone_app *app, int signum);

/**
 * Stop iozone. It can be started over with tsapi_iozone_start().
 *
 * @param      app             iozone app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_iozone_start
 */
extern te_errno tsapi_iozone_stop(tsapi_iozone_app *app);

/**
 * Destroy iozone app. The app cannot be used after calling this function.
 *
 * @param      app             iozone app handle.
 *
 * @return     Status code.
 *
 * @sa tsapi_iozone_create
 */
extern te_errno tsapi_iozone_destroy(tsapi_iozone_app *app);

/**
 * Print iozone results. The function should be called after a successfull call
 * of tsapi_iozone_wait().
 *
 * @param      app             iozone app handle.
 *
 * @return     Status code.
 * @retval     TE_EFAIL        iozone hasn't produced the line with the results.
 */
extern te_errno tsapi_iozone_print_results(tsapi_iozone_app *app);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TSAPI_IOZONE_H__ */
