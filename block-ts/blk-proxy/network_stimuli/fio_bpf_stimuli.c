/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Network stimuli test
 *
 * @defgroup blk-proxy-fio_bpf_stimuli I/O under Network with an activated BPF stimulus
 * @grouporder{2}
 * @ingroup virtblk-blk-proxy-network-stimuli
 *
 * Run FIO tool and check its behaviour with various BPF network stimuli
 *
 * @param env            Network environment configuration:
 *                       - @c network
 * @param ioengine       FIO parameter: I/O Engine type
 * @param rwtype         FIO parameter: Read or write type
 * @param rwmixread      FIO parameter: Percentage of mixed workload that is
 *                                      reads
 * @param numjobs        FIO parameter: Duplicate this job many times
 * @param iodepth        FIO parameter: Number of IO buffers to keep in flight
 * @param blocksize      FIO parameter: Block size unit
 * @param runtime_sec    FIO parameter: Stop workload when the time expires
 * @param stimulus       Type of tested stimulus:
 *                       - @c drop - drop @p stimulus_param packets
 *                       - @c duplicate - duplicate one packet
 *                                        @p stimulus_param times
 *                       - @c delay - delay one packet for @p stimulus_param
 *                                    packets
 * @param stimulus_param Parameter value for @p stimulus
 *                       - @c 1
 *                       - @c 10
 *                       - @c 20
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */

#define TE_TEST_NAME "blk-proxy/network_stimuli/fio_bpf_stimuli"

#include "tsapi.h"
#include "tapi_bpf.h"
#include "tapi_bpf_stim.h"
#include "tsapi-fio.h"
#include "tapi_job_factory_rpc.h"

int
main(int argc, char *argv[])
{
    rcf_rpc_server *rpcs_disk;
    rcf_rpc_server *rpcs_host;
    const char *disk_ta = "Agt_disk";
    const char *host_ta = "Agt_host";
    const struct if_nameindex *srv_if = NULL;
    unsigned int stimulus;
    int stimulus_param;

    tapi_bpf_stim_hdl *handle = NULL;
    tapi_job_factory_t *fio_factory = NULL;
    tapi_fio *fio = NULL;
    tapi_fio_report report;
    tapi_fio_opts fio_opts = TAPI_FIO_OPTS_DEFAULTS;
    tapi_env env;

    TEST_START;
    TEST_START_ENV;
    TEST_GET_RPCS(host_ta, "pco", rpcs_host);
    TEST_GET_RPCS(disk_ta, "pco", rpcs_disk);
    TEST_GET_IF(srv_if);
    TEST_GET_CT_STIMULUS_PARAM(stimulus);
    TEST_GET_INT_PARAM(stimulus_param);

    TEST_STEP("Load and link to the interface a BPF program for @p stimulus");
    tapi_bpf_stim_init(rpcs_disk, srv_if->if_name,
                       stimulus, FALSE, &handle);

    switch (stimulus)
    {
        case TAPI_BPF_STIM_STIMULUS_DROP:
            TEST_SUBSTEP("Activate @c drop stimulus to drop the next"
                         " @p stimulus_param packets");
            CHECK_RC(tapi_bpf_stim_drop(handle, stimulus_param));
            break;

        case TAPI_BPF_STIM_STIMULUS_DUPLICATE:
            TEST_SUBSTEP("Activate @c duplicate stimulus to duplicate"
                         " the next packet @p stimulus_param times");
            CHECK_RC(tapi_bpf_stim_dup(handle, stimulus_param));
            break;

        case TAPI_BPF_STIM_STIMULUS_DELAY:
            TEST_SUBSTEP("Activate @c delay stimulus to wait the next"
                         " @p stimulus_param packets before sending"
                         " the delayed one");
            CHECK_RC(tapi_bpf_stim_delay(handle, stimulus_param));
            break;

        default:
            ERROR_VERDICT("Unknown stimulus");
    }

    TEST_STEP("Read fio options");
    tsapi_fio_opts_read(&fio_opts, argc, argv, rpcs_host->ta);

    TEST_STEP("Prepare FIO run");
    CHECK_RC(tapi_job_factory_rpc_create(rpcs_host, &fio_factory));
    CHECK_RC(tsapi_fio_create(&fio_opts, fio_factory, host_ta, &fio));

    TEST_STEP("Start FIO and wait for completion");
    CHECK_RC(tapi_fio_start(fio));
    CHECK_RC(tsapi_fio_flex_wait(fio));
    CHECK_RC(tapi_fio_stop(fio));

    TEST_STEP("Get FIO report and log it");
    CHECK_RC_VERDICT(tapi_fio_get_report(fio, &report),
                     "Failed to parse FIO output");
    tapi_fio_log_report(&report);

    TEST_SUCCESS;
cleanup:
    tapi_fio_destroy(fio);
    tapi_job_factory_destroy(fio_factory);
    tapi_bpf_stim_del(handle);
    TEST_END;
}
