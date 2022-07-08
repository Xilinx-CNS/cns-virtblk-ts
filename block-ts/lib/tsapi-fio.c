/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for fio manipulation
 *
 * Helper for fio manipulation
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "tapi_cfg_pci.h"
#include "tsapi-fio.h"
#include "tsapi.h"
#include "tsapi-blkproxy.h"
#include "tsapi-conf.h"

/* See description in tsapi-fio.h */
void
tsapi_fio_opts_read(tapi_fio_opts *opts, int argc, char **argv, const char *ta)
{
    unsigned int count;
    char **blocks;

    CHECK_RC(tsapi_get_devices(ta, &blocks, &count));

    /* By default using first block */
    opts->filename = blocks[0];

    tsapi_fio_opts_read_suf(opts, argc, argv, "");
}

static const char *
fio_suffix(const char *opt, const char *suffix)
{
    static char option[256];

    TE_SPRINTF(option, "%s%s", opt, suffix);

    return option;
}

#define FIO_HAS_PARAM(_name, _suffix)                                          \
    (test_find_param(argc, argv, _name) != NULL)

#define FIO_UINT_PARAM(_name, _suffix)                                         \
    test_get_uint_param(argc, argv, fio_suffix(_name, _suffix))

#define FIO_INT_PARAM(_name, _suffix)                                          \
    test_get_int_param(argc, argv, fio_suffix(_name, _suffix))

#define FIO_STRING_PARAM(_name, _suffix)                                       \
    test_get_string_param(argc, argv, fio_suffix(_name, _suffix))

#define FIO_NUMJOBS_PARAM(_name, _suffix)                                      \
    test_get_fio_numjobs_param(argc, argv, fio_suffix(_name, _suffix))

#define FIO_ENUM_PARAM(_name, _suffix, _maps)                                  \
    test_get_enum_param(argc, argv, fio_suffix(_name, _suffix),                \
        (struct param_map_entry []) { _maps, {NULL, 0} })

/* See description in tsapi-fio.h */
void
tsapi_fio_opts_read_suf(tapi_fio_opts *opts, int argc, char **argv,
                        const char *suffix)
{
    opts->direct = TRUE;
    opts->rand_gen = "tausworthe64";
    opts->blocksize = FIO_UINT_PARAM("blocksize", suffix);
    opts->ioengine = FIO_ENUM_PARAM("ioengine", suffix,
                                    TAPI_FIO_IOENGINE_MAPPING_LIST);
    opts->rwtype = FIO_ENUM_PARAM("rwtype", suffix,
                                  TAPI_FIO_RWTYPE_MAPPING_LIST);
    opts->runtime_sec = FIO_INT_PARAM("runtime_sec", suffix);
    opts->iodepth = FIO_UINT_PARAM("iodepth", suffix);
    opts->numjobs = FIO_NUMJOBS_PARAM("numjobs", suffix);
    opts->rwmixread = FIO_UINT_PARAM("rwmixread", suffix);

    if (FIO_HAS_PARAM("name", suffix))
        opts->name = FIO_STRING_PARAM("name", suffix);

    if (FIO_HAS_PARAM("user", suffix))
        opts->user = FIO_STRING_PARAM("user", suffix);

    if (FIO_HAS_PARAM("rbdname", suffix))
        opts->rbdname = FIO_STRING_PARAM("rbdname", suffix);

    if (FIO_HAS_PARAM("pool", suffix))
        opts->pool = FIO_STRING_PARAM("pool", suffix);

    if (FIO_HAS_PARAM("size", suffix))
        opts->size = FIO_STRING_PARAM("size", suffix);

}

static int16_t
get_timeout(const tapi_fio_opts *opts, const int16_t additional_timeout)
{
    const int16_t error = 30;
    const int16_t five_minutes = 5 * 60;
    const double numjobs_coef = ((double)opts->numjobs.value) /
                                TAPI_FIO_MAX_NUMJOBS;
    int16_t timeout = opts->runtime_sec +
                      round(five_minutes * numjobs_coef) + error;

    if (timeout > 0) {
        return timeout + additional_timeout;
    }

    return timeout;
}

 /* See description in tsapi-fio.h */
te_errno
tsapi_fio_flex_wait(tapi_fio *fio)
{
    int16_t timeout = TAPI_FIO_TIMEOUT_DEFAULT;

    if (fio->app.opts.numjobs.value > 248 || fio->app.opts.blocksize > 1024)
    {
        timeout = get_timeout(&fio->app.opts, TE_SEC2MS(60));
    }

    return tapi_fio_wait(fio, timeout);
}

/* See description in tsapi-fio.h */
te_errno
tsapi_fio_multi_init(rcf_rpc_server *rcps, tsapi_fio_context *ctxs, int n)
{
    int i;
    te_errno rc;

    for (i = 0; i < n; i++)
    {
        te_string name = TE_STRING_INIT_STATIC(32);

        te_string_append(&name, "fio%d", i);
        rc = rcf_rpc_server_create_process(rcps, name.ptr, 0,
                                           &(ctxs[i].forked_rpcs));

        ctxs[i].opts = TAPI_FIO_OPTS_DEFAULTS;

        if (rc != 0)
            return rc;

        rc = tapi_job_factory_rpc_create(ctxs[i].forked_rpcs,
                                         &ctxs[i].forked_factory);
        if (rc != 0)
            return rc;
    }

    return 0;
}

/* See description in tsapi-fio.h */
te_errno
tsapi_fio_create(const tapi_fio_opts *options, tapi_job_factory_t *factory,
                 const char *ta, tapi_fio **fio)
{
    char path[PATH_MAX];
    te_errno rc = 0;

    if (ta != NULL)
        TSAPI_APPEND_PATH_TO_AGT_DIR(path, ta, "fio");


    *fio = tapi_fio_create(options, factory, path);

    return rc;
}

/* See description in tsapi-fio.h */
te_errno
tsapi_fio_multi_fini(tsapi_fio_context *ctxs, int n)
{
    int i;
    te_errno rc;

    for (i = 0; i < n; i++)
    {
        tapi_job_factory_destroy(ctxs[i].forked_factory);
        if ((rc = rcf_rpc_server_destroy(ctxs[i].forked_rpcs)) != 0)
            return rc;
    }

    return 0;
}

/*
 * Add argument in the "key=value" format to MI log
 *
 * @param logger MI logger instance
 * @param arg    Argument
 *
 * @return Status code
 */
static te_errno
add_fio_arg_to_mi_log(te_mi_logger *logger, const char *arg)
{
    char *ch;
    te_string key = TE_STRING_INIT;
    te_string val = TE_STRING_INIT;
    te_errno rc;

    ch = strstr(arg, "=");
    if (ch == NULL)
    {
        ERROR("Invalid argument string format");
        return TE_EFAIL;
    }

    rc = te_string_append(&key, "FIO-%s", arg);
    if (rc != 0)
        goto out;

    te_string_cut(&key, strlen(arg) - (ch - arg));

    rc = te_string_append(&val, "%s", ch + 1);
    if (rc != 0)
        goto out;

    te_mi_logger_add_meas_key(logger, &rc, key.ptr, "%s", val.ptr);

out:
    te_string_free(&key);
    te_string_free(&val);
    return rc;
}

/* See description in tsapi-fio.h */
te_errno
tsapi_fio_args_to_mi_log(te_mi_logger *logger, tapi_fio *fio)
{
    char **iter;
    unsigned int i;
    te_string cmd = TE_STRING_INIT;
    te_errno rc;
    const char *list_args[] = {
        TSAPI_FIO_ARGS_FOR_MI_LOG
    };

    const char *list_exclude_args[] = {
        /*
         * This argument contains temporary path so it is
         * not useful for the user
         */
        "output",
    };

    rc = te_string_append(&cmd, "fio");
    if (rc != 0)
    {
        te_string_free(&cmd);
        return rc;
    }

    TE_VEC_FOREACH(&(fio->app.args), iter)
    {
        if (*iter == NULL)
            break;

        if (strcmp_start("--", *iter) != 0)
            continue;

        for (i = 0; i < (sizeof(list_args) / sizeof(list_args[0])); i++)
        {
            if (strcmp_start(list_args[i], *iter + strlen("--")) == 0)
            {
                rc = add_fio_arg_to_mi_log(logger, *iter + strlen("--"));
                if (rc != 0)
                {
                    te_string_free(&cmd);
                    return rc;
                }
            }
        }

        for (i = 0;
             i < (sizeof(list_exclude_args) / sizeof(list_exclude_args[0]));
             i++)
        {
            if (strcmp_start(list_exclude_args[i], *iter + strlen("--")) != 0)
            {
                rc = te_string_append(&cmd, " %s", *iter);
                if (rc != 0)
                {
                    te_string_free(&cmd);
                    return rc;
                }
            }
        }

    }

    te_mi_logger_add_meas_key(logger, &rc, "FIO-cmd", "%s", cmd.ptr);
    te_string_free(&cmd);
    return rc;
}

/* See description in tsapi-fio.h */
te_errno
tsapi_fio_mi_report(tapi_fio *fio, const tapi_fio_report *report,
                    const char *blk_proxy_conf, const char *ta,
                    te_bool multipath)
{
    tsapi_conf_t conf = TSAPI_CONF_INIT;
    te_mi_logger *logger;
    te_errno rc;
    char config_path[PATH_MAX];

    rc = te_mi_logger_meas_create("fio", &logger);
    if (rc != 0)
        return rc;

    rc = tapi_fio_mi_report(logger, report);
    if (rc != 0)
    {
        ERROR("Failed to get FIO MI report");
        return rc;
    }

    TSAPI_APPEND_PATH_TO_AGT_DIR(config_path, ta, "%s.conf", blk_proxy_conf);

    rc = tsapi_conf_read_from_file(ta, config_path, &conf);
    if (rc != 0)
    {
        ERROR("Failed to parse blk-proxy configuration file");
        return rc;
    }

    rc = tsapi_blkproxy_add_conf_to_mi_log(logger, &conf, multipath);
    if (rc != 0)
    {
        ERROR("Failed to add blk-proxy config to MI log");
        return rc;
    }

    rc = tsapi_fio_args_to_mi_log(logger, fio);
    if (rc != 0)
    {
        ERROR("Failed to add FIO arguments to MI log");
        return rc;
    }

    te_mi_logger_destroy(logger);
    return 0;
}
