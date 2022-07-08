#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
# Copy compilebench sources to TA installation directory
# in order for the to be copied to TA.

echo "Building compilebench on $(hostname)"

try() {
  echo "$*"
  $* || exit 1
}

for ta_type in ${TE_TA_TYPES} ; do
    try cp -rT "${EXT_SOURCES}" "${TE_AGENTS_INST}/${ta_type}"
done
