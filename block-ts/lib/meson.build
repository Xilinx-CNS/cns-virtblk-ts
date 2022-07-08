# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
sources = [
    'block-ts.c',
    'tsapi-ceph.c',
    'tsapi-fio.c',
    'tsapi-fs.c',
    'tsapi.c',
    'tsapi-mke2fs.c',
    'tsapi-xfs.c',
    'tsapi-plugin.c',
    'tsapi-conf.c',
    'tsapi-blkproxy.c',
    'tsapi-inventory.c',
    'tsapi-backend.c',
    'tsapi-nvme.c'
]

includes += include_directories('tool')
subdir('tool')

tslib = static_library('block-ts', sources,
                       include_directories: includes,
                       dependencies: test_deps)
