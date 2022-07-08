/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for work to XFS test suite
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "tsapi-xfs.h"

te_errno
xfs_test_suite_init(xfs_test_suite *xfs_ts, rcf_rpc_server *rpcs,
                    const char *xfs_path, const char *group, const char *name)
{
    te_errno rc;
    te_string test_dev = TE_STRING_INIT_STATIC(256);
    te_string test_dir = TE_STRING_INIT_STATIC(256);
    te_string scratch_dev = TE_STRING_INIT_STATIC(256);
    te_string scratch_dir = TE_STRING_INIT_STATIC(256);
    te_string path = TE_STRING_INIT_STATIC(256);
    te_string command = TE_STRING_INIT;
    tapi_job_simple_desc_t desc;

    xfs_ts->group = strdup(group);
    xfs_ts->name = strdup(name);

    rc = tapi_job_factory_rpc_create(rpcs, &xfs_ts->factory);
    if (rc != 0)
        return rc;

    /* XFS test suite required start from own catalog */
    rc = te_string_append(&command, "cd %s; ./%s %s/%s", xfs_path,
                          "check", xfs_ts->group, xfs_ts->name);
    if (rc != 0)
        return rc;

    rc = te_string_append(&test_dev, "TEST_DEV=%s", xfs_ts->test_dev);
    if (rc != 0)
        return rc;

    rc = te_string_append(&test_dir, "TEST_DIR=%s", xfs_ts->test_dir);
    if (rc != 0)
        return rc;

    rc = te_string_append(&scratch_dev, "SCRATCH_DEV=%s", xfs_ts->scratch_dev);
    if (rc != 0)
        return rc;

    rc = te_string_append(&scratch_dir, "SCRATCH_MNT=%s", xfs_ts->scratch_dir);
    if (rc != 0)
        return rc;

    rc = te_string_append(&path, "PATH=/bin:/usr/bin:/usr/sbin:/sbin");
    if (rc != 0)
        return rc;

    desc = (tapi_job_simple_desc_t){
        .program = "/bin/bash",
        .argv = (const char *[]){ "/bin/bash", "-c", command.ptr, NULL },
        .env = (const char *[]){ test_dev.ptr, test_dir.ptr,
                                 scratch_dev.ptr, scratch_dir.ptr,
                                 path.ptr, "RESULT_BASE=/tmp", NULL },
        .job_loc = &xfs_ts->job,
        .stdout_loc = &xfs_ts->out[0],
        .stderr_loc = &xfs_ts->out[1],
        .filters = TAPI_JOB_SIMPLE_FILTERS(
            {
                .use_stdout = TRUE,
                .filter_name = "stdout-xfs-test-suite",
                .log_level = TE_LL_RING,
            },
            {
                .use_stderr = TRUE,
                .filter_name = "stderr-xfs-test-suite",
                .log_level = TE_LL_ERROR,
            }
        )
    };

    return tapi_job_simple_create(xfs_ts->factory, &desc);
}

te_errno
xfs_test_suite_start(xfs_test_suite *xfs_ts)
{
    return tapi_job_start(xfs_ts->job);
}

te_errno
xfs_test_suite_wait(xfs_test_suite *xfs_ts, unsigned seconds)
{
    te_errno rc;
    tapi_job_status_t status;

    rc = tapi_job_wait(xfs_ts->job, TE_SEC2MS(seconds), &status);

    if (rc == TE_EINPROGRESS)
    {
        RING("Perhaps the XFS test suite is hangs? Kill it");
        rc = tapi_job_kill(xfs_ts->job, SIGKILL);

        return rc != 0 ? rc: TE_EFAIL;
    }

    if (rc != 0)
        return rc;

    if (status.type == TAPI_JOB_STATUS_EXITED && status.value == 0)
    {
        RING("XFS Test sute done");
        return 0;
    }

    ERROR("XFS Test suite failed");
    return TE_EFAIL;
}

te_errno
xfs_test_suite_destroy(xfs_test_suite *xfs_ts)
{
    te_errno rc;

    if (xfs_ts == NULL)
        return 0;

    rc = tapi_job_destroy(xfs_ts->job, -1);
    tapi_job_factory_destroy(xfs_ts->factory);

    free(xfs_ts->group);
    free(xfs_ts->name);
    free(xfs_ts->test_dev);
    free(xfs_ts->test_dir);
    free(xfs_ts->scratch_dev);
    free(xfs_ts->scratch_dir);

    return rc;
}

te_errno
xfs_test_suite_make_xfs(rcf_rpc_server *rpcs, const char *block)
{
    te_errno rc;

    tapi_job_t *job = NULL;
    tapi_job_status_t status;
    tapi_job_factory_t *factory = NULL;
    tapi_job_simple_desc_t desc;
    tapi_job_channel_t *out[2] = {};

    desc = (tapi_job_simple_desc_t){
        .program = "mkfs.xfs",
        .argv = (const char *[]){ "mkfs.xfs", "-f", block, NULL },
        .env = NULL,
        .job_loc = &job,
        .stdout_loc = &out[0],
        .stderr_loc = &out[1],
        .filters = TAPI_JOB_SIMPLE_FILTERS(
            {
                .use_stdout = TRUE,
                .filter_name = "mkfs.xfs",
                .log_level = TE_LL_RING,
            },
            {
                .use_stderr = TRUE,
                .filter_name = "mkfs.xfs",
                .log_level = TE_LL_ERROR,
            }
        )
    };

    rc = tapi_job_factory_rpc_create(rpcs, &factory);
    if (rc != 0)
        return rc;

    rc = tapi_job_simple_create(factory, &desc);
    if (rc != 0)
        return rc;

    rc = tapi_job_start(job);
    if (rc != 0)
        return rc;

    rc = tapi_job_wait(job, TE_SEC2MS(60), &status);
    if (rc != 0)
        return rc;

    if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
    {
        rc = TE_EFAIL;
    }

    tapi_job_destroy(job, -1);
    tapi_job_factory_destroy(factory);

    return rc;
}
