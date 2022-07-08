#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

echo "Building xfstests on $(hostname)"

try() {
  echo "$*"
  $* || exit 1
}

try cd "${EXT_SOURCES}"

try make -j$(nproc)

components=(
    check
    common
    libtool
    ltp
    include
    src
    tests
)

for ta_type in ${TE_TA_TYPES} ; do
    try cp -rt "${TE_AGENTS_INST}/${ta_type}" "${components[@]}"
done
