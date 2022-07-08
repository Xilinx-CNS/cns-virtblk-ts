#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
#
# Build fio and copy the binary to TA insatllation directory
# in order for it to be copied to TA.

echo "Building fio on $(hostname)"

try() {
  echo "$*"
  $* || exit 1
}

try cd "${EXT_SOURCES}"

try ./configure
try make -j$(nproc)

components=(
    fio
)

for ta_type in ${TE_TA_TYPES} ; do
    try cp -t "${TE_AGENTS_INST}/${ta_type}" "${components[@]}"
done
