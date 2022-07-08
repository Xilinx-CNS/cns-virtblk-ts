#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
# Copy bpf programs sources to TA installation directory
# in order for the to be copied to TA and built.

echo "Building bpf programm on $(hostname)"

try() {
  echo "$*"
  $* || exit 1
}

# To build bpf programs clang is needed
export PATH=$PATH:/wrk/xce-tools/rhel8.2/bin

try cd "${EXT_SOURCES}"
try make -j$(nproc)

for ta_type in ${TE_TA_TYPES} ; do
    try cp -rT "${EXT_SOURCES}" "${TE_AGENTS_INST}/${ta_type}"
done
