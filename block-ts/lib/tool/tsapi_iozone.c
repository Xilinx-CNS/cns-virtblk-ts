/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief iozone tool TAPI
 *
 * TAPI to handle iozone tool.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_LGR_USER "TSAPI IOZONE"

#include "tsapi_iozone.h"
#include "ts-tool.h"

/** Timeout of a graceful termination of iozone */
#define TSAPI_IOZONE_TERM_TIMEOUT_MS 1000
/** Timeout for which to wait for a message to appear on a filter */
#define TSAPI_IOZONE_RECEIVE_TIMEOUT_MS 1000

/** IOzone binary name */
static const char *iozone_bin = "iozone";

/** IOzone handle */
struct tsapi_iozone_app {
    /* TAPI job handle */
    tapi_job_t *job;
    /* Output channels handles */
    tapi_job_channel_t *out_chs[2];
    /* Filter that gets results */
    tapi_job_channel_t *results_filter;
};

/** Default IOzone options */
const tsapi_iozone_opt tsapi_iozone_default_opt = {
    .sync_writes      = TRUE,
    .record_size      = NULL,
    .file_size        = NULL,
    .file_under_test  = NULL,
};

/** IOzone option binds */
static const tapi_job_opt_bind tsapi_iozone_binds[] = TAPI_JOB_OPT_SET(
        TAPI_JOB_OPT_BOOL("-o", tsapi_iozone_opt, sync_writes),
        TAPI_JOB_OPT_STRING("-r", FALSE, tsapi_iozone_opt, record_size),
        TAPI_JOB_OPT_STRING("-s", FALSE, tsapi_iozone_opt, file_size),
        TAPI_JOB_OPT_STRING("-f", FALSE, tsapi_iozone_opt, file_under_test)
);

TS_TOOL_DEF_CREATE(iozone, tsapi_iozone_binds, iozone_bin);

/* See description in tsapi_iozone.h */
te_errno
tsapi_iozone_create(tapi_job_factory_t *factory, tsapi_iozone_opt *opt,
                    const char *working_dir, tsapi_iozone_app **app)
{
    te_errno rc;
    const char *regexp = " +\\d+ +\\d+ +\\d+ +\\d+ +\\d+ +\\d+ +\\d+ +\\d+"
                         " +\\d+ +\\d+ +\\d+ +\\d+ +\\d+ +\\d+ +\\d+";

    if (opt->file_under_test == NULL && working_dir != NULL)
    {
        opt->file_under_test = te_string_fmt("%s/file_under_test", working_dir);
        if (opt->file_under_test == NULL)
        {
            ERROR("Failed to allocate memory for file_under_test name");
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }
    }

    rc = tsapi_iozone_create_common(factory, opt, app);
    if (rc != 0)
        goto out;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET((*app)->out_chs[0]), NULL,
                                TRUE, 0, &(*app)->results_filter);
    if (rc != 0)
    {
        ERROR("Failed to attach filter to a job instance, error: %r", rc);
        goto out;
    }

    rc = tapi_job_filter_add_regexp((*app)->results_filter, regexp, 0);
    if (rc != 0)
        ERROR("Failed to add regexp to a filter, error: %r", rc);

out:
    free(opt->file_under_test);
    return rc;
}

/* See description in tsapi_iozone.h */
te_errno
tsapi_iozone_start(tsapi_iozone_app *app)
{
    te_errno rc;

    rc = tapi_job_clear(TAPI_JOB_CHANNEL_SET(app->results_filter));
    if (rc != 0)
    {
        ERROR("Failed to clear job's filter, error: %r", rc);
        return rc;
    }

    return tapi_job_start(app->job);
}

TS_TOOL_DEF_WAIT(iozone);

TS_TOOL_DEF_KILL(iozone);

TS_TOOL_DEF_STOP(iozone, TSAPI_IOZONE_TERM_TIMEOUT_MS);

TS_TOOL_DEF_DESTROY(iozone, TSAPI_IOZONE_TERM_TIMEOUT_MS);

/* See description in tsapi_iozone.h */
te_errno
tsapi_iozone_print_results(tsapi_iozone_app *app)
{
    te_errno           rc;
    tapi_job_buffer_t  buf = TAPI_JOB_BUFFER_INIT;

    rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(app->results_filter),
                          TSAPI_IOZONE_RECEIVE_TIMEOUT_MS, &buf);
    if (rc != 0 || buf.eos)
    {
        if (rc == 0)
            rc = TE_RC(TE_TAPI, TE_EFAIL);

        ERROR("Failed to get iozone results, error: %r", rc);
    }
    else
    {
        RING("IOzone results:\nkB reclen write rewrite read reread random_read "
             "random_write bkwd_read record_rewrite stride_read fwrite frewrite "
             "fread freread\n%s", buf.data.ptr);
    }

    te_string_free(&buf.data);

    return rc;
}
