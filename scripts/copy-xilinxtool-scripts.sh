#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

try() {
  $* || {
    echo "Failed: $*" >&2
    exit 1
  }
}

echo ${EXT_SOURCES} >&2

for ta_type in ${TE_TA_TYPES} ; do
  mkdir -p "${TE_AGENTS_INST}/${ta_type}/snapper/"
  try cp -r "${EXT_SOURCES}/*" "${TE_AGENTS_INST}/${ta_type}/snapper/"
done
