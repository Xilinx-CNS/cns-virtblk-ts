#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
#
# Create NVME server using ansible
#

virtblk_ci_ansible_dir=$1
nvme_inventory_path=$2

pushd ${virtblk_ci_ansible_dir} >/dev/null
ansible-playbook playbooks/nvme-server.yml -i $nvme_inventory_path
