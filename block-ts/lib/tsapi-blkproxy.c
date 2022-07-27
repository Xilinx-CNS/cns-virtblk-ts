/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper for work with blk-proxy
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */

#include "tsapi-blkproxy.h"
#include "te_vector.h"
#include "tapi_job_opt.h"
#include "tapi_cfg_process.h"
#include "tapi_cfg_cpu.h"
#include "tapi_mem.h"
#include "te_string.h"
#include "te_kvpair.h"
#include "tsapi-backend.h"
#include "tsapi-inventory.h"
#include "te_expand.h"

/** Prefix for configuration strings */
#define TSAPI_BLKPROXY_CONFIG_PREFIX "bp"
#define TSAPI_BLKPROXY_PROXY_DEV_PREFIX "MapVirtioBlk"

/** Default reactor mask of the blk-proxy for the hosts that have 16 CPUs */
#define TSAPI_BLKPROXY_MASK_FOR_16_CPU "0xFFFE"

/** Default reactor mask of the blk-proxy for the hosts that have 8 CPUs */
#define TSAPI_BLKPROXY_MASK_FOR_8_CPU "0xFE"

/** Script name to convert a blk-proxy config from .conf to .json */
#define TSAPI_BLKPROXY_CONFIG_CONVERT_NAME "config_converter.py"

/* See VIRTBLK-1445 */
/** Default number of 2MB hugepages for the blk-proxy. */
#define TSAPI_BLKPROXY_HUGEPAGES_N 768

char *
tsapi_blkproxy_get_logflags(void)
{
    char *val;
    cfg_val_type val_type = CVT_STRING;

    CHECK_RC(cfg_get_instance_fmt(&val_type, &val,
                                  "/local:/blk_proxy:/logflags:"));

    if (strlen(val) == 0)
        return NULL;
    else
        return val;
}

const tsapi_blkproxy_opts tsapi_blkproxy_opts_default_opt = {
    .mem_size = 1200,
    .cpu_mask = TSAPI_BLKPROXY_MASK_FOR_8_CPU,
    .config = NULL,
    .pci_whitelist = TSAPI_BLKPROXY_PCI_ADDRESS,
    .logflags = NULL,
};

/**
 * Check that the SPDK version is older than 21.07
 *
 * @return TRUE if the SPDK version is older than 21.07,
 *         and FALSE otherwise
 */
static te_bool
is_old_spdk(void)
{
    char *old_spdk = getenv("TE_OLD_SPDK");

    if (old_spdk != NULL && strcmp(old_spdk, "true") == 0)
        return TRUE;
    else
        return FALSE;
}

static char *
get_local_config_name(const tsapi_blkproxy_handle *handle, te_bool json_config)
{
    te_string buffer = TE_STRING_INIT;

    te_string_append(&buffer, "%s/%s.%s", handle->config_local_dir.ptr,
                     handle->config_name, json_config ? "json" : "conf");

    return buffer.ptr;
}

static char *
get_agent_config_name(const tsapi_blkproxy_handle *handle, te_bool json_config)
{
    te_string buffer = TE_STRING_INIT;

    te_string_append(&buffer, "%s/%s.%s", handle->config_agent_dir.ptr,
                     handle->config_name, json_config ? "json" : "conf");

    return buffer.ptr;
}

/**
 * Build onload env variables for blk-proxy
 *
 * @param envs te_kvpair list of envs
 *
 * @return Status code
 */
static te_errno
build_blk_proxy_onload_env(te_kvpair_h *envs)
{
    te_errno        rc = 0;
    cfg_handle     *env_handles = NULL;
    char           *env_name = NULL;
    char           *env_value = NULL;
    cfg_val_type    val_type = CVT_STRING;
    unsigned int    n_env = 0;
    unsigned int    i;

    rc = cfg_find_pattern("/local:/blk_proxy:/onload_env:*",
                           &n_env, &env_handles);
    if (rc != 0)
    {
        ERROR("Failed to get Onload env for the blk-proxy, rc=%r", rc);
        goto out;
    }

    for (i = 0; i < n_env; i++)
    {
        rc = cfg_get_inst_name(env_handles[i], &env_name);
        if (rc != 0)
        {
            ERROR("Failed to get Onload env name, rc=%r", rc);
            goto out;
        }

        rc = cfg_get_instance(env_handles[i], &val_type, &env_value);
        if (rc != 0)
        {
            ERROR("Failed to get value of %s, rc=%r", env_value, rc);
            goto out;
        }

        rc = te_kvpair_add(envs, env_name, "%s", env_value);
        if (rc != 0)
        {
            ERROR("Failed to add new entry in list of kvpairs: %r", rc);
            goto out;
        }

        free(env_name);
        free(env_value);

        /* Set to NULL to avoid double freeing */
        env_name = NULL;
        env_value = NULL;
    }

out:
    free(env_handles);
    free(env_name);
    free(env_value);

    return rc;
}

/**
 * Build environment variables to start blk-proxy
 *
 * @param envs te_kvpair list of envs
 *
 * @return Status code
 */
static te_errno
build_blkproxy_env(te_kvpair_h *envs)
{
    te_errno rc;

    rc = build_blk_proxy_onload_env(envs);
    if (rc != 0)
    {
        ERROR("Failed to build Onload env for the blk-proxy");
        return rc;
    }

    return 0;
}

/**
 * Custom function to build logflags option to tapi_job_opt_formatting
 *
 * @param value Pointer to an argument.
 * @param args  Argument vector to which formatted argument
 *              is appended.
 *
 * @return Status code
 */
static te_errno
create_optional_logflags(const void *value, te_vec *args)
{
    const char *str = *(const char *const *)value;
    const char *logflags_delim = ",";
    char *logflags_tmp = NULL;
    char *tmp;
    char *saveptr;
    te_errno rc;

    if (str == NULL)
    {
        rc = TE_ENOENT;
        goto out;
    }

    logflags_tmp = tapi_strdup(str);

    for (tmp = strtok_r(logflags_tmp, logflags_delim, &saveptr);
         tmp != NULL;
         tmp = strtok_r(NULL, logflags_delim, &saveptr))
    {
        rc = te_vec_append_str_fmt(args, "--logflag");
        if (rc != 0)
        {
            rc = TE_EFAIL;
            goto out;
        }

        rc = te_vec_append_str_fmt(args, "%s", tmp);
        if (rc != 0)
        {
            rc = TE_EFAIL;
            goto out;
        }
    }

out:
    free(logflags_tmp);
    return 0;
}

/**
 * Bind logflags argument.
 *
 * @param     _struct           Option struct.
 * @param     _field            Field name of the string in option struct.
 */
#define CREATE_OPT_LOGFLAGS(_struct, _field) \
    {create_optional_logflags, NULL, FALSE, NULL, \
     offsetof(_struct, _field) }

/**
 * Build arguments to start blk-proxy
 *
 * @param envs Vector of envs
 *
 * @return Status code
 */
static te_errno
build_blkproxy_args(const tsapi_blkproxy_handle *handle, te_vec *args,
                    tsapi_blkproxy_opts *opts)
{
    te_errno rc;
    te_vec result_args = TE_VEC_INIT(char *);
    te_vec tmp_args = TE_VEC_INIT(char *);
    const char *ta = handle->rpcs->ta;

    const char *path = NULL;
    char *onload_path = NULL;

    if (tsapi_use_onload())
    {
        onload_path = tsapi_get_onload_path();
        if (onload_path == NULL)
        {
            ERROR("Path to onload is not specified");
            return TE_ENOENT;
        }

        rc = te_vec_append_str_fmt(&result_args, "%s", onload_path);
        if (rc != 0)
        {
            ERROR("Failed to add an element to vector");
            return TE_EFAIL;
        }
    }

    if (opts->config == NULL)
        opts->config = get_agent_config_name(handle, !handle->is_old_spdk);

    if (opts->logflags == NULL)
        opts->logflags = tsapi_blkproxy_get_logflags();

    const tapi_job_opt_bind blkproxy_binds[] = TAPI_JOB_OPT_SET(
        TAPI_JOB_OPT_UINT("-s", FALSE, NULL, tsapi_blkproxy_opts, mem_size),
        TAPI_JOB_OPT_STRING("-m", FALSE, tsapi_blkproxy_opts, cpu_mask),
        TAPI_JOB_OPT_STRING("-c", FALSE, tsapi_blkproxy_opts, config),
        TAPI_JOB_OPT_STRING((is_old_spdk()) ? "-W" : "-A", FALSE,
                            tsapi_blkproxy_opts, pci_whitelist),
        CREATE_OPT_LOGFLAGS(tsapi_blkproxy_opts, logflags)
    );

    path = tsapi_blkproxy_bin(ta);

    rc = tapi_job_opt_build_args(path, blkproxy_binds, opts, &tmp_args);
    if (rc != 0)
        return rc;

    rc = te_vec_append_vec(&result_args, &tmp_args);
    if (rc != 0)
        return rc;

    *args = result_args;

    return 0;
}

static te_errno
build_config_local_dir(const char *ta, te_string *path)
{
    char ta_type[RCF_MAX_ID];
    const char *installdir;
    te_errno rc;

    rc = rcf_ta_name2type(ta, ta_type);
    if (rc != 0)
    {
        ERROR("Failed to get '%s' type: %r", ta, rc);
        return rc;
    }

    installdir = getenv("TE_INSTALL");
    if (installdir == NULL)
    {
        ERROR("TE_INSTALL is not exported");
        return TE_ENOENT;
    }

    return te_string_append(path, "%s/agents/%s/", installdir, ta_type);
}

static te_errno
build_config_agent_dir(const char *ta, te_string *path)
{
    cfg_val_type  cvt = CVT_STRING;
    char         *ta_dir = NULL;
    te_errno      rc;

    rc = cfg_get_instance_fmt(&cvt, &ta_dir, "/agent:%s/dir:", ta);
    if (rc != 0)
    {
        ERROR("Failed to get '%s' agent dir", ta);
        return rc;
    }

    rc = te_string_append(path, "%s", ta_dir);
    free(ta_dir);
    return rc;
}

/* See description in tsapi-blkproxy.h */
te_errno
tsapi_blkproxy_handle_init(tsapi_blkproxy_handle **handle,
                           rcf_rpc_server *rpcs, const char *ps_name,
                           const char *config_name)
{
    tsapi_blkproxy_handle *result = NULL;
    te_errno rc;

    if (rpcs == NULL)
        return TE_EINVAL;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        return TE_ENOMEM;

    result->rpcs = rpcs;
    result->ps_name = (ps_name == NULL) ? TSAPI_BLOCK_PROXY_PS_NAME : ps_name;
    result->config_name = (config_name == NULL) ? TSAPI_BLKPROXY_CONFIG_NAME :
                                                  config_name;
    result->is_old_spdk = is_old_spdk();

    result->config_agent_dir = (te_string)TE_STRING_INIT;
    result->config_local_dir = (te_string)TE_STRING_INIT;

    rc = build_config_agent_dir(rpcs->ta, &result->config_agent_dir);
    if (rc != 0)
    {
        ERROR("Failed to build path to the agent directory: %r", rc);
        free(result);
        return rc;
    }

    rc = build_config_local_dir(rpcs->ta, &result->config_local_dir);
    if (rc != 0)
    {
        te_string_free(&result->config_agent_dir);
        free(result);
        ERROR("Failed to buiid path to the agent build directory: %r", rc);
        return rc;
    }

    *handle = result;

    return 0;
}

void
tsapi_blkproxy_handle_free(tsapi_blkproxy_handle *handle)
{
    if (handle == NULL)
        return;

    free(handle);
}

/**
 * Set reactor mask for blk-proxy based on the number of available CPUs.
 *
 * @param ta      Test agent name
 * @param opts    Blk-proxy options
 *
 * @return Status code
 */
static te_errno
set_cpu_mask(const char *ta, tsapi_blkproxy_opts *opts)
{
    te_errno rc;
    size_t cpu_n;

    rc = tapi_cfg_get_all_threads(ta, &cpu_n, NULL);
    if (rc != 0)
    {
        ERROR("Failed to get CPU number on %s: rc=%r", ta, rc);
        return rc;
    }

    switch(cpu_n)
    {
        case 8:
            opts->cpu_mask = TSAPI_BLKPROXY_MASK_FOR_8_CPU;
            break;

        case 16:
            opts->cpu_mask = TSAPI_BLKPROXY_MASK_FOR_16_CPU;
            break;

        default:
            ERROR("Unsupported CPU number on the ARM host: "
                  "current number: %zu, required nuber: 8 or 16", cpu_n);
            return TE_EFAIL;
    }

    return 0;
}

/* See description in tsapi-blkproxy.h */
te_errno
tsapi_blkproxy_init(const tsapi_blkproxy_handle *handle,
                    tsapi_blkproxy_opts *opts)
{
    te_errno rc;
    te_vec args = TE_VEC_INIT(char *);
    te_kvpair_h envs;
    te_kvpair *p = NULL;
    char **arg;
    int counter = 1;
    rcf_rpc_server *rpcs = handle->rpcs;
    const char *ps_name = handle->ps_name;

    if (handle == NULL)
    {
        ERROR("blk-proxy handle shouldn't be NULL");
        rc = TE_EINVAL;
        goto out;
    }

    rc = set_cpu_mask(rpcs->ta, opts);
    if (rc != 0)
    {
        ERROR("Failed to set CPU mask for blk-proxy: rc=%r", rc);
        goto out;
    }

    rc = build_blkproxy_args(handle, &args, opts);
    if (rc != 0)
    {
        ERROR("Failed to build arguments for blk-proxy");
        goto out;
    }

    te_kvpair_init(&envs);
    rc = build_blkproxy_env(&envs);
    if (rc != 0)
    {
        ERROR("Failed to build envs for blk-proxy");
        goto out;
    }

    rc = tapi_cfg_ps_add(rpcs->ta, ps_name,
                         TE_VEC_GET(char *, &args, 0), FALSE);
    if (rc != 0)
    {
        ERROR("Failed to add %s process", ps_name);
        goto out;
    }

    /* The first element in args vector it is path to bin. */
    te_vec_remove(&args, 0, 1);

    for (p = TAILQ_FIRST(&envs); p != NULL; p = TAILQ_NEXT(p, links))
    {
        rc = tapi_cfg_ps_add_env(rpcs->ta, ps_name, p->key, p->value);
        if (rc != 0)
        {
            ERROR("Failed to add env to %s process", ps_name);
            goto out;
        }
    }

    TE_VEC_FOREACH(&args, arg)
    {
        if (*arg != NULL)
        {
            rc = tapi_cfg_ps_add_arg(rpcs->ta, ps_name, counter++, *arg);
            if (rc != 0)
            {
                ERROR("Failed to add argument to blk-prpxy process");
                goto out;
            }
        }
    }
out:
    te_vec_deep_free(&args);
    te_kvpair_fini(&envs);

    return rc;
}

void
tsapi_blkproxy_options_free(tsapi_blkproxy_opts *opts)
{
    if (opts == NULL)
        return;

    free(opts->logflags);
    free(opts->config);
}

/* See description in tsapi-blkproxy.h */
te_errno
tsapi_blkproxy_kill_orphaned_stacks(rcf_rpc_server *rpcs)
{
    rpc_wait_status status;

    if (tsapi_use_onload())
    {
        status = rpc_system(rpcs, "onload_stackdump -z kill");
        if (status.value != 0)
        {
            ERROR("Failed to kill orphaned stacks: %s",
                  wait_status_flag_rpc2str(status.flag));
            return TE_EFAIL;
        }
    }

    return 0;
}

te_errno
tsapi_blkproxy_start(const tsapi_blkproxy_handle *handle)
{
    te_bool status;
    te_errno rc;
    const char *ps_name = handle->ps_name;
    const char *ta = handle->rpcs->ta;

    /* VIRTBLK-1259: Orphaned stacks should be killed before
     * the blk-proxy start.
     */
    rc = tsapi_blkproxy_kill_orphaned_stacks(handle->rpcs);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_ps_start(ta, ps_name);
    if (rc != 0)
    {
        ERROR("Failed to start %s", ps_name);
        return rc;
    }

    /*
     * VIRTBLK-651: blk-proxy team asked us to add a delay after the start
     *              of the blk-proxy.
     */
    te_motivated_sleep(15, "Waiting for the blk proxy to start successfully");

    rc = tapi_cfg_ps_get_status(ta, ps_name, &status);
    if (rc != 0)
    {
        ERROR("Failed to get status of %s process", ps_name);
        return rc;
    }

    if (!status)
    {
        ERROR("%s did not start", ps_name);
        return TE_EFAIL;
    }

    return 0;
}

te_errno
tsapi_blkproxy_stop(const tsapi_blkproxy_handle *handle)
{
    return tapi_cfg_ps_stop(handle->rpcs->ta, handle->ps_name);
}

te_errno
tsapi_blkproxy_get_status(const tsapi_blkproxy_handle *handle, te_bool *status)
{
    return tapi_cfg_ps_get_status(handle->rpcs->ta, handle->ps_name, status);
}

/**
 * Create JSON config for the blk-proxy based on the INI one.
 *
 * @param handle   blk-porxy handle
 *
 * @return Status code
 */
static te_errno
create_blkproxy_json_config(const tsapi_blkproxy_handle *handle)
{
    te_string cmd = TE_STRING_INIT;
    te_errno rc;

    rc = te_string_append(&cmd, "%s/%s < %s/%s.conf > %s/%s.json",
                          handle->config_local_dir.ptr,
                          TSAPI_BLKPROXY_CONFIG_CONVERT_NAME,
                          handle->config_local_dir.ptr, handle->config_name,
                          handle->config_local_dir.ptr, handle->config_name);
    if (rc != 0)
        return rc;

    if (system(cmd.ptr) == -1)
    {
        ERROR("Failed to convert config to JSON format");
        return TE_EINVAL;
    }

    return 0;
}

/* See description in tsapi-blkproxy.h */
const char *
tsapi_blkproxy_get_config_path(const tsapi_blkproxy_handle *handle,
                               te_bool json_config)
{
    cfg_val_type  cvt = CVT_STRING;
    const char   *ta = handle->rpcs->ta;
    char         *ta_dir = NULL;
    te_errno      rc;
    static char   buffer[PATH_MAX];

    rc = cfg_get_instance_fmt(&cvt, &ta_dir, "/agent:%s/dir:", ta);
    if (rc != 0)
    {
        ERROR("Failed to get %s agent dir", ta);
        return NULL;
    }

    TE_SPRINTF(buffer, "%s/%s.%s", ta_dir, handle->config_name,
               json_config ? "json" : "conf");

    free(ta_dir);

    return buffer;
}

static te_errno
copy_config_to_agent(const tsapi_blkproxy_handle *handle, te_bool json_config)
{
    char *local = NULL;
    char *remote = NULL;
    te_errno rc;

    local = get_local_config_name(handle, json_config);
    if (local == NULL)
    {
        ERROR("Failed to get path to local config file");
        return TE_ENOMEM;
    }

    remote = get_agent_config_name(handle, json_config);
    if (remote == NULL)
    {
        ERROR("Failed to get path to config file on the agent");
        free(local);
        return TE_ENOMEM;
    }

    rc = rcf_ta_put_file(handle->rpcs->ta, 0, local, remote);
    if (rc != 0)
        ERROR("Failed to copy conf file '%s' to agent: %r", local, rc);

    free(local);
    free(remote);

    return rc;
}

static te_errno
copy_configs_to_agent(const tsapi_blkproxy_handle *handle)
{
    te_errno rc;

    /*
     * We should copy both configuration files in order to have
     * consistency state between directories at any time.
     */
    rc = copy_config_to_agent(handle, TRUE);
    if (rc != 0)
        return rc;

    rc = copy_config_to_agent(handle, FALSE);

    return rc;
}

static te_errno
print_blkproxy_conf(const tsapi_blkproxy_handle *handle)
{
    char *path = NULL;
    te_string config = TE_STRING_INIT;
    FILE *file = NULL;
    char source_buf[256];
    te_errno rc = 0;

    path = get_local_config_name(handle, !handle->is_old_spdk);

    file = fopen(path, "r");
    if (file == NULL)
    {
        rc = te_rc_os2te(errno);
        ERROR("Failed to open file '%s': %r", file, rc);
        goto out;
    }

    while (fgets(source_buf, sizeof(source_buf), file) != NULL)
    {
        rc = te_string_append(&config, "%s", source_buf);
        if (rc != 0)
            goto out;
    }

    RING("Blk-proxy config:\n%s", config.ptr);

out:
    free(path);
    te_string_free(&config);

    return rc;
}

/**
 * Expand section's value with given parameters
 *
 * @param val           String with value to expand
 * @param conf_params   Parameters that will be used to expand
 * @param expanded_vap  Expanded value
 *
 * @note This funtion will find #{param} and expand it using
 *       given kv_pair parameters.
 *
 * @return Status code
 */
static te_errno
expand_section_val(const char *val, const te_kvpair_h *conf_params,
                   char **expanded_val)
{
    te_errno rc = 0;
    te_string tmp = TE_STRING_INIT;

    rc = te_string_append(&tmp, val);
    if (rc != 0)
    {
        ERROR("Failed to create te_string from section's value: %r", rc);
        goto out;
    }

    rc = te_string_replace_all_substrings(&tmp, "${", "#{");
    if (rc != 0)
    {
        ERROR("Failed to prepare section's value before expanding: %r", rc);
        goto out;
    }

    rc = te_expand_kvpairs(tmp.ptr, NULL, conf_params, expanded_val);
    if (rc != 0)
    {
        ERROR("Failed to expand value in section's key");
        goto out;
    }

out:
    te_string_free(&tmp);

    return rc;
}

/* See description in tsapi-blkproxy.h */
te_errno
tsapi_blkproxy_create_conf(const tsapi_blkproxy_handle *handle, int argc,
                           char **argv)
{
    te_errno rc;
    te_vec vec = TE_VEC_INIT(char *);
    char **iter;
    te_string section_name = TE_STRING_INIT;
    char *config_path = NULL;
    char *expanded_val = NULL;
    te_bool need_expand;
    tsapi_conf_t conf = TSAPI_CONF_INIT;
    tsapi_conf_section *section;
    tsapi_conf_kv kv;
    te_kvpair_h params;


    rc = tsapi_conf_opts_read(argc, argv, TSAPI_BLKPROXY_CONFIG_PREFIX, &vec);
    if (rc != 0)
        return rc;

    te_kvpair_init(&params);

    rc = tsapi_inventory_local_to_kv_pair(
                             tsapi_backend_mode2str(tsapi_backend_get_mode()),
                             &params);

    TE_VEC_FOREACH(&vec, iter)
    {
        need_expand = FALSE;
        te_string_free(&section_name);

        rc = tsapi_conf_parse_string(*iter, &section_name, &kv);
        if (rc != 0)
        {
            ERROR("Failed to parse configuration string");
            goto out;
        }

        if (strcmp(kv.value, "none") == 0)
            continue;

        section = tsapi_conf_section_get(&conf, section_name.ptr);
        if (section == NULL)
        {
            rc = tsapi_conf_create_section(&conf, section_name.ptr, "");
            if (rc != 0)
            {
                ERROR("Failed to create section %s in conf file",
                      section_name.ptr);
                goto out;
            }
        }

        /** Expand the section's value if it's needed */
        if (strstr(kv.value, "#{") != NULL)
        {
            need_expand = TRUE;
            rc = expand_section_val(kv.value, &params, &expanded_val);
            if (rc != 0)
                goto out;
        }

        rc = tsapi_conf_append(&conf, section_name.ptr, kv.key,
                               need_expand ? expanded_val : kv.value);
        if (rc != 0)
        {
            ERROR("Failed to append %s to %s section", kv.key,
                  section_name.ptr);
            goto out;
        }

        free(expanded_val);
        expanded_val = NULL;
    }

    config_path = get_local_config_name(handle, FALSE);
    if (config_path == NULL)
    {
        ERROR("Failed to get blk-proxy config local path");
        rc = TE_EFAIL;
        goto out;
    }

    rc = tsapi_conf_flush_local(&conf, config_path);
    if (rc != 0)
    {
        ERROR("Failed to flush config");
        goto out;
    }

    if (!handle->is_old_spdk)
    {
        rc = create_blkproxy_json_config(handle);
        if (rc != 0)
        {
            ERROR("Failed to create JSON config");
            goto out;
        }
    }

    rc = print_blkproxy_conf(handle);
    if (rc != 0)
        WARN("Failed to print blk-proxy config: %r", rc);

    rc = copy_configs_to_agent(handle);
out:
    tsapi_conf_free(&conf);
    te_string_free(&section_name);
    te_vec_free(&vec);
    free(config_path);
    free(expanded_val);

    return rc;
}

static te_errno
do_mount_umount(const tsapi_blkproxy_handle *handle, te_bool do_mount)
{
    const char *mountpoint = "$dev$hugepages-2M";
    const char *page_size_kb = "2048";
    te_errno rc;

    if (do_mount)
    {
        rc = cfg_add_instance_fmt(NULL, CFG_VAL(NONE, NULL),
                                  "/agent:%s/mem:/hugepages:%s/mountpoint:%s",
                                  handle->rpcs->ta, page_size_kb, mountpoint);
    }
    else
    {
        rc = cfg_del_instance_fmt(TRUE,
                                  "/agent:%s/mem:/hugepages:%s/mountpoint:%s",
                                  handle->rpcs->ta, page_size_kb, mountpoint);
    }

    return rc;
}

te_errno
tsapi_blkproxy_mount_hugepage_dir(const tsapi_blkproxy_handle *handle)
{
    return do_mount_umount(handle, TRUE);
}

te_errno
tsapi_blkproxy_umount_hugepage_dir(const tsapi_blkproxy_handle *handle)
{
    return do_mount_umount(handle, FALSE);
}

/*
 * Count the number of non-zero bits in the string
 * representation of the mask in hex
 *
 * @param mask_str String representation of the mask in hex
 * @param count    OUT: Number on non-zero bits
 *
 * @return Status code
 */
static te_errno
count_bits_in_mask(const char *mask_str, unsigned int *count)
{
    unsigned long int mask;
    te_errno rc;

    rc = te_strtoul(mask_str, 16, &mask);
    if (rc != 0)
        return rc;

    *count = 0;

    for (; mask != 0; (*count)++)
        mask &= (mask - 1);

    return 0;
}

/*
 * Get the mask name for the MI log
 *
 * @param mask_name Name of the mask from the blk-proxy conf
 *
 * @return Name of the mask for MI log
 */
static char *
map_bp_conf_mask2mi_log_mask(const char *mask_name)
{
    static char *mappings[][2] = {
        {"Cpumask", "Q"},
        {"R_Cpumask", "R"},
        {"W_Cpumask", "W"},
        {NULL, NULL}
    };
    int i;

    for (i = 0; mappings[i][0] != NULL; i++)
    {
        if (strcmp(mappings[i][0], mask_name) == 0)
            return mappings[i][1];
    }

    return NULL;
}

static te_errno
add_thread_mask_to_mi_log(te_mi_logger *logger, long dev_number,
                          const char *mask_name, const char *mask_value)
{
    te_string thread_mask = TE_STRING_INIT;
    te_errno rc;
    unsigned int bits_count;

    rc = te_string_append(&thread_mask, "Dev%d-%s-Threads", dev_number,
                          mask_name);
    if (rc != 0)
        goto out;

    rc = count_bits_in_mask(mask_value, &bits_count);
    if (rc != 0)
        goto out;

    te_mi_logger_add_meas_key(logger, &rc, thread_mask.ptr,
                              "%d", bits_count);
    if (rc != 0)
        goto out;

out:
    te_string_free(&thread_mask);
    return rc;
}

static te_errno
add_qrw_masks_to_mi_log(te_mi_logger *logger, te_kvpair_h *masks,
                        long dev_number)
{
    te_errno rc = 0;
    te_string qrw_mask = TE_STRING_INIT;
    te_string qrw_mask_values = TE_STRING_INIT;
    te_kvpair *p;

    rc = te_string_append(&qrw_mask, "Dev%d-", dev_number);
    if (rc != 0)
        goto out;

    for (p = TAILQ_FIRST(masks); p != NULL; p = TAILQ_NEXT(p, links))
    {
        rc = te_string_append(&qrw_mask, "%s", p->key);
        if (rc != 0)
            goto out;

        rc = te_string_append(&qrw_mask_values, "%s,", p->value);
        if (rc != 0)
            goto out;
    }

    rc = te_string_append(&qrw_mask, "-Mask");
    if (rc != 0)
        goto out;

    te_mi_logger_add_meas_key(logger, &rc, qrw_mask.ptr,
                              "%s", qrw_mask_values.ptr);


out:
    te_string_free(&qrw_mask);
    te_string_free(&qrw_mask_values);

    return rc;
}

static te_errno
add_crossed_masks_info_to_mi_log(te_mi_logger *logger, te_kvpair_h *masks,
                                 long dev_number)
{
    te_errno rc = 0;
    const char *mask_value_str;
    unsigned long int mask = 0;
    unsigned long int crossed_mask = 0;
    te_string crossed_masks_key = TE_STRING_INIT;

    rc = te_string_append(&crossed_masks_key, "Dev%d-Threads-Crossed",
                         dev_number);
    if (rc != 0)
        goto out;

#define CHECK_CROSS_MASK(_mask_name, _op) \
    do {                                                      \
        mask_value_str = te_kvpairs_get(masks, _mask_name);   \
        if (mask_value_str != NULL)                           \
        {                                                     \
            rc = te_strtoul(mask_value_str, 16, &mask);       \
            if (rc != 0)                                      \
                goto out;                                     \
                                                              \
            crossed_mask = crossed_mask _op mask;             \
        }                                                     \
    } while(0)

    /*
     * Order is important as we want to check that read or write masks
     * cross with the reactor mask
     */
    CHECK_CROSS_MASK("W", |);
    CHECK_CROSS_MASK("R", |);
    CHECK_CROSS_MASK("Q", &);
#undef CHECK_CROSS_MASK

    te_mi_logger_add_meas_key(logger, &rc, crossed_masks_key.ptr,
                              "%s", crossed_mask == 0 ? "FALSE" : "TRUE");

out:
    te_string_free(&crossed_masks_key);
    return rc;
}

static te_errno
add_mapvirtblk_section_to_mi_log(te_mi_logger *logger, tsapi_conf_section *section)
{
    long dev_number;
    te_errno rc = 0;
    tsapi_conf_kv *item;
    char *mi_mask_name = NULL;
    te_kvpair_h masks;

    te_kvpair_init(&masks);

    rc = te_strtol(section->name + strlen(TSAPI_BLKPROXY_PROXY_DEV_PREFIX),
                   10, &dev_number);
    if (rc != 0)
        return rc;

    TE_VEC_FOREACH(&section->items, item)
    {
        mi_mask_name = map_bp_conf_mask2mi_log_mask(item->key);
        if (mi_mask_name == NULL)
            continue;

        add_thread_mask_to_mi_log(logger, dev_number,
                                  mi_mask_name, item->value);

        rc = te_kvpair_add(&masks, mi_mask_name, "%s", item->value);
        if (rc != 0)
        {
            ERROR("Failed to add new entry in list of kvpairs: %r", rc);
            goto out;
        }
    }

    rc = add_crossed_masks_info_to_mi_log(logger, &masks, dev_number);
    if (rc != 0)
    {
        ERROR("Failed to add info about masks crossing to MI log: %r", rc);
        goto out;
    }

    rc = add_qrw_masks_to_mi_log(logger, &masks, dev_number);
    if (rc != 0)
    {
        ERROR("Failed to add QRW masks to MI log: %r", rc);
        goto out;
    }

out:
    te_kvpair_fini(&masks);
    return rc;

}

static te_errno
add_dev_type_section_to_mi_log(te_mi_logger *logger, tsapi_conf_section *section)
{
    te_errno rc;

    te_mi_logger_add_meas_key(logger, &rc, "Dev-Type", "%s", section->name);
    return rc;
}

static te_errno
add_dev_count_to_mi_log(te_mi_logger *logger, tsapi_conf_t *conf)
{
    tsapi_conf_section *section;
    int count = 0;
    te_errno rc;

    TE_VEC_FOREACH(&conf->sections, section)
    {
        if (strcmp_start(TSAPI_BLKPROXY_PROXY_DEV_PREFIX, section->name) == 0)
            count++;
    }

    te_mi_logger_add_meas_key(logger, &rc, "Dev", "%d", count);
    return rc;
}

/* See description in tsapi-blkproxy.h */
te_errno
tsapi_blkproxy_add_conf_to_mi_log(te_mi_logger *logger, tsapi_conf_t *conf,
                                  te_bool multipath)
{
    tsapi_conf_section *section;
    te_errno rc = 0;
    te_bool is_dev_type_add = FALSE;

    te_mi_logger_add_meas_key(logger, &rc, "Multipath",
                              (multipath) ? "True" : "False");
    if (rc != 0)
    {
        ERROR("Failed to add 'multipath' key ti MI log, rc = %r", rc);
        return rc;
    }

    rc = add_dev_count_to_mi_log(logger, conf);
    if (rc != 0)
    {
        ERROR("Failed to add the number of devices to MI log, rc = %r", rc);
        return rc;
    }

    TE_VEC_FOREACH(&conf->sections, section)
    {
        if (strcmp_start(TSAPI_BLKPROXY_PROXY_DEV_PREFIX, section->name) == 0)
        {
            rc = add_mapvirtblk_section_to_mi_log(logger, section);
            if (rc != 0)
            {
                ERROR("Failed to parse %s section, rc = %r",
                      TSAPI_BLKPROXY_PROXY_DEV_PREFIX, rc);
                break;
            }
        }
        else if (!is_dev_type_add)
        {
            rc = add_dev_type_section_to_mi_log(logger, section);
            if (rc != 0)
            {
                ERROR("Failed to parse dev-type, rc = %r", rc);
                break;
            }
            is_dev_type_add = TRUE;
        }
    }

    return rc;
}
