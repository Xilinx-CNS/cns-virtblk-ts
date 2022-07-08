#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

export TE_TS_DIR=$TS_TOPDIR/block-ts

function guess_dir() {
    local var_name=$1
    local dir=$2
    local base_dirs_to_check=(
        "$TS_TOPDIR"
        "$TS_TOPDIR"/..
        "$TS_TOPDIR"/../..
    )

    if [[ -n "${!var_name}" ]]; then
        return
    fi

    for base_dir in "${base_dirs_to_check[@]}"; do
        local dir_to_check=$base_dir/$dir
        if [[ -d ${dir_to_check} ]]; then
            eval 'export ${var_name}=${dir_to_check}'
            echo "Guessed ${var_name}=${dir_to_check}"
            break
        fi
    done
}

function run_fail() {
    local reason=$1
    echo $reason
    exit 1
}

function guess_pre_opts() {
    guess_dir TE_BASE te
    guess_dir TE_BASE test-environment
    guess_dir SF_TS_CONFDIR ts-conf
    guess_dir SF_TS_CONFDIR oktet-ts-conf

    test -d $TE_BASE || run_fail "TE_BASE should be defined"
    test -d $SF_TS_CONFDIR || run_fail "SF_TS_CONFDIR should be defined"

    export TE_SSH_KEY_DIR=${SF_TS_CONFDIR}/keys

    if [[ -z $TE_BUILD ]]; then
        export TE_BUILD=$TS_TOPDIR/../te-build
        echo "Guessed TE_BUILD=$TE_BUILD"
    fi
}

function guess_post_opts() {
    guess_dir XFS_TS_DIR xfstests-dev
    guess_dir SPDK_SRC spdk
    guess_dir COMPILEBENCH_SRC compilebench-0.6
    guess_dir DBENCH_SRC dbench-4.0
    guess_dir FIO_SRC fio
    guess_dir FSMARK_SRC fs_mark
    # We use this version because it is used in Phoronix Test Suite and it
    # builds fine unlike 3_491 which is currently 'stable'
    guess_dir IOZONE_SRC iozone3_465
    guess_dir CMOD_SRC_TOP smartnic_cmodel/..
    guess_dir CMOD_SRC_TOP smartnic-cmodel/..
    guess_dir VIRTBLK_CI_PATH virtblk-ci
    guess_dir SNAPPER_SRC smartnic-snapper
}

guess_pre_opts
