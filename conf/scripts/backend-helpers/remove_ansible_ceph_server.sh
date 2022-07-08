#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

# Remove CEPH cluster
fsid="$(ceph fsid)"
cephadm rm-cluster --fsid ${fsid} --force

# Stop podman container with CEPH server
podman stop -a

# Remove logical volumes
lvremove -f $(lvdisplay | grep -oE "/dev/vg[0-9]*/lv-ceph[0-9]*")

# Remove volume groups
vgremove -f $(vgs | grep -oE "vg[0-9]*")

# Remove physical volumes
pvremove -f $(pvs | grep -oE "/dev/ram[0-9]*")

# Unload brd module
modprobe -r brd
