#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

# Remove NVME server
nvmetcli clear

# Unload brd module
modprobe -r brd
