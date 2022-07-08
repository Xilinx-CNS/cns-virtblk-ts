#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

# Author: Mikhail Skorzhinskii <rasmi@oktetlabs.ru>
#
# Script for building ceph for virtblk-projet

export BPATH=$1
export BTYPE="$2"
export ONLOAD_BUILD=$3

if [ -z "$BPATH" ]
then
	echo "Provide build path, please."
	exit
fi

export PATH=$PATH:/bin

echo "Building at $BPATH..."
cd $BPATH

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ONLOAD_BUILD/lib
WITH_ONLOAD=OFF

if [ "$ONLOAD_BUILD" != "" ]; then
        WITH_ONLOAD=ON
fi

if [ "$BTYPE" == "--build-all" ]; then
        ./do_cmake.sh \
                -DWITH_ONLOAD=$WITH_ONLOAD \
                -DCMAKE_CXX_FLAGS="-I${ONLOAD_BUILD}/include -L${ONLOAD_BUILD}/lib"
else
        ./do_cmake.sh \
                -DWITH_RDMA=OFF -DWITH_OPENLDAP=OFF -DWITH_GSSAPI=OFF -DWITH_FUSE=OFF \
                -DWITH_XFS=OFF -DWITH_ZFS=OFF -DWITH_BLUESTORE=OFF -DWITH_SPDK=OFF \
                -DWITH_PMEM=OFF -DWITH_BLUEFS=OFF -DWITH_QATZIP=OFF \
                -DWITH_LIBCEPHFS=OFF -DWITH_KVS=OFF -DWITH_KRBD=OFF \
                -DWITH_LEVELDB=OFF -DWITH_BROTLI=OFF -DWITH_LZ4=OFF \
                -DWITH_CEPH_DEBUG_MUTEX=OFF -DWITH_XIO=OFF -DWITH_DPDK=OFF \
                -DWITH_BLKIN=OFF -DWITH_RADOSGW=OFF -DWITH_CEPHFS=OFF -DWITH_MGR=OFF \
                -DWITH_EVENTTRACE=OFF -DWITH_LTTNG=OFF -DWITH_BABELTRACE=OFF \
                -DWITH_TESTS=OFF -DWITH_FIO=OFF -DWITH_SYSTEM_FIO=OFF \
                -DWITH_SYSTEM_ROCKSDB=OFF -DWITH_SEASTAR=OFF \
                -DWITH_MGR_DASHBOARD_FRONTEND=OFF -DWITH_SYSTEM_NPM=OFF \
                -DWITH_SYSTEMD=OFF -DWITH_GRAFANA=OFF -DWITH_CEPHFS_JAVA=OFF \
                -DWITH_CEPHFS_SHELL=OFF -DWITH_TOOLS=OFF -DWITH_OSD=OFF \
                -DWITH_MON=OFF -DWITH_KV=OFF -DWITH_OS=OFF -DWITH_PYTHON=OFF \
                -DWITH_ONLOAD=$WITH_ONLOAD \
                -DCMAKE_CXX_FLAGS="-I${ONLOAD_BUILD}/include -L${ONLOAD_BUILD}/lib"
fi

cd build

make -j10
