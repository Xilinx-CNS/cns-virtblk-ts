#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
#
# Author: Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
#
# Script for building onload for virtblk-projet

export BPATH=$1
export OUTPUT=$2

if [ -z "$BPATH" ]
then
	echo "Provide onload path, please."
	exit
fi

if [ -z "$OUTPUT" ]
then
	echo "Provide output path, please."
	exit
fi

export PATH=$PATH:/bin:$BPATH/scripts

echo "Building at $BPATH..."
cd $BPATH

mmakebuildtree --gnu
make -C build/gnu_x86_64 -j$(nproc)

echo "Copy to $OUTPUT..."

mkdir -p $OUTPUT/include $OUTPUT/lib

cp -Prv build/gnu_x86_64/lib/onload_ext/libonload_ext.* $OUTPUT/lib
cp -r src/include/onload $OUTPUT/include
