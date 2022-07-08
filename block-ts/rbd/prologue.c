/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Test Suite prologue
 *
 * @defgroup rbd-prologue Prologue for RBD tests
 * @ingroup virtblk-rbd
 * @{
 *
 * Block Test Suite
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

/**
 * @objective Perform setup of test environment for all tests
 */

#define TE_TEST_NAME    "prologue"

#include "block-ts.h"

static void
update_agents_gtest_path(void)
{
    cfg_handle   *handles = NULL;
    unsigned int  num;
    unsigned      i;
    const char *path = getenv("CEPH_BIN");

    if (path == NULL)
    {
        ERROR_ARTIFACT("CEPH_BIN not exist");
    }

    CHECK_RC(cfg_find_pattern("/agent:*", &num, &handles));
    for (i = 0; i < num; i++)
    {
        cfg_oid *oid = NULL;

        CHECK_RC(cfg_get_oid(handles[i], &oid));
        CHECK_RC(tapi_sh_env_ta_path_append(CFG_OID_GET_INST_NAME(oid, 1), path));
    }
}

int
main(int argc, char *argv[])
{
    TEST_START;

    TEST_STEP("Prepare environment for start rbd tests");
    update_agents_gtest_path();
    CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, "/:"));

    TEST_SUCCESS;
cleanup:
    TEST_END;
}

/**@} <!-- END rbd-prologue --> */
