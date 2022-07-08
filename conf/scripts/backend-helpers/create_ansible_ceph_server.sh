#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
#
# Create CEPH server in container using ansible
#

virtblk_ci_ansible_dir=$1
ceph_inventory_path=$2

pushd ${virtblk_ci_ansible_dir} >/dev/null
ansible-playbook playbooks/ceph-cluster.yml playbooks/ceph-cluster-storage.yml \
                 -i $ceph_inventory_path
