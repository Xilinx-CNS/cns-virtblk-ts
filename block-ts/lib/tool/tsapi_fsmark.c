/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief fsmark tool TAPI
 *
 * TAPI to handle fsmark tool.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_LGR_USER "TSAPI FSMARK"

#include "tsapi_fsmark.h"
#include "ts-tool.h"
#include "conf_api.h"
#include "te_str.h"

/** Timeout of a graceful termination of fsmark */
#define TSAPI_FSMARK_TERM_TIMEOUT_MS 1000

/** FSMark binary name */
static const char *fsmark_bin = "fs_mark";

/** FSMark handle */
struct tsapi_fsmark_app {
    /* TAPI job handle */
    tapi_job_t *job;
    /* Output channels handles */
    tapi_job_channel_t *out_chs[2];
};

/** Default FSMark options */
const tsapi_fsmark_opt tsapi_fsmark_default_opt = {
    .numfiles     = TAPI_JOB_OPT_OMIT_UINT,
    .file_size    = TAPI_JOB_OPT_OMIT_UINT,
    .working_dir  = NULL,
};

/** FSMark option binds */
static const tapi_job_opt_bind tsapi_fsmark_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_UINT_OMITTABLE("-n", FALSE, NULL, tsapi_fsmark_opt, numfiles),
    TAPI_JOB_OPT_UINT_OMITTABLE("-s", FALSE, NULL, tsapi_fsmark_opt, file_size),
    TAPI_JOB_OPT_STRING("-d", FALSE, tsapi_fsmark_opt, working_dir)
);

TS_TOOL_DEF_CREATE(fsmark, tsapi_fsmark_binds, fsmark_bin);

/* See description in tsapi_fsmark.h */
te_errno
tsapi_fsmark_create(tapi_job_factory_t *factory, const tsapi_fsmark_opt *opt,
                    tsapi_fsmark_app **app)
{
    te_errno rc;

    rc = tsapi_fsmark_create_common(factory, opt, app);
    if (rc != 0)
        return rc;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET((*app)->out_chs[0]),
                                "fsmark stdout", TRUE, TE_LL_RING, NULL);
    if (rc != 0)
        ERROR("Failed to attach filter to a job instance, error: %r", rc);

    return rc;
}

/* See description in tsapi_fsmark.h */
te_errno
tsapi_fsmark_start(tsapi_fsmark_app *app)
{
    return tapi_job_start(app->job);
}

TS_TOOL_DEF_WAIT(fsmark);

TS_TOOL_DEF_KILL(fsmark);

TS_TOOL_DEF_STOP(fsmark, TSAPI_FSMARK_TERM_TIMEOUT_MS);

TS_TOOL_DEF_DESTROY(fsmark, TSAPI_FSMARK_TERM_TIMEOUT_MS);
