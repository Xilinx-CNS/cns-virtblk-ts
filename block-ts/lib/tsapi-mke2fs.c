/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper to work with mke2fs test suite
 *
 * Helper to work with mke2fs test suite.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#include "tsapi-mke2fs.h"

#define MKE2FS_TS_FSCK_TERM_TIMEOUT_MS     1000
#define MKE2FS_TS_FILE_WAIT_TIMEOUT_MS     10000
#define MKE2FS_TS_FILE_RECEIVE_TIMEOUT_MS  1000
#define MKE2FS_TS_FILE_TERM_TIMEOUT_MS     1000

/* Check the filesystem using e2fsck tool without fixing any errors */
static te_errno
e2fsck_check(tapi_job_factory_t *factory, const char *device, int timeout_ms)
{
    te_errno             rc;
    tapi_job_t          *job = NULL;
    tapi_job_channel_t  *out_chs[2];
    tapi_job_status_t    status;
    const char          *tool = "e2fsck";
    const char          *args[] = {tool, "-n", device, NULL};

    rc = tapi_job_simple_create(factory,
                            &(tapi_job_simple_desc_t){
                                .program    = tool,
                                .argv       = args,
                                .job_loc    = &job,
                                .stdout_loc = &out_chs[0],
                                .stderr_loc = &out_chs[1],
                                .filters    = TAPI_JOB_SIMPLE_FILTERS(
                                    {
                                        .use_stdout  = TRUE,
                                        .readable    = FALSE,
                                        .log_level   = TE_LL_RING,
                                        .filter_name = "e2fsck stdout",
                                    },
                                    {
                                        .use_stderr  = TRUE,
                                        .readable    = FALSE,
                                        .log_level   = TE_LL_ERROR,
                                        .filter_name = "e2fsck stderr",
                                    }
                                )
                            });
    if (rc != 0)
    {
        ERROR("Failed to create job for e2fsck tool");
        return rc;
    }

    rc = tapi_job_start(job);
    if (rc != 0)
    {
        ERROR("Failed to start e2fsck");
        goto out;
    }

    rc = tapi_job_wait(job, timeout_ms, &status);
    if (rc != 0)
    {
        ERROR("Failed to wait for e2fsck completion");
        goto out;
    }

    if (status.type != TAPI_JOB_STATUS_EXITED)
    {
        ERROR("e2fsck terminated abnormally");
        rc = TE_RC(TE_TAPI, TE_ESHCMD);
    }
    else if (status.value != 0)
    {
        ERROR("e2fsck detected some errors");
        rc = TE_RC(TE_TAPI, TE_EPROTO);
    }
    else
    {
        RING("e2fsck did not detect any errors with the filesystem");
    }

out:
    tapi_job_destroy(job, MKE2FS_TS_FSCK_TERM_TIMEOUT_MS);

    return rc;
}

/* Check that the type of the created filesystem is the same as was requested */
static te_errno
fs_type_check(tapi_job_factory_t *factory, const char *fs_type,
              const char *device)
{
    te_errno             rc;
    tapi_job_t          *job = NULL;
    tapi_job_channel_t  *out_chs[2];
    tapi_job_channel_t  *fs_type_filter;
    tapi_job_status_t    status;
    const char          *tool = "file";
    const char          *args[] = {tool, "-s", device, NULL};

    if (fs_type == NULL)
        return 0;

    rc = tapi_job_simple_create(factory,
                            &(tapi_job_simple_desc_t){
                                .program    = tool,
                                .argv       = args,
                                .job_loc    = &job,
                                .stdout_loc = &out_chs[0],
                                .stderr_loc = &out_chs[1],
                                .filters    = TAPI_JOB_SIMPLE_FILTERS(
                                    {
                                        .use_stdout  = TRUE,
                                        .readable    = TRUE,
                                        .re          = fs_type,
                                        .extract     = 0,
                                        .filter_var  = &fs_type_filter,
                                    },
                                    {
                                        .use_stdout  = TRUE,
                                        .readable    = FALSE,
                                        .log_level   = TE_LL_RING,
                                        .filter_name = "file stdout",
                                    },
                                    {
                                        .use_stderr  = TRUE,
                                        .readable    = FALSE,
                                        .log_level   = TE_LL_ERROR,
                                        .filter_name = "file stderr",
                                    }
                                )
                            });
    if (rc != 0)
    {
        ERROR("Failed to create job for file tool");
        return rc;
    }

    rc = tapi_job_start(job);
    if (rc != 0)
    {
        ERROR("Failed to start file tool");
        goto out;
    }

    rc = tapi_job_wait(job, MKE2FS_TS_FILE_WAIT_TIMEOUT_MS, &status);
    if (rc != 0)
    {
        ERROR("Failed to wait for file tool completion");
        goto out;
    }
    if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
    {
        ERROR("File tool execution failed");
        rc = TE_RC(TE_TAPI, TE_ESHCMD);
        goto out;
    }

    if (tapi_job_filters_have_data(TAPI_JOB_CHANNEL_SET(fs_type_filter),
                                   MKE2FS_TS_FILE_RECEIVE_TIMEOUT_MS))
    {
        RING("The filesystem was created with the requested type");
    }
    else
    {
        ERROR("The created filesystem type is not the one that was requested");
        rc = TE_RC(TE_TAPI, TE_EPROTO);
    }

out:
    tapi_job_destroy(job, MKE2FS_TS_FILE_TERM_TIMEOUT_MS);

    return rc;
}

te_errno
mke2fs_ts_check_fs(tapi_job_factory_t *factory, tapi_mke2fs_app *app,
                   const char *fs_type, const char *device,
                   int e2fsck_timeout_ms)
{
    te_errno rc;

    rc = e2fsck_check(factory, device, e2fsck_timeout_ms);
    if (rc == 0)
        rc = fs_type_check(factory, fs_type, device);
    if (rc == 0)
        rc = tapi_mke2fs_check_journal(app);

    return rc;
}
