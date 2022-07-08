#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

echo "Building on $(hostname)"

try() {
  $* || {
    echo "Failed: $*" >&2
    exit 1
  }
}

try cd "${EXT_SOURCES}"

# FIXME it should be a parameter via environment
RTE_KERNELDIR=${1:-''}

test -n "$RTE_KERNELDIR" && export RTE_KERNELDIR

build_opts=(
    --target-arch=armv8-a+crc
    --disable-tests
    --without-isal
    --without-crypto
    --without-fio
    --without-vhost
    --without-virtio
    --without-reduce
    --without-rdma
    --without-fc
    --without-iscsi-initiator
    --without-vtune
    --without-ocf
    --without-uring
    --without-fuse
    --without-nvme-cuse
)

if test "${TE_OLD_SPDK}" = "true" ; then
    build_opts+=(
        --without-vpp
        --without-igb-uio-driver
    )
else
    build_opts+=(
        --disable-examples
        --disable-unit-tests
    )
fi

test -n "${TE_CEPH_EXEC}" && build_opts+=" --with-rbd"
test -n "${TE_USE_ONLOAD}" && build_opts+=" --enable-zero-copy"

test -f mk/config.mk || {
    try "./configure ${build_opts[@]}"
}

try make -j$(nproc)

components=()

if test "${TE_OLD_SPDK}" = "true" ; then
    components+=(
        app/blk_proxy/blk_proxy
    )
else
    components+=(
        build/bin/blk_proxy
        scripts/config_converter.py
    )
fi

for ta_type in ${TE_TA_TYPES} ; do
  try cp -t "${TE_AGENTS_INST}/${ta_type}" ${components[@]}
done
