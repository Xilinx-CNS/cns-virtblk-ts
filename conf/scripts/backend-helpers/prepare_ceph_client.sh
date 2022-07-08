#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
#
# Copy config and keyring files to CEPH client.
#

client_ip=$1
use_onload=$2
use_plugin=$3

ssh -qxT -o BatchMode=yes -o StrictHostKeyChecking=no ${client_ip} 'mkdir -p /etc/ceph'
scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null /etc/ceph/{ceph.conf,ceph.client.admin.keyring} root@${client_ip=}:/etc/ceph/

# Add all necessary entries to the CEPH config for the plugin to work correctly
if [ "${use_onload}" = "true" ]; then
    IFS='' read -r -d '' CEPH_OPTIONS <<"EOF"
        ms type = async+onload
        ms async onload buffer pool size = 2M
        ms async explicit poll = true
        ms nocrc = true
        ms crc data = false
        rbd cache = false
EOF

    if [ "${use_plugin}" = "true" ]; then
        CEPH_OPTIONS+="	ms async onload rx plugin = true"
    fi

    echo "${CEPH_OPTIONS}" | ssh -qxT -o BatchMode=yes -o StrictHostKeyChecking=no ${client_ip} 'cat >> /etc/ceph/ceph.conf'
fi
