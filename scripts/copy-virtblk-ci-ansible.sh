#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

echo "Copy ansible directory from virtblk-ci to the $(hostname)"

try() {
  echo "$*"
  $* || exit 1
}

try cd "${EXT_SOURCES}"

components=(
    playbooks/ceph-cluster.yml
    playbooks/ceph-cluster-storage.yml
    playbooks/nvme-server.yml
    roles
    ansible.cfg
)

# Copy ansible scripts to the tmp directory of the agent to
# be sure that all temporary files created under sudo will
# be removed.
for ta_type in ${TE_TA_TYPES} ; do
    try mkdir -p "${TE_AGENTS_INST}/${ta_type}/tmp/virtblk-ci-ansible"

    for f in ${components[@]} ; do
        try mkdir -p "${TE_AGENTS_INST}/${ta_type}/tmp/virtblk-ci-ansible/$(dirname $f)"
        try cp -r "$f" "${TE_AGENTS_INST}/${ta_type}/tmp/virtblk-ci-ansible/$f"
    done
done
