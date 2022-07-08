#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

VIRTIO_BLK="${1:-$(pwd)/virtio-blk}"
VIRTIO_BLK_SIZE="${VIRTIO_BLK_SIZE:-10G}"
SHARED_PATH="${2:-/local}"

if ! test -e $VIRTIO_BLK; then
       qemu-img create -f qcow2 ${VIRTIO_BLK} ${VIRTIO_BLK_SIZE}
fi

fesnap --                                                       \
       --qemu-mh --enable-kvm --mh_screen_program screen        \
       --instance arm  --qemu-hostnet-fwd tcp::50002-:50002     \
       --instance arm  --qemu-hostnet-sshd 24222                \
       --instance host --qemu-hostnet-fwd tcp::50001-:50001     \
       --instance host --qemu-hostnet-sshd 24122                \
       --instance arm  --virtio-blk ${VIRTIO_BLK}               \
       --instance both --nosim                                  \
       --passthrough $SHARED_PATH                               \
       --distro Ubuntu18 \
       --rootiso /tools/solarflare/firmware-tools/cosim/rootiso/1.3-UBUNTU18.04.cosim.qonq.2 --init
