#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

# Author: Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
#
# Script for generating documentation
#
# - DOXYREST_PREFIX   - prefix path to doxyrest if you want to use sphinx to
#                       generate HTML
#

export TS_TOPDIR="$(cd "$(dirname "$(which "$0")")/.."; pwd -P)"
. ${TS_TOPDIR}/scripts/guess.sh

help() {
    cat <<EOF
Tool is used to generate documentation for the virtio-blk test suite.

It's based on doxygen and doxyrest.

Usage: ./$0 [--help]
EOF
}

for opt ; do
    case "${opt}" in
        --help|-h)
            help
            exit 0
            ;;
        *)
            help
            exit 1
            ;;
    esac
done

if [ -z "$DOXYREST_PREFIX" ]; then
    $TE_BASE/scripts/doxyrest_deploy.sh --how
    exit -1
fi
[ -z "$DOXYREST_BIN" ] && DOXYREST_BIN="$DOXYREST_PREFIX/bin/doxyrest"

[ -z "$SPHINX_BUILD_BIN" ] && SPHINX_BUILD_BIN="sphinx-build"
if ! which $SPHINX_BUILD_BIN >/dev/null; then
    echo "Please install sphinx before:"
    echo " $ pip install sphinx sphinx_rtd_theme"
    exit -2
fi

pushd ${TS_TOPDIR} >/dev/null

doxygen block-ts/doc/Doxyfile >/dev/null

DOXYREST_OPTS=(
    --config=block-ts/doc/doxyrest-config.lua
    --frame-dir=$DOXYREST_PREFIX/share/doxyrest/frame/cfamily
    --frame-dir=$DOXYREST_PREFIX/share/doxyrest/frame/common
)

${DOXYREST_BIN} "${DOXYREST_OPTS[@]}" || exit 1

SPHINX_BUILD_OPTS=(
    -j auto                     # use cpu count
    -q                          # be quite: warn and error only
    block-ts/doc                # input
    block-ts/doc/generated/html # output
)

${SPHINX_BUILD_BIN} "${SPHINX_BUILD_OPTS[@]}"
popd >/dev/null
