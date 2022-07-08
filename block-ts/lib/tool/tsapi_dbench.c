/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief dbench tool TAPI
 *
 * TAPI to handle dbench tool.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_LGR_USER "TSAPI DBENCH"

#include "tsapi_dbench.h"
#include "ts-tool.h"
#include "conf_api.h"
#include "te_str.h"

/** Timeout of a graceful termination of dbench */
#define TSAPI_DBENCH_TERM_TIMEOUT_MS     1000
/** Timeout for which to wait for message to appear on a filter */
#define TSAPI_DBENCH_RECEIVE_TIMEOUT_MS  1000

/** Dbench binary name */
static const char *dbench_bin = "dbench";

/** Dbench handle */
struct tsapi_dbench_app {
    /* TAPI job handle */
    tapi_job_t *job;
    /* Output channels handles */
    tapi_job_channel_t *out_chs[2];
    /* Filter that gets results */
    tapi_job_channel_t *results_filter;
};

/** Default dbench options */
const tsapi_dbench_opt tsapi_dbench_default_opt = {
    .client_txt_location  = NULL,
    .runtime              = TAPI_JOB_OPT_OMIT_UINT,
    .working_dir          = NULL,
    .numclients           = TAPI_JOB_OPT_OMIT_UINT,
};

/** Dbench option binds */
static const tapi_job_opt_bind tsapi_dbench_binds[] = TAPI_JOB_OPT_SET(
        TAPI_JOB_OPT_STRING("-c", FALSE, tsapi_dbench_opt, client_txt_location),
        TAPI_JOB_OPT_UINT_OMITTABLE("-t", FALSE, NULL,
                                    tsapi_dbench_opt, runtime),
        TAPI_JOB_OPT_STRING("-D", FALSE, tsapi_dbench_opt, working_dir),
        TAPI_JOB_OPT_UINT_OMITTABLE(NULL, FALSE, NULL,
                                    tsapi_dbench_opt, numclients)
);

static te_errno
get_client_txt_location(const char *ta, char *client_txt_location,
                        tsapi_dbench_opt *opt)
{
    te_errno      rc;
    cfg_val_type  val_type = CVT_STRING;
    char         *ta_dir;

    if (opt->client_txt_location != NULL)
        return 0;

    rc = cfg_get_instance_sync_fmt(&val_type, &ta_dir, "/agent:%s/dir:", ta);
    if (rc != 0)
    {
        ERROR("Failed to get agent directory, error: %r", rc);
        return rc;
    }

    TE_SNPRINTF(client_txt_location, PATH_MAX, "%s/client.txt", ta_dir);
    opt->client_txt_location = client_txt_location;

    free(ta_dir);

    return 0;
}

TS_TOOL_DEF_CREATE(dbench, tsapi_dbench_binds, dbench_bin);

/* See description in tsapi_dbench.h */
te_errno
tsapi_dbench_create(tapi_job_factory_t *factory, tsapi_dbench_opt *opt,
                    tsapi_dbench_app **app)
{
    te_errno     rc;
    const char  *ta = tapi_job_factory_ta(factory);
    char         client_txt_location[PATH_MAX];

    if (ta == NULL)
    {
        ERROR("Failed to get agent name");
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    rc = get_client_txt_location(ta, client_txt_location, opt);
    if (rc != 0)
    {
        ERROR("Failed to get path to client.txt file, error: %r", rc);
        return rc;
    }

    rc = tsapi_dbench_create_common(factory, opt, app);
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
                                    " Operation +Count.*\\n -+(.|\\n)*", 0);
    if (rc != 0)
        ERROR("Failed to add regexp to a filter, error: %r", rc);

    return rc;
}

/* See description in tsapi_dbench.h */
te_errno
tsapi_dbench_start(tsapi_dbench_app *app)
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

TS_TOOL_DEF_WAIT(dbench);

TS_TOOL_DEF_KILL(dbench);

TS_TOOL_DEF_STOP(dbench, TSAPI_DBENCH_TERM_TIMEOUT_MS);

TS_TOOL_DEF_DESTROY(dbench, TSAPI_DBENCH_TERM_TIMEOUT_MS);

/* See description in tsapi_dbench.h */
te_errno
tsapi_dbench_print_results(tsapi_dbench_app *app)
{
    te_errno           rc;
    tapi_job_buffer_t  buf = TAPI_JOB_BUFFER_INIT;

    rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(app->results_filter),
                          TSAPI_DBENCH_RECEIVE_TIMEOUT_MS, &buf);
    if (rc != 0 || buf.eos)
    {
        if (rc == 0)
            rc = TE_RC(TE_TAPI, TE_EFAIL);

        ERROR("Failed to get dbench results, error: %r", rc);
    }
    else
    {
        RING("Dbench results:\n%s", buf.data.ptr);
    }

    te_string_free(&buf.data);

    return rc;
}
