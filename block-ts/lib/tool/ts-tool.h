/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helpers for creating TSAPI for tools
 *
 * Helpers for creating TSAPI for tools.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TSAPI_TOOL_H__
#define __TSAPI_TOOL_H__

#include <signal.h>

#include "tapi_job.h"
#include "tapi_job_opt.h"
#include "te_alloc.h"

/**
 * Internal macro to define a function which creates tool's application.
 *
 * @param tool               Name of a tool.
 * @param tsapi_binds        Tool's option binds.
 * @param path_to_bin        Path to tool's binary.
 */
#define TS_TOOL_DEF_CREATE(_tool, _tsapi_binds, _path_to_bin)                \
static te_errno                                                              \
tsapi_ ## _tool ## _create_common(tapi_job_factory_t *factory,               \
                                 const tsapi_ ## _tool ## _opt *opt,         \
                                 tsapi_ ## _tool ## _app **app)              \
{                                                                            \
    te_errno                  rc;                                            \
    tsapi_ ## _tool ## _app  *result;                                        \
    te_vec                    args = TE_VEC_INIT(char *);                    \
                                                                             \
    result = TE_ALLOC(sizeof(*result));                                      \
    if (result == NULL)                                                      \
        return TE_RC(TE_TAPI, TE_ENOMEM);                                    \
                                                                             \
    rc = tapi_job_opt_build_args(_path_to_bin, _tsapi_binds, opt, &args);    \
    if (rc != 0)                                                             \
    {                                                                        \
        ERROR("Failed to build " #_tool " options, error: %r", rc);          \
        goto out;                                                            \
    }                                                                        \
                                                                             \
    rc = tapi_job_simple_create(factory,                                     \
                        &(tapi_job_simple_desc_t){                           \
                            .program     = _path_to_bin,                     \
                            .argv        = (const char **)args.data.ptr,     \
                            .job_loc     = &result->job,                     \
                            .stdout_loc  = &result->out_chs[0],              \
                            .stderr_loc  = &result->out_chs[1],              \
                            .filters     = TAPI_JOB_SIMPLE_FILTERS(          \
                                {                                            \
                                    .use_stderr  = TRUE,                     \
                                    .readable    = FALSE,                    \
                                    .log_level   = TE_LL_ERROR,              \
                                    .filter_name = #_tool " stderr",         \
                                }                                            \
                            )                                                \
                        });                                                  \
    if (rc != 0)                                                             \
    {                                                                        \
        ERROR("Failed to create job instance for " #_tool", error %r", rc);  \
        goto out;                                                            \
    }                                                                        \
                                                                             \
    *app = result;                                                           \
                                                                             \
out:                                                                         \
    te_vec_deep_free(&args);                                                 \
    if (rc != 0)                                                             \
        free(result);                                                        \
                                                                             \
    return rc;                                                               \
}

/**
 * Internal macro to define a function which waits for tool's completion.
 *
 * @param tool               Name of a tool.
 */
#define TS_TOOL_DEF_WAIT(_tool)                                              \
te_errno                                                                     \
tsapi_ ## _tool ## _wait(tsapi_ ## _tool ## _app *app, int timeout_ms)       \
{                                                                            \
    te_errno           rc;                                                   \
    tapi_job_status_t  status;                                               \
                                                                             \
    rc = tapi_job_wait(app->job, timeout_ms, &status);                       \
    if (rc != 0)                                                             \
        return rc;                                                           \
                                                                             \
    TAPI_JOB_CHECK_STATUS(status);                                           \
                                                                             \
    return 0;                                                                \
}

/**
 * Internal macro to define a function which sends a signal to a tool.
 *
 * @param tool               Name of a tool.
 */
#define TS_TOOL_DEF_KILL(_tool)                                              \
te_errno                                                                     \
tsapi_ ## _tool ## _kill(tsapi_ ## _tool ## _app *app, int signum)           \
{                                                                            \
    return tapi_job_kill(app->job, signum);                                  \
}


/**
 * Internal macro to define a function which stops a tool.
 *
 * @param tool               Name of a tool.
 * @param term_timeout_ms    Timeout of graceful termination of a job.
 */
#define TS_TOOL_DEF_STOP(_tool, _term_timeout_ms)                            \
te_errno                                                                     \
tsapi_ ## _tool ## _stop(tsapi_ ## _tool ## _app *app)                       \
{                                                                            \
    return tapi_job_stop(app->job, SIGTERM, _term_timeout_ms);               \
}


/**
 * Internal macro to define a function which destroys tool's application.
 *
 * @param tool               Name of a tool.
 * @param term_timeout_ms    Timeout of graceful termination of a job.
 */
#define TS_TOOL_DEF_DESTROY(_tool, _term_timeout_ms)                         \
te_errno                                                                     \
tsapi_ ## _tool ## _destroy(tsapi_ ## _tool ## _app *app)                    \
{                                                                            \
    te_errno rc;                                                             \
                                                                             \
    if (app == NULL)                                                         \
        return 0;                                                            \
                                                                             \
    rc = tapi_job_destroy(app->job, _term_timeout_ms);                       \
    if (rc != 0)                                                             \
        ERROR("Failed to destroy " #_tool " job, error: %r", rc);            \
                                                                             \
    free(app);                                                               \
                                                                             \
    return rc;                                                               \
}

#endif /* __TSAPI_TOOL_H__ */
