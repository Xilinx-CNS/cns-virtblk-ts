#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
#
# Build iozone and copy the binary to TA installation directory
# in order for it to be copied to TA.

echo "Building iozone on $(hostname)"

try() {
    echo "$*"
    $* || exit 1
}

try cd "${EXT_SOURCES}/src/current"

try make -j$(nproc) linux

components=(
    iozone
)

for ta_type in ${TE_TA_TYPES} ; do
    try cp -t "${TE_AGENTS_INST}/${ta_type}" "${components[@]}"
done
