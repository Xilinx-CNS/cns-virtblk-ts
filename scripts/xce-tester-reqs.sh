#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

RESULT=

SSH_WITH_OPTS=( ssh -qxT -o BatchMode=yes -o StrictHostKeyChecking=no )

duthost="${TE_DUT_TST2}"

BLK_DEV_NUM="$(${SSH_WITH_OPTS[@]} ${duthost} lspci -d 1bf4: | wc -l)" 2>/dev/null
if [[ "$BLK_DEV_NUM" = "1" ]]; then
    RESULT+="--tester-req=!2BLK --tester-req=!MULTI_BLK "
elif [[ "$BLK_DEV_NUM" = "2" ]]; then
    RESULT+="--tester-req=!1BLK --tester-req=!MULTI_BLK "
else
    RESULT+="--tester-req=!1BLK --tester-req=!2BLK "
fi

if [[ "$BACKEND_MODE" = "malloc" ]]; then
    RESULT+="--tester-req=!BIG_DISK "
    RESULT+="--tester-req=!CEPH "
    RESULT+="--tester-req=!NULLBLOCK "
    RESULT+="--tester-req=!NVME "
    RESULT+="--tester-req=!REMOTE_BACKNED "
elif [[ "$BACKEND_MODE" = "nullblock" ]]; then
    RESULT+="--tester-req=!BIG_DISK "
    RESULT+="--tester-req=!MALLOC "
    RESULT+="--tester-req=!CEPH "
    RESULT+="--tester-req=!NVME "
    RESULT+="--tester-req=!REMOTE_BACKNED "
elif [[ "$BACKEND_MODE" = "nvme" ]]; then
    RESULT+="--tester-req=!MALLOC "
    RESULT+="--tester-req=!CEPH "
    RESULT+="--tester-req=!NULLBLOCK "
else
    RESULT+="--tester-req=!MALLOC "
    RESULT+="--tester-req=!NULLBLOCK "
    RESULT+="--tester-req=!NVME "
fi

if [[ "$TE_USE_ONLOAD" = "true" ]]; then
    RESULT+="--tester-req=!NO_ONLOAD "
else
    RESULT+="--tester-req=!ONLOAD "
fi

if [[ "$PERF_SHORT" = "true" ]]; then
    RESULT+="--tester-req=!PERF_FULL "
fi

stor_size_gb=""
if test -n "${TE_CEPH_STOR_SIZE_GB}"; then
    # 2 is the required CEPH service information
    stor_size_gb=$(( TE_CEPH_STOR_SIZE_GB - 2))
elif test -n "${TE_NVME_STOR_SIZE_GB}"; then
    stor_size_gb=${TE_NVME_STOR_SIZE_GB}
fi

if test "${stor_size_gb}" != ""; then
    size_gb=1
    stor_size_per_dev=$(( stor_size_gb / BLK_DEV_NUM ))

    for degree in {0..6}; do
        if test ${size_gb} -gt ${stor_size_per_dev}; then
            RESULT+="--tester-req=!MORE_THAN_${size_gb}GB "
        fi

        size_gb=$(( 2 * size_gb ))
    done
fi

echo "${RESULT}"
