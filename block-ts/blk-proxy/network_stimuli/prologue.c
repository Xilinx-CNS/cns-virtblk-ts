/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Network stimuli prologue
 *
 * @defgroup blk-proxy-network_stimuli-prologue Network stimuli prologue
 * @grouporder{1}
 * @ingroup virtblk-blk-proxy-network-stimuli
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 *
 * Prepare hosts before the testing of Network stimuli package.
 */

#define TE_TEST_NAME "blk-proxy/network_stimuli/prologue"

#include "tsapi.h"
#include "conf_api.h"
#include <sys/resource.h>

int
main(int argc, char *argv[])
{
    const char     *disk_ta = "Agt_disk";
    cfg_val_type    val_t_int = CVT_INTEGER;
    unsigned int    memlock_cur;
    unsigned int    memlock_max;
    unsigned int    memloc_inf = (unsigned int)RLIM_INFINITY;

    TEST_START;

    TEST_STEP("Increase @b RLIMIT_MEMLOCK up to @c RLIM_INFINITY");
    CHECK_RC(cfg_get_instance_fmt(&val_t_int, &memlock_cur,
                        "/agent:%s/rlimits:/memlock:/cur:", disk_ta));
    CHECK_RC(cfg_get_instance_fmt(&val_t_int, &memlock_max,
                        "/agent:%s/rlimits:/memlock:/max:", disk_ta));

    if (memlock_cur != memloc_inf || memlock_max != memloc_inf)
    {
        RING("Current rlimits/memlock values: cur=%u, max=%u", memlock_cur,
             memlock_max);
        CHECK_RC(cfg_set_instance_fmt(val_t_int, &memloc_inf,
                        "/agent:%s/rlimits:/memlock:/max:", disk_ta));
        CHECK_RC(cfg_set_instance_fmt(val_t_int, &memloc_inf,
                        "/agent:%s/rlimits:/memlock:/cur:", disk_ta));
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
