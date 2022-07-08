#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
#
# Build dbench and copy the binary to TA insatllation directory
# in order for it to be copied to TA.

echo "Building dbench on $(hostname)"

try() {
  echo "$*"
  $* || exit 1
}

try cd "${EXT_SOURCES}"

try ./autogen.sh
try ./configure
try make -j$(nproc)

components=(
    dbench
    client.txt
)

for ta_type in ${TE_TA_TYPES} ; do
    try cp -t "${TE_AGENTS_INST}/${ta_type}" "${components[@]}"
done
