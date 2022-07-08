#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

function try() {
  $* || {
    echo "Failed: $*" >&2
    exit 1
  }
}

DOMAIN=$(hostname -d)

case $DOMAIN in
    xcelab*)
        ;;
    *)
        echo ""
        exit 0
        ;;
esac

SSH_WITH_OPTS=( ssh -qxT -o BatchMode=yes -o StrictHostKeyChecking=no )

XILINXTOOL_OPTS=()

test -d "$XILINXTOOL_SRC" ||
    { echo "XILINXTOOL_SRC=${XILINXTOOL_SRC} is not a directory" >&2 ; exit 1; }

XILINXTOOL=$XILINXTOOL_SRC/scripts/xilinxtool

test -n ${TE_JTAG_HOST} ||
    { echo "Host for starting the UART is not specified" >&2 ; exit 1; }

test -n ${TE_DUT_TST2} ||
    { echo "DUT host is not specified" >&2 ; exit 1; }

HOST="${TE_JTAG_HOST}"
duthost="${TE_DUT_TST2}"

XILINXTOOL_OPTS+=( --duthost ${duthost} --kill-uart-nmc --uart-cmd )

SSH=( "${SSH_WITH_OPTS[@]}" "${HOST}" )

function do_cmdclient() {
    local cmds="$1"

    "${SSH[@]}" ${XILINXTOOL} ${XILINXTOOL_OPTS[@]} \'${cmds}\' 2>/dev/null
}

OUT_VERSION="$(do_cmdclient version)"

declare -A VER_TAGS
VER_TAGS[fw]='MC Firmware version is'
VER_TAGS[fw-build]='Build name:'
VER_TAGS[fw-rev]='Extended info:'
VER_TAGS[suc]='SUC Firmware version is'
VER_TAGS[fpga]='FPGA version is'
VER_TAGS[fpga-extra]='FPGA extra:'
VER_TAGS[soc-rootfs]='SoC main rootfs version is'

declare TAG
declare VALUE
for TAG in "${!VER_TAGS[@]}" ; do
    VALUE="$(echo "${OUT_VERSION}" | grep "^${VER_TAGS[${TAG}]}" \
           | sed -e 's/\[//g' -e 's/\]//g' | sed -e 's/.* //')"
    test -z "${VALUE}" || TAGS="${TAGS} ${TAG}:${VALUE}"
done

for TAG in ${TAGS} ; do
    RESULT="${RESULT} --trc-tag=${TAG}"
done

BLK_DEV_NUM="$(${SSH_WITH_OPTS[@]} ${duthost} lspci -d 1bf4: | wc -l)" 2>/dev/null
RESULT="${RESULT} --trc-tag=blk_dev_num:${BLK_DEV_NUM}"

if [[ "$BACKEND_MODE" = "ceph" ]]; then
    RESULT="${RESULT} --trc-tag=ceph_osd_num:${TE_CEPH_OSD_NUM}"
    RESULT="${RESULT} --trc-tag=ceph_stor_size:${TE_CEPH_STOR_SIZE_GB}"
fi

echo "${RESULT}"
