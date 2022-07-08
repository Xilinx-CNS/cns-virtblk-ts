/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Block Test Suite
 *
 * Main header file containing useful helpers
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "block-ts.h"
#include "tsapi.h"
#include "te_vector.h"
#include "tapi_mem.h"
#include "te_sockaddr.h"
#include "tapi_rpc_stdio.h"
#include "tapi_cfg_process.h"
#include "tsapi-conf.h"

#define BLOCK_TS_CEPH_CONF       "/etc/ceph/ceph.conf"
#define BLOCK_TS_CEPH_MONMAP     "/tmp/monmap"
#define BLOCK_TS_CEPH_FSID       "6cac38af-505c-4c8d-9aeb-4fbe814313b0"

#define BLOCK_TS_CEPH_MON_NAME   "main"
/* Default port for CEPH monitor */
#define BLOCK_TS_CEPH_MON_PORT   6789

#define BLOCK_TS_CEPH_OSD_FIRST  "0"
#define BLOCK_TS_CEPH_OSD_HOST   "disk_host"

#define BLOCK_TS_CEPH_MON_DATA   "/var/lib/ceph/mon"
#define BLOCK_TS_CEPH_OSD_DATA   "/var/lib/ceph/osd"

/* Path to ceph logs dir */
#define BLOCK_TS_CEPH_LOG_DIR_PATH "/var/log/ceph"

/* Ceph logs file name */
#define BLOCK_TS_CEPH_LOG_FILE_NAME "ceph.log"

/* Name of the tool which is used to print ceph logs */
#define BLOCK_TS_CEPH_LOG_TOOL_NAME "tail"

static te_errno
gtest_setenv_ld_preload(rcf_rpc_server *rpcs, const char *ceph_lib,
                        const char *onload_lib)
{
    size_t i;
    int rc;
    const char *libs[] = {
        "libcls_cas.so",
        "libcls_log.so",
        "libcls_lua.so",
        "libcls_rgw.so",
        "libcls_user.so",
    };
    te_string value = TE_STRING_INIT;

    for (i = 0; i < TE_ARRAY_LEN(libs); i++) {
        if ((rc = te_string_append(&value, "%s/%s:", ceph_lib, libs[i])) != 0) {
            te_string_free(&value);
            return rc;
        }
    }

    if ((rc = te_string_append(&value, "%s/%s", onload_lib,
                               "libonload_ext.so.2")) != 0)
    {
        te_string_free(&value);
        return rc;
    }

    if (rpc_setenv(rpcs, "LD_PRELOAD", value.ptr, FALSE) != 0)
    {
        te_string_free(&value);
        return RPC_ERRNO(rpcs);
    }

    te_string_free(&value);
    return 0;
}

static te_errno
block_ts_test_get_features_disable(block_ts_rbd_features *features,
                                   const char *feature)
{
    te_errno rc;
    block_ts_rbd_feature feat;

    if ((rc = block_ts_rbd_feature_str2feat(feature, &feat)) != 0)
        return rc;

    (*features) &= ~feat;
    return 0;
}

static te_errno
block_ts_test_get_features_keyword(block_ts_rbd_features *features,
                                   const char *keyword)
{
    if (strcmp(keyword, "UNSET_FEATURES") == 0)
    {
        *features = 0;
        return 0;
    }

    return TE_EINVAL;
}

static te_errno
block_ts_test_get_features(const char *features_string,
                           block_ts_rbd_features *features)
{
    te_errno rc;
    char *fstr = NULL;
    char *to_free = NULL;
    const block_ts_rbd_features default_features =
        BLOCK_TS_RBD_FEATURE_LAYERING        |
        BLOCK_TS_RBD_FEATURE_STRIPINGV2      |
        BLOCK_TS_RBD_FEATURE_EXCLUSIVE_LOCK  |
        BLOCK_TS_RBD_FEATURE_OBJECT_MAP      |
        BLOCK_TS_RBD_FEATURE_FAST_DIFF       |
        BLOCK_TS_RBD_FEATURE_DEEP_FLATTEN    |
        BLOCK_TS_RBD_FEATURE_JOURNALING      |
        BLOCK_TS_RBD_FEATURE_DATA_POOL;

    block_ts_rbd_features result = default_features;

    to_free = (char *)tapi_strdup(features_string);

    for (fstr = strtok(to_free, ":"); fstr != NULL; fstr = strtok(NULL, ":"))
    {
        switch (fstr[0])
        {
            case '-':
                rc = block_ts_test_get_features_disable(&result, fstr + 1);
                break;
            default:
                rc = block_ts_test_get_features_keyword(&result, fstr);
        }

        if (rc != 0)
        {
            free(to_free);
            return rc;
        }
    }

    *features = result;
    free(to_free);
    return 0;
}

static te_errno
block_ts_test_specific_setenv(rcf_rpc_server *rpcs,
                              int argc, char *argv[])
{
    te_errno rc;
    const char *features = "";
    block_ts_rbd_features rbd_features = 0;

    if (TEST_HAS_PARAM(features))
        TEST_GET_STRING_PARAM(features);

    if ((rc = block_ts_test_get_features(features, &rbd_features)) != 0)
        return rc;

    if (rbd_features == 0)
    {
        if (rpc_unsetenv(rpcs, "RBD_FEATURES") != 0)
            return RPC_ERRNO(rpcs);
    }
    else
    {
        te_string value = TE_STRING_INIT_STATIC(128);
        te_string ring_val = TE_STRING_INIT_STATIC(256);

        if ((rc = te_string_append(&value, "%d", rbd_features)) != 0)
            return rc;

        if ((rc = block_ts_rbd_features_string(rbd_features, &ring_val)) != 0)
            return rc;

        if (rpc_setenv(rpcs, "RBD_FEATURES", value.ptr, TRUE) != 0)
            return RPC_ERRNO(rpcs);
    }

    return 0;
}

/* See description in block-ts.h */
te_errno
block_ts_gtest_setenv(rcf_rpc_server *rpcs, int argc, char *argv[])
{
    te_errno rc;
    tapi_job_factory_t *factory = NULL;
    const char *ceph_lib = getenv("CEPH_LIB");
    const char *onload_lib = getenv("ONLOAD_LIB");

    if (ceph_lib == NULL)
    {
        RING("CEPH_LIB not exist");
        return 0;
    }

    if (rpc_setenv(rpcs, "CEPH_LIB", ceph_lib, FALSE) != 0)
        return RPC_ERRNO(rpcs);

    if ((rc = block_ts_test_specific_setenv(rpcs, argc, argv)) != 0)
        return rc;

    if ((rc = gtest_setenv_ld_preload(rpcs, ceph_lib, onload_lib)) != 0) {
        return RPC_ERRNO(rpcs);
    }

    if ((rc = tapi_job_factory_rpc_create(rpcs, &factory)) != 0)
        return rc;

    rc = tapi_job_factory_set_path(factory);
    tapi_job_factory_destroy(factory);

    return rc;
}

struct log_conv {
    block_ts_log_conv conv;
    tapi_job_channel_t *channel;
    tapi_job_buffer_t buffer;
};

#define LOG_CONV_INIT(_conv) (struct log_conv) {   \
    .conv = _conv,                                 \
    .buffer = TAPI_JOB_BUFFER_INIT,                \
}

static te_vec log_convs = TE_VEC_INIT(struct log_conv);

/* See description in block-ts.h */
te_errno
block_ts_log_conv_add(tapi_job_channel_t *in, block_ts_log_conv conv)
{
    te_errno rc;
    struct log_conv log_conv = LOG_CONV_INIT(conv);

    assert(conv.re != NULL);

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(in), "Log Conversation",
                                true, TE_LL_WARN, &log_conv.channel);
    if (rc != 0)
        return rc;

    rc = tapi_job_filter_add_regexp(log_conv.channel, log_conv.conv.re, 0);
    if (rc != 0)
        return rc;

    if ((rc = te_vec_append(&log_convs, &log_conv)) != 0)
        return rc;

    return 0;
}

static void
log_conv_action_test_skip(block_ts_log_conv *conv)
{
    if (conv->verdict != NULL)
        TEST_SKIP("%s", conv->verdict);
    TEST_SKIP("");
}

/* See description in block-ts.h */
te_errno
block_ts_log_conv_run(void)
{
    te_errno rc;
    struct log_conv *log_conv = NULL;

    TE_VEC_FOREACH(&log_convs, log_conv) {
        block_ts_log_conv conv = log_conv->conv;
        while (!log_conv->buffer.eos)
        {
            rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(log_conv->channel),
                                  -1, &log_conv->buffer);
            if (rc != 0)
            {
                te_vec_free(&log_convs);
                return rc;
            }
        }

        if (strcmp(log_conv->buffer.data.ptr, "") != 0)
        {
            switch (log_conv->conv.action)
            {
                case BLOCK_TS_LOG_CONV_TEST_SKIP:
                    te_vec_free(&log_convs);
                    log_conv_action_test_skip(&conv);
                    break;
            }
        }
    }

    te_vec_free(&log_convs);
    return 0;
}

/* See description in block-ts.h */
const char *
block_ts_gtest_absbin(const char *bin)
{
    te_errno rc;
    const char *path = getenv("CEPH_BIN");
    static te_string absbin = TE_STRING_INIT;

    if (path == NULL)
    {
        ERROR_ARTIFACT("CEPH_BIN not exist");
        return NULL;
    }

    te_string_reset(&absbin);
    if ((rc = te_string_append(&absbin, "%s/%s", path, bin)) != 0)
    {
        ERROR("Cannot generate full binary path for binary '%s': %r", bin, rc);
        return NULL;
    }

    return absbin.ptr;
}

typedef struct block_ts_rbd_feature_map_entry {
    const char *name;
    block_ts_rbd_feature feature;
} block_ts_rbd_feature_map_entry;

#define BLOCK_TS_RBD_FEATURE_MAP_ENTRY(_name) \
    { .name = #_name, .feature = BLOCK_TS_RBD_FEATURE_##_name }

block_ts_rbd_feature_map_entry block_ts_rbd_feature_map_entries[] = {
    BLOCK_TS_RBD_FEATURE_MAP_ENTRY(LAYERING),
    BLOCK_TS_RBD_FEATURE_MAP_ENTRY(STRIPINGV2),
    BLOCK_TS_RBD_FEATURE_MAP_ENTRY(EXCLUSIVE_LOCK),
    BLOCK_TS_RBD_FEATURE_MAP_ENTRY(OBJECT_MAP),
    BLOCK_TS_RBD_FEATURE_MAP_ENTRY(FAST_DIFF),
    BLOCK_TS_RBD_FEATURE_MAP_ENTRY(DEEP_FLATTEN),
    BLOCK_TS_RBD_FEATURE_MAP_ENTRY(JOURNALING),
    BLOCK_TS_RBD_FEATURE_MAP_ENTRY(DATA_POOL),
    BLOCK_TS_RBD_FEATURE_MAP_ENTRY(OPERATIONS),
    BLOCK_TS_RBD_FEATURE_MAP_ENTRY(MIGRATING)
};

/* See description in block-ts.h */
te_errno
block_ts_rbd_features_string(block_ts_rbd_features features, te_string *str)
{
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(block_ts_rbd_feature_map_entries); i++)
    {
        te_errno rc;
        block_ts_rbd_feature_map_entry entry =
            block_ts_rbd_feature_map_entries[i];

        if (features & entry.feature)
        {
            rc = te_string_append(str, "%s (%d)\n", entry.name, entry.feature);
            if (rc != 0)
            {
                te_string_free(str);
                return rc;
            }
        }
    }

    return 0;
}

/* See description in block-ts.h */
te_errno
block_ts_rbd_feature_str2feat(const char *feature,
                              block_ts_rbd_feature *feature_val)
{
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(block_ts_rbd_feature_map_entries); i++)
    {
        block_ts_rbd_feature_map_entry entry =
            block_ts_rbd_feature_map_entries[i];

        if (strcmp(feature, entry.name) == 0)
        {
            *feature_val = entry.feature;
            return 0;
        }
    }

    return TE_ESRCH;
}

static te_bool
ceph_sh_env_contain_path(const char *ta, const char *path)
{
    cfg_val_type val_type = CVT_STRING;
    char *current_path;
    te_errno rc;

    rc = cfg_get_instance_fmt(&val_type, &current_path,
                              "/agent:%s/env:PATH", ta);

    if (rc == TE_RC(TE_CONF_API, TE_ENOENT))
        return FALSE;
    else if (rc == 0)
        return strstr(current_path, path);
    else
      TEST_FAIL("Unknown error rc=%r");

    return FALSE;
}

static te_errno
block_ts_ceph_cluster_env_prepare(rcf_rpc_server *rpcs,
                                  tsapi_ceph_common_t *common)
{
    te_errno rc = 0;
    const char *ceph_lib = getenv("CEPH_LIB");
    const char *ceph_bin = getenv("CEPH_BIN");
    const char *ceph_pybind = getenv("CEPH_PYBIND");

    if (ceph_lib == NULL)
    {
        RING("CEPH_LIB variable is empty");
        return 0;
    }

    if (ceph_bin == NULL)
    {
        ERROR("CEPH_BIN variable is empty");
        return TE_EFAIL;
    }

    if (ceph_sh_env_contain_path(rpcs->ta, ceph_bin) == FALSE)
    {
        rc = tapi_sh_env_ta_path_append(rpcs->ta, ceph_bin);
        if (rc != 0)
            return rc;
    }

    if (rpc_setenv(rpcs, "LD_LIBRARY_PATH", ceph_lib, TRUE) != 0)
        return RPC_ERRNO(rpcs);
    if (rpc_setenv(rpcs, "PYTHONPATH", ceph_pybind, TRUE) != 0)
        return RPC_ERRNO(rpcs);

    return tapi_job_factory_set_path(common->factory);
}

/* See description in block-ts.h */
te_errno
block_ts_ceph_cluster_conf_create(rcf_rpc_server *rpcs,
                                  const struct sockaddr *ceph_net_ip,
                                  const struct sockaddr *ceph_mon_addr,
                                  unsigned int ceph_net_ip_pfx)
{
    te_errno rc;
    te_bool is_enabled_logging = FALSE;
    tsapi_conf_t conf = TSAPI_CONF_INIT;
    te_string ceph_net_str = TE_STRING_INIT;
    te_string ceph_mon_addr_str = TE_STRING_INIT;
    const char *ceph_lib = getenv("CEPH_LIB");

    if (ceph_lib == NULL)
        RING("CEPH_LIB variable is empty");

    if ((rc = te_string_append(&ceph_net_str, "%s/%d",
                               te_sockaddr_get_ipstr(ceph_net_ip),
                               ceph_net_ip_pfx)) != 0)
    {
        te_string_free(&ceph_net_str);
        return rc;
    }


    if ((rc = te_string_append(&ceph_mon_addr_str, "%s:%d/0",
                               te_sockaddr_get_ipstr(ceph_mon_addr),
                               BLOCK_TS_CEPH_MON_PORT)) != 0)
    {
        te_string_free(&ceph_mon_addr_str);
        return rc;
    }

    tsapi_conf_create_section(&conf, "global", "=");

#define CEPH_CONF(_key, _value)                                \
    do {                                                       \
        rc = tsapi_conf_append(&conf, "global", _key, _value); \
                                                               \
        if (rc != 0)                                           \
            goto out;                                          \
    } while(0)

    CEPH_CONF("fsid", BLOCK_TS_CEPH_FSID);
    CEPH_CONF("public network", ceph_net_str.ptr);
    CEPH_CONF("cluster network", ceph_net_str.ptr);

    /* We are not currently supporting the v2 messaging format. */
    CEPH_CONF("ms bind msgr1", "true");
    CEPH_CONF("ms bind msgr2", "false");

    /* We don't need redundancy, ignore warnings */
    CEPH_CONF("mon warn on pool no redundancy", "false");
    /* This warning can briefly appear after starting the cluster, ignore it */
    CEPH_CONF("mon warn on pool pg num not power of two", "false");

    CEPH_CONF("auth cluster required", "none");
    CEPH_CONF("auth service required", "none");
    CEPH_CONF("auth client required", "none");

    /* Disable this automatic CRUSH map management */
    CEPH_CONF("osd crush update on start", "false");

    CEPH_CONF("osd objectstore", "filestore");
    CEPH_CONF("osd pool default size", "1");
    CEPH_CONF("osd pool default min size", "1");
    CEPH_CONF("osd crush chooseleaf type", "0");
    CEPH_CONF("mon initial members", BLOCK_TS_CEPH_MON_NAME);
    CEPH_CONF("mon host", ceph_mon_addr_str.ptr);

    /* Add address for CEPH monitor */
    CEPH_CONF("mon addr", ceph_mon_addr_str.ptr);

    if (tsapi_use_onload())
    {
        CEPH_CONF("ms type", "async+onload");
        CEPH_CONF("ms async onload buffer pool size", "2M");
        CEPH_CONF("ms nocrc", "true");
        CEPH_CONF("ms crc data", "false");
        CEPH_CONF("rbd cache", "false");

        if (tsapi_use_plugin())
            CEPH_CONF("ms async onload rx plugin", "true");
    }

    rc = tsapi_get_backend_param_bool("ceph", "enable_logging",
                                      &is_enabled_logging);
    if (rc != 0)
        goto out;

    if (is_enabled_logging)
    {
        te_string ceph_log_path = TE_STRING_INIT;

        if ((rc = te_string_append(&ceph_log_path, "%s/%s/%s",
                                   BLOCK_TS_CEPH_LOG_DIR_PATH,
                                   BLOCK_TS_CEPH_FSID,
                                   BLOCK_TS_CEPH_LOG_FILE_NAME)) != 0)
        {
            te_string_free(&ceph_log_path);
            return rc;
        }

        CEPH_CONF("log_to_file", "true");
        CEPH_CONF("log_file", ceph_log_path.ptr);

        te_string_free(&ceph_log_path);
    }

#undef CEPH_CONF

    rc = tsapi_conf_flush(rpcs, &conf, BLOCK_TS_CEPH_CONF);

out:
    tsapi_conf_free(&conf);
    te_string_free(&ceph_net_str);
    te_string_free(&ceph_mon_addr_str);

    return rc;
}

/* See description in block-ts.h */
te_errno
block_ts_ceph_cluster_monmap_create(rcf_rpc_server *rpcs,
                                    const struct sockaddr *ceph_mon_addr)
{
    tsapi_ceph_mmt_t mmt = TSAPI_CEPH_MMT_T_INIT;

    mmt.opts.config = BLOCK_TS_CEPH_CONF;
    mmt.opts.fsid = BLOCK_TS_CEPH_FSID;
    mmt.opts.need_create = TRUE;
    mmt.opts.monmap = BLOCK_TS_CEPH_MONMAP;
    mmt.opts.add = &(tsapi_ceph_mmt_add_opt) {
        .ip = te_sockaddr_get_ipstr(ceph_mon_addr),
        .name = BLOCK_TS_CEPH_MON_NAME,
        .port = BLOCK_TS_CEPH_MON_PORT,
    };

    CHECK_RC(tsapi_ceph_mmt_init(rpcs, &mmt));
    CHECK_RC(block_ts_ceph_cluster_env_prepare(rpcs, &mmt.common));
    CHECK_RC(tsapi_ceph_mmt_start(&mmt));
    CHECK_RC(tsapi_ceph_mmt_wait(&mmt));
    CHECK_RC(tsapi_ceph_mmt_cleanup(&mmt));

    return 0;
}

static te_errno
block_ts_ceph_cluster_mon_helper_start(rcf_rpc_server *rpcs, te_bool mkfs)
{
    tsapi_ceph_mon_t mon = TSAPI_CEPH_MON_T_INIT;

    mon.opts.common_opts.config = BLOCK_TS_CEPH_CONF;
    mon.opts.common_opts.id = BLOCK_TS_CEPH_MON_NAME;
    mon.opts.common_opts.monmap = BLOCK_TS_CEPH_MONMAP;
    mon.opts.common_opts.mkfs = mkfs;

    CHECK_RC(tsapi_ceph_mon_init(rpcs, &mon));
    CHECK_RC(block_ts_ceph_cluster_env_prepare(rpcs, &mon.common));
    CHECK_RC(tsapi_ceph_mon_start(&mon));
    CHECK_RC(tsapi_ceph_mon_wait(&mon));
    CHECK_RC(tsapi_ceph_mon_cleanup(&mon));

    return 0;
}

static te_errno
ceph_rpc_mkdirp(rcf_rpc_server *rpcs, const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;
    te_string path = TE_STRING_INIT_STATIC(RCF_MAX_PATH);
    rpc_file_mode_flags flags = RPC_S_IRUSR | RPC_S_IWUSR | RPC_S_IXUSR |
                                RPC_S_IRGRP | RPC_S_IXGRP |
                                RPC_S_IROTH | RPC_S_IXOTH;

    va_start(ap, fmt);
    rc = te_string_append_va(&path, fmt, ap);
    va_end(ap);

    if (rc != 0)
        return rc;

    if (rpc_mkdirp(rpcs, path.ptr, flags) != 0)
    {
        return RPC_ERRNO(rpcs);
    }

    return rc;
}

static te_errno
ceph_rpc_mount(rcf_rpc_server *rpcs, const char *block, const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;
    te_string mount_cmd = TE_STRING_INIT_STATIC(RCF_MAX_PATH);
    rpc_wait_status status;

    va_start(ap, fmt);
    rc = te_string_append(&mount_cmd, "mount -o noatime,inode64 %s ", block);
    if (rc == 0)
    {
        rc = te_string_append_va(&mount_cmd, fmt, ap);
    }
    va_end(ap);

    if (rc != 0)
    {
        return rc;
    }

    status = rpc_system(rpcs, mount_cmd.ptr);
    if (status.value != 0)
    {
        return TE_EFAIL;
    }

    return rc;
}

te_errno
block_ts_ceph_cluster_mon_start(rcf_rpc_server *rpcs)
{
    RPC_AWAIT_ERROR(rpcs);

    CHECK_RC(ceph_rpc_mkdirp(rpcs, "%s/%s",
        BLOCK_TS_CEPH_MON_DATA, "ceph-main"));
    CHECK_RC(block_ts_ceph_cluster_mon_helper_start(rpcs, TRUE));
    CHECK_RC(block_ts_ceph_cluster_mon_helper_start(rpcs, FALSE));

    return 0;
}

static te_errno
block_ts_ceph_cluster_osd_helper_start(rcf_rpc_server *rpcs, te_bool mkfs)
{
    tsapi_ceph_osd_t osd = TSAPI_CEPH_OSD_T_INIT;

    osd.opts.common_opts.config = BLOCK_TS_CEPH_CONF;
    osd.opts.common_opts.id = BLOCK_TS_CEPH_OSD_FIRST;
    osd.opts.common_opts.monmap = BLOCK_TS_CEPH_MONMAP;
    osd.opts.common_opts.mkfs = mkfs;

    CHECK_RC(tsapi_ceph_osd_init(rpcs, &osd));
    CHECK_RC(block_ts_ceph_cluster_env_prepare(rpcs, &osd.common));
    CHECK_RC(tsapi_ceph_osd_start(&osd));
    CHECK_RC(tsapi_ceph_osd_wait(&osd));
    CHECK_RC(tsapi_ceph_osd_cleanup(&osd));

    return 0;
}

te_errno
block_ts_ceph_cluster_osd_start(rcf_rpc_server *rpcs)
{
    tsapi_ceph_t ceph = TSAPI_CEPH_T_INIT;
    tsapi_ceph_opts_osd_subcommand osd_subcommand = {
        .create = TRUE,
    };

    ceph.opts.common_opts.config = BLOCK_TS_CEPH_CONF;
    ceph.opts.common_opts.monmap = BLOCK_TS_CEPH_MONMAP;
    ceph.opts.osd_subcommand = &osd_subcommand;

    CHECK_RC(tsapi_ceph_init(rpcs, &ceph));
    CHECK_RC(block_ts_ceph_cluster_env_prepare(rpcs, &ceph.common));
    CHECK_RC(tsapi_ceph_start(&ceph));
    CHECK_RC(tsapi_ceph_wait(&ceph));
    CHECK_RC(tsapi_ceph_cleanup(&ceph));

    RPC_AWAIT_ERROR(rpcs);
    /* TODO: get rig from mknod + mkfs */
    rpc_system(rpcs, "mknod /dev/ram0 b 1 0");
    rpc_system(rpcs, "mkfs -t xfs fs-options -f -i size=2048 /dev/ram0");
    CHECK_RC(ceph_rpc_mkdirp(rpcs, "%s/ceph-%s",
        BLOCK_TS_CEPH_OSD_DATA, BLOCK_TS_CEPH_OSD_FIRST));
    CHECK_RC(ceph_rpc_mount(rpcs, "/dev/ram0", "%s/ceph-%s",
        BLOCK_TS_CEPH_OSD_DATA, BLOCK_TS_CEPH_OSD_FIRST));

    CHECK_RC(block_ts_ceph_cluster_osd_helper_start(rpcs, TRUE));

    /* Add OSD to crush map */
    rpc_system_ex(rpcs, "ceph osd crush add %s 1.0 root=default host=%s",
                  BLOCK_TS_CEPH_OSD_FIRST, BLOCK_TS_CEPH_OSD_HOST);

    CHECK_RC(block_ts_ceph_cluster_osd_helper_start(rpcs, FALSE));

    ceph = TSAPI_CEPH_T_INIT;
    ceph.opts.common_opts.config = BLOCK_TS_CEPH_CONF;
    ceph.opts.common_opts.monmap = BLOCK_TS_CEPH_MONMAP;
    ceph.opts.status = TRUE;

    CHECK_RC(tsapi_ceph_init(rpcs, &ceph));
    CHECK_RC(block_ts_ceph_cluster_env_prepare(rpcs, &ceph.common));
    CHECK_RC(tsapi_ceph_start(&ceph));
    CHECK_RC(tsapi_ceph_wait(&ceph));
    CHECK_RC(tsapi_ceph_cleanup(&ceph));

    return 0;
}

te_errno
block_ts_ceph_cluster_get_fsid(rcf_rpc_server *rpcs, te_string *fsid)
{
    tsapi_ceph_t ceph = TSAPI_CEPH_T_INIT;
    tsapi_ceph_opts_fsid_subcommand fsid_subcommand = {
        .get_fsid = TRUE,
    };
    te_errno rc;

    ceph.opts.fsid_subcommand = &fsid_subcommand;

    rc = tsapi_ceph_init(rpcs, &ceph);
    if (rc != 0)
    {
        ERROR("Failed to init ceph tool: %r", rc);
        goto out;
    }

    rc = tsapi_ceph_start(&ceph);
    if (rc != 0)
    {
        ERROR("Failed to start ceph tool: %r", rc);
        goto out;
    }

    rc = tsapi_ceph_wait(&ceph);
    if (rc != 0)
    {
        ERROR("Failed to wait ceph tool: %r", rc);
        goto out;
    }

    rc = tsapi_ceph_get_fsid_result(&ceph, fsid);
    if (rc != 0)
        ERROR("Failed to get report with ceph fsid: %r", rc);

out:
    rc = tsapi_ceph_cleanup(&ceph);
    if (rc != 0)
        ERROR("Failed to destroy ceph tool: %r", rc);

    return rc;
}

te_errno
block_ts_ceph_prepare_log_dir(rcf_rpc_server *rpcs)
{
    te_string ceph_log_dir = TE_STRING_INIT;
    te_errno rc;
    rpc_file_mode_flags mode_flags = RPC_S_IRWXU | RPC_S_IRGRP |
                                     RPC_S_IXGRP | RPC_S_IROTH |
                                     RPC_S_IXOTH;

    if ((rc = te_string_append(&ceph_log_dir, "%s/%s",
                               BLOCK_TS_CEPH_LOG_DIR_PATH,
                               BLOCK_TS_CEPH_FSID)) != 0)
    {
        te_string_free(&ceph_log_dir);
        return rc;
    }

    if (rpc_mkdirp(rpcs, ceph_log_dir.ptr, mode_flags) != 0)
    {
        ERROR("Could not create directory for ceph logs: rc=%r",
              RPC_ERRNO(rpcs));

        te_string_free(&ceph_log_dir);
        return RPC_ERRNO(rpcs);
    }

    return 0;
}

te_errno
block_ts_ceph_start_logger(rcf_rpc_server *rpcs)
{
    te_string       ceph_log_path = TE_STRING_INIT;
    te_string       fsid = TE_STRING_INIT;
    te_errno        rc = 0;
    int             i = 1;

    if ((rc = block_ts_ceph_cluster_get_fsid(rpcs, &fsid)) != 0)
    {
        ERROR("Failed to get CEPH fsid");
        goto out;
    }

    if ((rc = te_string_append(&ceph_log_path, "%s/%s/%s",
                               BLOCK_TS_CEPH_LOG_DIR_PATH, fsid.ptr,
                               BLOCK_TS_CEPH_LOG_FILE_NAME)) != 0)
    {
        ERROR("Failed to construct path to ceph logs");
        goto out;
    }

    if ((rc = tapi_cfg_ps_add(rpcs->ta, TSAPI_CEPH_LOGGER_NAME,
                              BLOCK_TS_CEPH_LOG_TOOL_NAME, FALSE)) != 0)
    {
        ERROR("Failed to add process: %s");
        goto out;
    }

#define ADD_LOGGER_ARG(_arg)                                                \
    do {                                                                    \
        if ((rc = tapi_cfg_ps_add_arg(rpcs->ta, TSAPI_CEPH_LOGGER_NAME,     \
                                      i++, _arg)) != 0)                     \
        {                                                                   \
            ERROR("Failed to add arg: %s", _arg);                           \
            goto out;                                                       \
        }                                                                   \
    } while(0)

    ADD_LOGGER_ARG("-F");
    ADD_LOGGER_ARG(ceph_log_path.ptr);

#undef ADD_LOGGER_ARG

    if ((rc = tapi_cfg_ps_start(rpcs->ta, TSAPI_CEPH_LOGGER_NAME)) != 0)
    {
        ERROR("Failed to start %s, error %r", TSAPI_CEPH_LOGGER_NAME, rc);
        return rc;
    }

out:
    te_string_free(&ceph_log_path);
    te_string_free(&fsid);

    return rc;
}

/* See description in block-ts.h */
te_errno
block_ts_ceph_enable_logger(void *data)
{
    te_errno rc;
    te_bool enable;
    rcf_rpc_server *rpcs = (rcf_rpc_server *)data;

    rc = tsapi_get_backend_param_bool("ceph", "enable_logging", &enable);
    if (rc != 0)
        return rc;

    if (enable)
        rc = block_ts_ceph_start_logger(rpcs);

    return rc;
}
