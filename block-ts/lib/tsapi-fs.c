/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper to work with fs test suite
 *
 * Helper to work with fs test suite.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#include "tsapi-fs.h"
#include "tsapi-mke2fs.h"
#include "tapi_test_log.h"
#include "tapi_rpc_stdio.h"
#include "tapi_job_factory_rpc.h"

/* Time to wait for checking filesystem */
#define FS_TS_CHECK_FS_TIMEOUT 30

#define FS_TS_STEP(_step, _fs...)  \
    do {                           \
        if (_step)                 \
            TEST_STEP(_fs);        \
        else                       \
            TEST_SUBSTEP(_fs);     \
    } while (0)

te_errno
fs_ts_create_filesystem(rcf_rpc_server *rpcs, const char *device, te_bool step)
{
    te_errno             rc;
    tapi_job_factory_t  *factory = NULL;
    tapi_mke2fs_app     *app = NULL;
    tapi_mke2fs_opt      opt = tapi_mke2fs_default_opt;

    opt.device = device;
    opt.fs_type = "ext4";
    FS_TS_STEP(step, "Create filesystem of type %s on %s",
               opt.fs_type, opt.device);
    rc = tapi_job_factory_rpc_create(rpcs, &factory);
    if (rc != 0)
    {
        ERROR("Failed to create job factory, error: %r", rc);
        return rc;
    }
    rc = tapi_mke2fs_do(factory, &opt, &app, TE_SEC2MS(120));
    if (rc != 0)
    {
        ERROR("mke2fs failed to create filesystem, error: %r", rc);
        goto out;
    }

    FS_TS_STEP(step, "Check the filesystem");
    rc = mke2fs_ts_check_fs(factory, app, opt.fs_type, opt.device,
                            TE_SEC2MS(FS_TS_CHECK_FS_TIMEOUT));

out:
    if (tapi_mke2fs_destroy(app) != 0)
        WARN("Failed to destroy mke2fs app");
    tapi_job_factory_destroy(factory);

    return rc;
}

te_errno
fs_ts_mount_filesystem(rcf_rpc_server *rpcs, const char *device,
                       const char *mount_dir, te_bool step)
{
    rpc_wait_status st;

    FS_TS_STEP(step, "Mount %s at %s", device, mount_dir);
    RPC_AWAIT_ERROR(rpcs);
    st = rpc_system_ex(rpcs, "mount %s %s", device, mount_dir);
    if (st.flag != RPC_WAIT_STATUS_EXITED || st.value != 0)
    {
        ERROR("mount command %s, error: %u",
              wait_status_flag_rpc2str(st.flag), st.value);

        return TE_RC(TE_RPC, TE_ESHCMD);
    }

    return 0;
}

te_errno
fs_ts_umount_filesystem(rcf_rpc_server *rpcs, const char *mount_dir,
                        te_bool step)
{
    rpc_wait_status st;

    FS_TS_STEP(step, "Umount %s", mount_dir);
    RPC_AWAIT_ERROR(rpcs);
    st = rpc_system_ex(rpcs, "umount %s", mount_dir);
    if (st.flag != RPC_WAIT_STATUS_EXITED || st.value != 0)
    {
        ERROR("umount command %s, error: %u",
              wait_status_flag_rpc2str(st.flag), st.value);

        return TE_RC(TE_RPC, TE_ESHCMD);
    }

    return 0;
}

te_errno
fs_ts_recreate_filesystem(rcf_rpc_server *rpcs, const char *device,
                          const char *mount_dir, te_bool step)
{
    te_errno rc;

    rc = fs_ts_umount_filesystem(rpcs, mount_dir, step);
    if (rc == 0)
        rc = fs_ts_create_filesystem(rpcs, device, step);
    if (rc == 0)
        rc = fs_ts_mount_filesystem(rpcs, device, mount_dir, step);

    return rc;
}
