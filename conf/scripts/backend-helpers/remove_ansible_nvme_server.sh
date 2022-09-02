#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

# Remove NVME SPDK server
systemctl stop nvme-server
journalctl -u nvme-server

sleep 1

# Unload brd module
modprobe -r brd
