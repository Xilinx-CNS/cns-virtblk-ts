#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

SSH_WITH_OPTS=( ssh -qxT -o BatchMode=yes -o StrictHostKeyChecking=no )

test -n ${TE_DUT_TST3} ||
    { echo "CEPH host is not specified" >&2 ; exit 1; }

CEPH_OSD_NUM="$(${SSH_WITH_OPTS[@]} ${TE_DUT_TST3} ceph osd stat \
              | grep -oE "^[0-9]*")" 2>/dev/null
export TE_CEPH_OSD_NUM=${CEPH_OSD_NUM}

CEPH_STOR_SIZE="$(${SSH_WITH_OPTS[@]} ${TE_DUT_TST3} ceph df \
                | grep -oE "TOTAL[[:blank:]]+[0-9]*" \
                | grep -o "[0-9]*")" 2>/dev/null
export TE_CEPH_STOR_SIZE_GB=${CEPH_STOR_SIZE}
