/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Block Test Suite
 *
 * Main header file containing useful helpers
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "tsapi.h"
#include "tsapi-ceph.h"
#include "tsapi-inventory.h"
#include "te_sockaddr.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg.h"
#include "tsapi-fio.h"
#include "te_alloc.h"

/** Time to wait for message to appear on the FSID filter */
#define TSAPI_CEPH_RECEIVE_FSID_TIMEOUT_MS 1000

/* RPC call timeout to remove container with CEPH server, in ms */
#define TSAPI_CEPH_SERVER_REMOVAL_TIMEOUT 3 * 60 * 1000

/* RPC call timeout to create CEPH server using cephadm tool, in ms */
#define TSAPI_CEPH_SERVER_CREATION_TIMEOUT 20 * 60 * 1000

/* Default name of a CEPH block device */
#define TSAPI_CEPH_BLK_DEV_NAME "ramdev"

/* Maximal number of CEPH block devices */
#define TSAPI_CEPH_BLK_NUM_MAX 3

/*
 * Size of the required CEPH service information.
 * No one really knows this constant but 2GB should be enough.
 */
#define TSAPI_CEPH_AUX_INFO_SZ_GB 2

#define ADD_OPTION(_cond, _option_fmt...)                                      \
    do {                                                                       \
        te_errno rc;                                                           \
                                                                               \
        if (!(_cond))                                                          \
        {                                                                      \
            break;                                                             \
        }                                                                      \
                                                                               \
        rc = te_vec_append_str_fmt(vec_opts, _option_fmt);                     \
                                                                               \
        if (rc != 0)                                                           \
        {                                                                      \
            te_vec_deep_free(vec_opts);                                        \
            return rc;                                                         \
        }                                                                      \
    } while(0)

#define ADD_OPTION_KV(_cond, _key, _value_fmt...)                              \
    do {                                                                       \
        ADD_OPTION(_cond, "%s", _key);                                         \
        ADD_OPTION(_cond, _value_fmt);                                         \
    } while(0)

#define CEPH_RC(_expr)                                                         \
    do {                                                                       \
        te_errno rc = _expr;                                                   \
                                                                               \
        if ((rc = _expr) != 0)                                                 \
        {                                                                      \
            return rc;                                                         \
        }                                                                      \
    } while(0)

static te_errno
ceph_common_build_command(te_vec *vec_opts, tsapi_ceph_common_opts *opts)
{
    ADD_OPTION_KV(opts->id != NULL, "--id", "%s", opts->id);
    ADD_OPTION_KV(opts->config != NULL, "--conf", "%s", opts->config);
    ADD_OPTION_KV(opts->monmap != NULL, "--monmap", "%s", opts->monmap);
    ADD_OPTION(opts->mkfs, "--mkfs");
    return 0;
}

static te_errno
ceph_build_command(te_vec *vec_opts, tsapi_ceph_opts *opts)
{
    te_errno rc = ceph_common_build_command(vec_opts, &opts->common_opts);

    if (rc != 0)
    {
        return rc;
    }

    if (opts->osd_subcommand != NULL)
    {
        ADD_OPTION(TRUE, "osd");
        ADD_OPTION(opts->osd_subcommand->create, "create");
    }

    if (opts->fsid_subcommand != NULL)
        ADD_OPTION(opts->fsid_subcommand->get_fsid, "fsid");

    ADD_OPTION(opts->status, "-s");

    return rc;
}

static te_errno
ceph_osd_build_command(te_vec *vec_opts, tsapi_ceph_osd_opts *opts)
{
    return ceph_common_build_command(vec_opts, &opts->common_opts);
}

static te_errno
ceph_mon_build_command(te_vec *vec_opts, tsapi_ceph_mon_opts *opts)
{
    return ceph_common_build_command(vec_opts, &opts->common_opts);
}

static te_errno
ceph_mmt_add_opt_host_str(te_string *str, tsapi_ceph_mmt_add_opt *opt)
{
    te_errno rc;

    if (opt->ip != NULL)
    {
        rc = te_string_append(str, "%s", opt->ip);
    }

    if (opt->port != 0)
    {
        rc = te_string_append(str, ":%d", opt->port);
    }

    return rc;
}

static te_errno
ceph_mmt_build_command(te_vec *vec_opts, tsapi_ceph_mmt_opts *opts)
{
    ADD_OPTION(opts->need_create == TRUE, "--create");
    ADD_OPTION(opts->need_print == TRUE, "--print");

    if (opts->add != NULL)
    {
        te_string add_opt = TE_STRING_INIT_STATIC(128);

        ceph_mmt_add_opt_host_str(&add_opt, opts->add);

        ADD_OPTION_KV(TRUE, "--add", "%s", opts->add->name);
        ADD_OPTION(TRUE, "%s", add_opt.ptr);
    }

    ADD_OPTION_KV(opts->fsid != NULL, "--fsid", "%s", opts->fsid);
    ADD_OPTION_KV(opts->config != NULL, "--conf", "%s", opts->config);
    ADD_OPTION(opts->monmap != NULL, "%s", opts->monmap);

    return 0;
}

#define CEPH_GENERIC_DESC_INIT(_bin, _common, _opts, _func)                    \
    do {                                                                       \
        te_errno rc;                                                           \
        te_vec vec_opts = TE_VEC_INIT(char *);                                 \
        tapi_job_simple_desc_t desc;                                           \
        const char *end_marker = NULL;                                         \
                                                                               \
        desc = (tapi_job_simple_desc_t) {                                      \
            .program = _bin,                                                   \
            .job_loc = &_common.job,                                           \
            .stdout_loc = &_common.out[0],                                     \
            .stderr_loc = &_common.out[1],                                     \
            .filters = TAPI_JOB_SIMPLE_FILTERS(                                \
                {                                                              \
                    .use_stdout = TRUE,                                        \
                    .filter_name = "stdout-" _bin,                             \
                    .log_level = TE_LL_RING,                                   \
                },                                                             \
                {                                                              \
                    .use_stderr = TRUE,                                        \
                    .filter_name = "stderr-" _bin,                             \
                    .log_level = TE_LL_ERROR,                                  \
                }                                                              \
            )                                                                  \
        };                                                                     \
                                                                               \
        rc = te_vec_append_str_fmt(&vec_opts, "%s", _bin);                     \
        if (rc != 0)                                                           \
        {                                                                      \
            return rc;                                                         \
        }                                                                      \
                                                                               \
        rc = _func(&vec_opts, &_opts);                                         \
        if (rc != 0)                                                           \
        {                                                                      \
            te_vec_deep_free(&vec_opts);                                       \
            return rc;                                                         \
        }                                                                      \
                                                                               \
        rc = TE_VEC_APPEND(&vec_opts, end_marker);                             \
        if (rc != 0)                                                           \
        {                                                                      \
            te_vec_deep_free(&vec_opts);                                       \
            return rc;                                                         \
        }                                                                      \
                                                                               \
        desc.argv = (const char **)vec_opts.data.ptr;                          \
                                                                               \
        rc = tapi_job_factory_rpc_create(rpcs, &_common.factory);              \
        if (rc != 0)                                                           \
        {                                                                      \
            te_vec_deep_free(&vec_opts);                                       \
            return rc;                                                         \
        }                                                                      \
                                                                               \
        rc = tapi_job_simple_create(_common.factory, &desc);                   \
        if (rc != 0)                                                           \
        {                                                                      \
            tapi_job_factory_destroy(_common.factory);                         \
            te_vec_deep_free(&vec_opts);                                       \
            return rc;                                                         \
        }                                                                      \
    } while(0)

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_osd_init(rcf_rpc_server *rpcs, tsapi_ceph_osd_t *osd)
{
    CEPH_GENERIC_DESC_INIT("ceph-osd", osd->common, osd->opts,
                           ceph_osd_build_command);
    return 0;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_mon_init(rcf_rpc_server *rpcs, tsapi_ceph_mon_t *mon)
{
    CEPH_GENERIC_DESC_INIT("ceph-mon", mon->common, mon->opts,
                           ceph_mon_build_command);
    return 0;
}

/**
 * Attach a filter to get ceph fsid.
 *
 * @param      ceph          ceph app handle.
 *
 * @return     Status code.
 */
static te_errno
tsapi_ceph_attach_fsid_filter(tsapi_ceph_t *ceph)
{
    te_errno rc;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(ceph->common.out[0]),
                                "FSID filter", TRUE, 0,
                                &ceph->common.fsid_filter);
    if (rc != 0)
    {
        ERROR("Failed to attach FSID filter to a job instance, error: %r", rc);
        return rc;
    }

    rc = tapi_job_filter_add_regexp(ceph->common.fsid_filter,
                                    "^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-"
                                    "[a-f0-9]{4}-[a-f0-9]{12}$", 0);
    if (rc != 0)
    {
        ERROR("Failed to add regex to FSID filter, error: %r", rc);
        return rc;
    }

    return 0;
}

te_errno
tsapi_ceph_get_fsid_result(tsapi_ceph_t *ceph, te_string *fsid)
{
    te_errno           rc;
    tapi_job_buffer_t  buf = TAPI_JOB_BUFFER_INIT;

    rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(ceph->common.fsid_filter),
                          TSAPI_CEPH_RECEIVE_FSID_TIMEOUT_MS, &buf);

    if (rc != 0)
    {
        ERROR("Failed to get fsid result, error: %r", rc);
        return rc;
    }

    if (buf.eos)
    {
        ERROR("End of stream before fsid message");
        te_string_free(&buf.data);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    rc = te_string_append(fsid, "%s", buf.data.ptr);
    if (rc != 0)
    {
        ERROR("Filed to construct string with fsid, error: %r", rc);
        te_string_free(fsid);
    }

    te_string_free(&buf.data);

    return rc;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_init(rcf_rpc_server *rpcs, tsapi_ceph_t *ceph)
{
    te_errno rc;

    CEPH_GENERIC_DESC_INIT("ceph", ceph->common, ceph->opts,
                           ceph_build_command);

    if (ceph->opts.fsid_subcommand != NULL &&
        ceph->opts.fsid_subcommand->get_fsid)
    {
        rc = tsapi_ceph_attach_fsid_filter(ceph);
        if (rc != 0)
            return rc;
    }

    return 0;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_mmt_init(rcf_rpc_server *rpcs, tsapi_ceph_mmt_t *monmaptool)
{
    CEPH_GENERIC_DESC_INIT("monmaptool", monmaptool->common, monmaptool->opts,
                           ceph_mmt_build_command);
    return 0;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_osd_start(tsapi_ceph_osd_t *osd)
{
    return tapi_job_start(osd->common.job);
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_mon_start(tsapi_ceph_mon_t *mon)
{
    return tapi_job_start(mon->common.job);
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_start(tsapi_ceph_t *ceph)
{
    return tapi_job_start(ceph->common.job);
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_mmt_start(tsapi_ceph_mmt_t *monmaptool)
{
    return tapi_job_start(monmaptool->common.job);
}

static te_errno
ceph_common_status_handle(const char *name, tapi_job_status_t status)
{
    switch (status.type)
    {
    case TAPI_JOB_STATUS_EXITED:
        if (status.value != 0)
        {
            ERROR("Process %s return %d", name, status.value);
            return TE_EFAIL;
        }
        return 0;
    case TAPI_JOB_STATUS_SIGNALED:
        if (status.value != 0)
        {
            ERROR("Process %s got signal %d", name, status.value);
            return TE_EFAIL;
        }
        return 0;
    case TAPI_JOB_STATUS_UNKNOWN:
        ERROR("Unknown status for %s process", name);
        return TE_EFAIL;
    }

    ERROR("Unsupported status type %d", status.type);
    return TE_EFAIL;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_osd_wait(tsapi_ceph_osd_t *osd)
{
    tapi_job_status_t status;

    CEPH_RC(tapi_job_wait(osd->common.job, TE_SEC2MS(60), &status));

    return ceph_common_status_handle("ceph-osd", status);
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_mon_wait(tsapi_ceph_mon_t *mon)
{
    tapi_job_status_t status;

    CEPH_RC(tapi_job_wait(mon->common.job, TE_SEC2MS(60), &status));

    return ceph_common_status_handle("ceph-mon", status);
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_wait(tsapi_ceph_t *ceph)
{
    tapi_job_status_t status;

    CEPH_RC(tapi_job_wait(ceph->common.job, TE_SEC2MS(60), &status));

    return ceph_common_status_handle("ceph", status);
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_mmt_wait(tsapi_ceph_mmt_t *monmaptool)
{
    tapi_job_status_t status;

    CEPH_RC(tapi_job_wait(monmaptool->common.job, TE_SEC2MS(60), &status));

    return ceph_common_status_handle("monmaptool", status);
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_osd_cleanup(tsapi_ceph_osd_t *osd)
{
    te_errno rc;

    rc = tapi_job_destroy(osd->common.job, TE_SEC2MS(30));
    tapi_job_factory_destroy(osd->common.factory);

    return rc;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_mon_cleanup(tsapi_ceph_mon_t *mon)
{
    te_errno rc;

    rc = tapi_job_destroy(mon->common.job, TE_SEC2MS(30));
    tapi_job_factory_destroy(mon->common.factory);

    return rc;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_cleanup(tsapi_ceph_t *ceph)
{
    te_errno rc;

    rc = tapi_job_destroy(ceph->common.job, TE_SEC2MS(30));
    tapi_job_factory_destroy(ceph->common.factory);

    return rc;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_mmt_cleanup(tsapi_ceph_mmt_t *monmaptool)
{
    te_errno rc;

    rc = tapi_job_destroy(monmaptool->common.job, TE_SEC2MS(30));
    tapi_job_factory_destroy(monmaptool->common.factory);

    return rc;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_fill_devices(rcf_rpc_server *rpcs, const char *size)
{
    tapi_job_factory_t *fio_factory = NULL;
    tapi_fio *fio = NULL;
    unsigned int count;
    char **blocks = NULL;
    unsigned i;
    te_errno rc;

    tapi_fio_opts fio_opts = {
        .name = "job",
        .filename = NULL,
        /* Set blocksize to 1MB */
        .blocksize = 1048576,
        .numjobs = {
            .value = 1,
            .factor = TAPI_FIO_NUMJOBS_WITHOUT_FACTOR,
        },
        .iodepth = 32,
        .rwmixread = 0,
        .rwtype = TAPI_FIO_RWTYPE_SEQ,
        .ioengine = TAPI_FIO_IOENGINE_LIBAIO,
        .output_path = TE_STRING_INIT,
        .direct = TRUE,
        .exit_on_error = TRUE,
        .rand_gen = NULL,
        .user = NULL,
        .prefix = NULL,
        .size = size,
    };

    rc = tsapi_get_devices(rpcs->ta, &blocks, &count);
    if (rc != 0)
    {
        ERROR("Failed to get block devices");
        goto out;
    }

    for (i = 0; i < count; i++)
    {
        fio_opts.filename = blocks[i];

        rc = tapi_job_factory_rpc_create(rpcs, &fio_factory);
        if (rc != 0)
        {
            ERROR("Failed to create FIO job factory: %r", rc);
            goto out;
        }

        rc = tsapi_fio_create(&fio_opts, fio_factory, rpcs->ta, &fio);
        if (rc != 0)
        {
            ERROR("Failed to create FIO context: %r", rc);
            goto out;
        }

        rc = tapi_fio_start(fio);
        if (rc != 0)
        {
            ERROR("Failed to start FIO tool: %r", rc);
            goto out;
        }

        rc = tsapi_fio_flex_wait(fio);
        if (rc != 0)
        {
            ERROR("Failed to wait for the FIO completion: %r", rc);
            goto out;
        }

        rc = tapi_fio_stop(fio);
        if (rc != 0)
        {
            ERROR("Failed to stop FIO tool: %r", rc);
            goto out;
        }
        tapi_fio_destroy(fio);
        tapi_job_factory_destroy(fio_factory);
    }

out:
    if (rc != 0)
    {
        tapi_fio_destroy(fio);
        tapi_job_factory_destroy(fio_factory);
    }

    if (blocks != NULL)
    {
        for (i = 0; i < count; i++)
            free(blocks[i]);

        free(blocks);
    }
    return rc;
}

/* Script names that are used to preparate Ceph in the container */
static const char *ceph_prepare_scripts[] = {
    [TSAPI_CEPH_CREATE] = "create_ansible_ceph_server.sh",
    [TSAPI_CEPH_REMOVE] = "remove_ansible_ceph_server.sh",
    [TSAPI_CEPH_PREPARE_CLIENT] = "prepare_ceph_client.sh",
};

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_prepare_params(const struct sockaddr *mon_addr,
                          unsigned int block_count)
{
    te_errno rc;
    int osd_num;
    char *host_name;
    int stor_size_gb;
    int dev_size_mb;
    double osd_size;
    te_string dev_name_val = TE_STRING_INIT;
    te_string dev_size_val = TE_STRING_INIT;
    te_string dev_name_key = TE_STRING_INIT;
    te_string dev_size_key = TE_STRING_INIT;
    unsigned int i;
    const char *ceph = "ceph";

    host_name = tsapi_get_backend_param_str(ceph, "host_name");
    if (host_name == NULL)
        return TE_EFAIL;

    rc = tsapi_get_backend_param_int(ceph, "osd_num", &osd_num);
    if (rc != 0)
        return rc;

    rc = tsapi_get_backend_param_int(ceph, "stor_size_gb", &stor_size_gb);
    if (rc != 0)
        return rc;

    if (stor_size_gb <= 0)
    {
        ERROR("Storage size of the CEPH server should be positive value");
        return TE_EINVAL;
    }

    osd_size = (double)stor_size_gb / osd_num;
    dev_size_mb = (stor_size_gb - TSAPI_CEPH_AUX_INFO_SZ_GB) / block_count  * 1024;

    rc = tsapi_inventory_local_add_str(ceph, "CEPH_SERVER_HOST", host_name);
    if (rc != 0)
        return rc;

    rc = tsapi_inventory_local_add_str(ceph, "CEPH_MON_IP",
                                       te_sockaddr_get_ipstr(mon_addr));
    if (rc != 0)
        return rc;

    rc = tsapi_inventory_local_add_int(ceph, "CEPH_OSD_SIZE", osd_size);
    if (rc != 0)
        return rc;

    rc = tsapi_inventory_local_add_int(ceph, "CEPH_OSD_NUM", osd_num);
    if (rc != 0)
        return rc;


    for (i = 0; i < TSAPI_CEPH_BLK_NUM_MAX; i++)
    {
        rc = te_string_append(&dev_name_val,
                              (i < block_count) ? "- name: %s%i" : "",
                              TSAPI_CEPH_BLK_DEV_NAME, i);

        rc = te_string_append(&dev_size_val,
                              (i < block_count) ? "  size_mb: %d" : "",
                              dev_size_mb);
        if (rc != 0)
            goto out;

        rc = te_string_append(&dev_name_key, "CEPH_DEV%i_NAME", i);
        if (rc != 0)
            goto out;

        rc = te_string_append(&dev_size_key, "CEPH_DEV%i_SIZE", i);
        if (rc != 0)
            goto out;

        rc = tsapi_inventory_local_add_str(ceph, dev_name_key.ptr,
                                           dev_name_val.ptr);
        if (rc != 0)
            goto out;

        rc = tsapi_inventory_local_add_str(ceph, dev_size_key.ptr,
                                           dev_size_val.ptr);
        if (rc != 0)
            goto out;

        te_string_reset(&dev_name_key);
        te_string_reset(&dev_size_key);
        te_string_reset(&dev_name_val);
        te_string_reset(&dev_size_val);
    }

out:
    te_string_free(&dev_name_key);
    te_string_free(&dev_size_key);
    te_string_free(&dev_name_val);
    te_string_free(&dev_size_val);

    return rc;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_start_server(rcf_rpc_server *rpcs, const char *helpers_path,
                        const char *virtblk_ci_ansible_path,
                        const char *inventory_path)
{
    rpc_wait_status status;
    te_errno rc = 0;

    rpcs->timeout = TSAPI_CEPH_SERVER_CREATION_TIMEOUT;
    status = rpc_system_ex(rpcs, "%s/%s %s %s",
                           helpers_path,
                           ceph_prepare_scripts[TSAPI_CEPH_CREATE],
                           virtblk_ci_ansible_path, inventory_path);
    if (status.value != 0)
    {
        ERROR("Failed to execute %s: %s", ceph_prepare_scripts[TSAPI_CEPH_CREATE],
              wait_status_flag_rpc2str(status.flag));
        rc = TE_EFAIL;
    }

    return rc;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_stop_server(rcf_rpc_server *rpcs, const char *helpers_path)
{
    rpc_wait_status status;
    te_errno rc = 0;

    rpcs->timeout = TSAPI_CEPH_SERVER_REMOVAL_TIMEOUT;
    status = rpc_system_ex(rpcs, "%s/%s", helpers_path,
                           ceph_prepare_scripts[TSAPI_CEPH_REMOVE]);
    if (status.value != 0)
    {
        ERROR("Failed to execute %s: %s",
              ceph_prepare_scripts[TSAPI_CEPH_REMOVE],
              wait_status_flag_rpc2str(status.flag));
        rc = TE_EFAIL;
    }

    return rc;
}

/* See description in tsapi-ceph.h */
te_errno
tsapi_ceph_prepare_client(rcf_rpc_server *rpcs, const void *data)
{
    const struct sockaddr *clt_addr = (const struct sockaddr *)data;
    rpc_wait_status status;
    char helpers_path[PATH_MAX];
    te_errno rc = 0;

    TSAPI_APPEND_PATH_TO_AGT_DIR(helpers_path, rpcs->ta,
                                 TSAPI_HELPERS_AGT_DIR_NAME);

    status = rpc_system_ex(rpcs, "%s/%s %s %s %s", helpers_path,
                           ceph_prepare_scripts[TSAPI_CEPH_PREPARE_CLIENT],
                           te_sockaddr_get_ipstr(clt_addr),
                           tsapi_use_onload() ? "true" : "false",
                           tsapi_use_plugin() ? "true" : "false");

    if (status.value != 0)
    {
        ERROR("Failed to execute %s: %s",
              ceph_prepare_scripts[TSAPI_CEPH_PREPARE_CLIENT],
              wait_status_flag_rpc2str(status.flag));
        rc = TE_EFAIL;
    }

    return rc;
}
