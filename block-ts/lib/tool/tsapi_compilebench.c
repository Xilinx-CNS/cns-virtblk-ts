/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief compilebench tool TAPI
 *
 * TAPI to handle compilebench tool.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_LGR_USER "TSAPI COMPILEBENCH"

#include "tsapi_compilebench.h"
#include "ts-tool.h"
#include "tapi_job_opt.h"
#include "conf_api.h"
#include "te_str.h"

/** Timeout of a graceful termination of compilebench */
#define TSAPI_COMPILEBENCH_TERM_TIMEOUT_MS     1000
/** Timeout for which to wait for message to appear on a filter */
#define TSAPI_COMPILEBENCH_RECEIVE_TIMEOUT_MS  1000

/** Compilebench binary name */
static const char *compilebench_bin = "compilebench";

/** Compilebench handle */
struct tsapi_compilebench_app {
    /* TAPI job handle */
    tapi_job_t *job;
    /* Output channels handles */
    tapi_job_channel_t *out_chs[2];
    /* Filter that gets results */
    tapi_job_channel_t *results_filter;
};

/** Default compilebench options */
const tsapi_compilebench_opt tsapi_compilebench_default_opt = {
    .buf_size      = TAPI_JOB_OPT_OMIT_UINT,
    .initial_dirs  = TAPI_JOB_OPT_OMIT_UINT,
    .runs          = TAPI_JOB_OPT_OMIT_UINT,
    .makej         = FALSE,
    .working_dir   = NULL,
    .sources_dir   = NULL,
};

/** Compilebench option binds */
static const tapi_job_opt_bind tsapi_compilebench_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_UINT_OMITTABLE("-b", FALSE, NULL,
                                tsapi_compilebench_opt, buf_size),
    TAPI_JOB_OPT_UINT_OMITTABLE("-i", FALSE, NULL,
                                tsapi_compilebench_opt, initial_dirs),
    TAPI_JOB_OPT_UINT_OMITTABLE("-r", FALSE, NULL,
                                tsapi_compilebench_opt, runs),
    TAPI_JOB_OPT_BOOL("-m", tsapi_compilebench_opt, makej),
    TAPI_JOB_OPT_STRING("-D", FALSE, tsapi_compilebench_opt, working_dir),
    TAPI_JOB_OPT_STRING("-s", FALSE, tsapi_compilebench_opt, sources_dir)
);

static te_errno
get_compilebench_sources(const char *ta, char *sources_dir,
                         tsapi_compilebench_opt *opt)
{
    te_errno      rc;
    cfg_val_type  val_type = CVT_STRING;
    char         *ta_dir;

    if (opt->sources_dir != NULL)
        return 0;

    rc = cfg_get_instance_fmt(&val_type, &ta_dir, "/agent:%s/dir:", ta);
    if (rc != 0)
    {
        ERROR("Failed to get agent directory, error: %r", rc);
        return rc;
    }

    TE_STRLCPY(sources_dir, ta_dir, PATH_MAX);
    opt->sources_dir = sources_dir;

    free(ta_dir);

    return 0;
}

/*
 * We use this function instead of one generated with TS_TOOL_DEF_CREATE
 * because compilebench produces some non-error messages to stderr. Therefore,
 * we need a custom stderr filter which will filter out those messages.
 */
static te_errno
tsapi_compilebench_create_common(tapi_job_factory_t *factory,
                                 const tsapi_compilebench_opt *opt,
                                 tsapi_compilebench_app **app)
{
    te_errno                 rc;
    tsapi_compilebench_app  *result;
    te_vec                   args = TE_VEC_INIT(char *);

    const char *stderr_regexp = "^(?!using working directory|native unpatched|"
                                "native patched|create dir|patch dir|"
                                "compile dir|read dir|stat dir|clean kernel|"
                                "delete kernel).+";

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    rc = tapi_job_opt_build_args(compilebench_bin, tsapi_compilebench_binds,
                                 opt, &args);
    if (rc != 0)
    {
        ERROR("Failed to build compilebench options, error: %r", rc);
        goto out;
    }

    rc = tapi_job_simple_create(factory,
                        &(tapi_job_simple_desc_t){
                            .program     = compilebench_bin,
                            .argv        = (const char **)args.data.ptr,
                            .job_loc     = &result->job,
                            .stdout_loc  = &result->out_chs[0],
                            .stderr_loc  = &result->out_chs[1],
                            .filters     = TAPI_JOB_SIMPLE_FILTERS(
                                {
                                    .use_stderr  = TRUE,
                                    .readable    = FALSE,
                                    .re          = stderr_regexp,
                                    .extract     = 0,
                                    .log_level   = TE_LL_ERROR,
                                    .filter_name = "compilebench stderr",
                                }
                            )
                        });
    if (rc != 0)
    {
        ERROR("Failed to create job instance for compilebench, error %r", rc);
        goto out;
    }

    *app = result;

out:
    te_vec_deep_free(&args);
    if (rc != 0)
        free(result);

    return rc;
}

/* See description in tsapi_compilebench.h */
te_errno
tsapi_compilebench_create(tapi_job_factory_t *factory,
                          tsapi_compilebench_opt *opt,
                          tsapi_compilebench_app **app)
{
    te_errno     rc;
    const char  *ta = tapi_job_factory_ta(factory);
    char         sources_dir[PATH_MAX];

    if (ta == NULL)
    {
        ERROR("Failed to get agent name");
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    rc = get_compilebench_sources(ta, sources_dir, opt);
    if (rc != 0)
    {
        ERROR("Failed to get path to compilebench source files, error: %r", rc);
        return rc;
    }

    rc = tsapi_compilebench_create_common(factory, opt, app);
    if (rc != 0)
        return rc;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET((*app)->out_chs[0]), NULL,
                                TRUE, 0, &(*app)->results_filter);
    if (rc != 0)
    {
        ERROR("Failed to attach filter to a job instance, error: %r", rc);
        return rc;
    }

    rc = tapi_job_filter_add_regexp((*app)->results_filter,
                                    "run complete:\\n=+(.|\\n)*", 0);
    if (rc != 0)
        ERROR("Failed to add regexp to a filter, error: %r", rc);

    return rc;
}

/* See description in tsapi_compilebench.h */
te_errno
tsapi_compilebench_start(tsapi_compilebench_app *app)
{
    te_errno rc;

    rc = tapi_job_clear(TAPI_JOB_CHANNEL_SET(app->results_filter));

    if (rc != 0)
    {
        ERROR("Failed to clear a job's filter, error: %r", rc);
        return rc;
    }

    return tapi_job_start(app->job);
}

TS_TOOL_DEF_WAIT(compilebench);

TS_TOOL_DEF_KILL(compilebench);

TS_TOOL_DEF_STOP(compilebench, TSAPI_COMPILEBENCH_TERM_TIMEOUT_MS);

TS_TOOL_DEF_DESTROY(compilebench, TSAPI_COMPILEBENCH_TERM_TIMEOUT_MS);

/* See description in tsapi_compilebench.h */
te_errno
tsapi_compilebench_print_results(tsapi_compilebench_app *app)
{
    te_errno           rc;
    tapi_job_buffer_t  buf = TAPI_JOB_BUFFER_INIT;

    rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(app->results_filter),
                          TSAPI_COMPILEBENCH_RECEIVE_TIMEOUT_MS, &buf);
    if (rc != 0 || buf.eos)
    {
        if (rc == 0)
            rc = TE_RC(TE_TAPI, TE_EFAIL);

        ERROR("Failed to get compilebench results, error: %r", rc);
    }
    else
    {
        RING("Compilebench results:\n%s", buf.data.ptr);
    }

    te_string_free(&buf.data);

    return rc;
}
