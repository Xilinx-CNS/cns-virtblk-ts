/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Rebind a driver on a PCI device
 *
 * @defgroup blk-proxy-config-driver-rebind Block driver rebinding
 * @ingroup virtblk-blk-proxy-config
 *
 * Rebind a driver on a PCI device
 *
 * @param vendor_id    Vendor's PCI id
 * @param device_id    Devices's PCI id
 * @param rebind_count The number of times to rebind a driver
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

/**
 * @objective Rebind a driver on a PCI device
 */

#define TE_TEST_NAME  "blk-proxy/config/driver_rebind"

/*
 * FIXME: if signal.h is not included, including tapi_test.h produces errors:
 * undeclared SIGINT, though it is included in tapi_test.h under HAVE_SIGNAL_H
 */
#include <signal.h>

#include "tsapi.h"
#include "tapi_test.h"

int
main(int argc, char *argv[])
{
    unsigned int rebind_count;
    const char *ta = "Agt_host";
    const char *vendor_id;
    const char *device_id;
    unsigned int i;
    unsigned int j;
    unsigned int block_dev_count;
    char *driver = NULL;

    TEST_START;
    TEST_GET_STRING_PARAM(vendor_id);
    TEST_GET_STRING_PARAM(device_id);
    TEST_GET_UINT_PARAM(rebind_count);

    if (rebind_count == 0)
        TEST_FAIL("Rebind count 0 does not make sense");

    block_dev_count = tsapi_virtio_blocks_count(ta);

    TEST_STEP("Rebind driver for each block device");
    for (j = 0; j < block_dev_count; j++)
    {
        TEST_STEP("Get current driver of a device");
        CHECK_RC(tapi_cfg_pci_get_driver_by_vend_dev_inst(ta, vendor_id,
                                                          device_id, j,
                                                          &driver));
        if (*driver == '\0')
            TEST_FAIL("Could not get current driver of a device");

        TEST_STEP("Perform driver unbind and bind @p rebind_count times");
        for (i = 0; i < rebind_count; i++)
        {
            TEST_SUBSTEP("Unbind the driver");
            CHECK_RC(tapi_cfg_pci_unbind_driver_by_vend_dev_inst(ta, vendor_id,
                                                                 device_id,
                                                                 j));

            TEST_SUBSTEP("Rebind the driver");
            CHECK_RC(tapi_cfg_pci_bind_driver_by_vend_dev_inst(ta, vendor_id,
                                                               device_id, j,
                                                               driver));

            /*
            * VIRTBLK-1171: blk proxy does not support bind/unbind if
            * there is any inflight data.
            */
            te_motivated_sleep(5, "Waiting to successfully bind");
        }

        free(driver);
        driver = NULL;
    }

    TEST_SUCCESS;
cleanup:
    if (driver != NULL)
        free(driver);
    TEST_END;
}
