#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

echo "Copy backend-helpers to the $(hostname)"

try() {
  echo "$*"
  $* || exit 1
}

try cd "${EXT_SOURCES}"

components=(
    create_ansible_ceph_server.sh
    remove_ansible_ceph_server.sh
    create_ansible_nvme_server.sh
    remove_ansible_nvme_server.sh
    prepare_ceph_client.sh
)

for ta_type in ${TE_TA_TYPES} ; do
    try mkdir -p ${TE_AGENTS_INST}/${ta_type}/backend-helpers
    try cp -t "${TE_AGENTS_INST}/${ta_type}/backend-helpers/" "${components[@]}"
done
