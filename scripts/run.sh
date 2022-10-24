#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

check_is_run_mode_set()
{
    if [[ $is_run_mode_set = true ]] ; then
        echo "Only one run-mode can be used." >&2
        exit 1
    fi

    is_run_mode_set=true
}

check_is_ceph_mode_set()
{
    if [[ $is_ceph_mode_set = true ]] ; then
        echo "Only one ceph-mode can be used." >&2
        exit 1
    fi

    is_ceph_mode_set=true
    ceph_mode=${1}
}

export TS_TOPDIR="$(cd "$(dirname "$(which "$0")")"; pwd -P)"
if [ "$(basename ${TS_TOPDIR})" == "scripts" ]; then
    export TS_TOPDIR=$(dirname ${TS_TOPDIR})
fi

source $TS_TOPDIR/scripts/guess.sh
source $TS_TOPDIR/scripts/trc-tags-helper

. ${TS_TOPDIR}/conf/scripts/lib.run

usage() {
cat <<EOF
USAGE: run.sh [run.sh options] [dispatcher.sh options]
Options:
  --cfg=<CFG>                       Configuration to be used
  --with-broken                     Don't skip tests with the BROKEN trc tag
  -n                                Don't build Test Engine, Test Agent and Test suite

  --cosim-log-tlps                  Turn on TLP logs
  --no-cosim-log-tlps               Turn of TLP logs (by default)
  --mc-fw-eftest                    DUT is running EFTEST FW
  --mc-fw-flash=<PATH>              Path to flash image
  --mc-fw-blk                       Use flash with 1 block device (by default)
  --mc-fw-2blk                      Use flash with 2 block devices
  --mc-fw-3blk                      Use flash with 3 block devices
  --mc-fw-pciid=<PCIID>             Vendor and device ID (and optionally subsystem vid/did) of
                                    functions, format:
                                    xxxx:xxxx[/yyyy:yyyy][,xxxx:xxxx[/yyyy:yyyy]]

  --virtio-modern                   Use modern virtio interface
  --virtio-transitional             Use legacy virtio interface

  --cmod-multihost                  Run CMOD in multi-host mode
  --cmod-singlehost                 Run CMOD in single-host mode
  --nullblock                       Run with nullblock devices

  --nvme                            Run with NVME server
  --nvme-connect                    Run and connect to existing NVME server

  --ceph-qemu                       Run with CEPH in qemu
  --ceph-container                  Run with CEPH in container
  --ceph-connect                    Run with already running CEPH

  --ceph-osd-num=<NUM>              Number of CEPH OSDs to create
                                    (only with --ceph-container)
  --ceph-stor-size=<SIZE-IN-GB>     Size of CEPH storage in GB
                                    (only with --ceph-container)
  --onload                          Run blk_proxy under Onload
  --enable-plugin                   Load and enable plugin on FPGA

  --arm-mtu=<MTU>                   MTU to set for SFC interface on ARM

  --ceph-build=<PATH>               Path to CEPH build
  --cmod-top=<PATH>                 Path to top CMOD directory
  --spdk-dir=<PATH>                 Path to SPDK source code
  --compilebench-dir=<PATH>         Path to compilebench source code
  --dbench-dir=<PATH>               Path to dbench source code
  --xfs-ts-dir=<PATH>               Path to xfstests-dev source code
  --iozone-dir=<PATH>               Path to iozone source code
  --fsmark-dir=<PATH>               Path to fsmark source code
  --fio-dir=<PATH>                  Path to fio source code

  --app-nobuild                     Skip build of all applications
  --spdk-nobuild                    Skip SPDK build
  --compilebench-nobuild            Skip compilebench build
  --dbench-nobuild                  Skip dbench build
  --xfs-nobuild                     Skip xfstests build
  --fsmark-nobuild                  Skip fsmark build
  --iozone-nobuild                  Skip iozone build
  --fio-nobuild                     Skip fio build

  --spdk-build                      Do SPDK build
  --compilebench-build              Do compilebench build
  --dbench-build                    Do dbench build
  --xfs-build                       Do xfstests build
  --fsmark-build                    Do fsmark build
  --iozone-build                    Do iozone build
  --fio-build                       Do fio build

  --old-spdk                        Run with a SPDK version before 21.07

  --perf-short                      Run the most important performance tests

  --sigusr2-cold-reboot             Make cold reboot for x86 host when
                                    SIGUSR2 is received

  Block proxy:

  --bp-logflags=<flag1,flag2>       Add logflags to blk_proxy
                                    For example:
                                    --bp-logflags=bp_datapath,bdev
  --ool-param NAME=value            Add Onload environment parameter


EOF
${TE_BASE}/dispatcher.sh --help
}

# Default config
export MC_FW_FLASH_DIR="p7d/cmod_dev"
export MC_FW_FLASH_NAME="${MC_FW_FLASH_DIR}/cmod_ef100p0_ef100p0_vnetp0_vblk.mcs"
export MC_FW_PCIID_TYPE=transitional
export COSIM_TLP_LOG0="/dev/null"
export COSIM_TLP_LOG1="/dev/null"

export TS_BLK_PROXY_CONF=${TS_TOPDIR}/conf/blk_proxy_conf

export TE_ENGINE_HOST=$(hostname)

do_compilebench_build=true
do_dbench_build=true
do_fsmark_build=true
do_iozone_build=true
do_spdk_build=true
do_xfs_build=true
do_fio_build=true

is_run_mode_set=false
ceph_mode=none

# Default number of OSDs
export TE_CEPH_OSD_NUM=3

export BACKEND_MODE=malloc
export TE_BACKEND_SRV_TYPE="none"

CFG=
RUN_OPTS=
while (( "$#" )); do
    ts_trc_tags_opt "$1"
    case "$1" in
        --help)
            usage
            exit 0
            ;;
        --cfg=*)
            RUN_OPTS+="--opts=run/${1#--cfg=} "
            CFG=${1#--cfg=}
            ;;
        --ceph-build=*)
            export CEPH_BUILD="${1#--ceph-build=}";;
        --with-broken)
            WITH_BROKEN="true";;
        --cmod-top=*)
            export CMOD_SRC_TOP="${1#--cmod-top=}";;
        --mc-fw-eftest)
            export RHSIM_FW_EFTEST="true";;
        --compilebench-dir=*)
            export COMPILEBENCH_SRC=${1#--compilebench-dir=};;
        --dbench-dir=*)
            export DBENCH_SRC=${1#--dbench-dir=};;
        --fsmark-dir=*)
            export FSMARK_SRC=${1#--fsmark-dir=};;
        --iozone-dir=*)
            export IOZONE_SRC=${1#--iozone-dir=};;
        --fio-dir=*)
            export FIO_SRC=${1#--fio-dir=};;
        --spdk-dir=*)
            export SPDK_SRC=${1#--spdk-dir=};;
        --app-nobuild)
            echo "Skiping build of all applications"
            do_compilebench_build=false
            do_dbench_build=false
            do_fsmark_build=false
            do_iozone_build=false
            do_spdk_build=false
            do_xfs_build=false
            do_fio_build=false
            ;;
        --compilebench-nobuild)
            echo "Skipping Compilebench build"
            do_compilebench_build=false
            ;;
        --dbench-nobuild)
            echo "Skipping Dbench build"
            do_dbench_build=false
            ;;
        --fsmark-nobuild)
            echo "Skipping FSMark build"
            do_fsmark_build=false
            ;;
        --iozone-nobuild)
            echo "Skipping IOzone build"
            do_iozone_build=false
            ;;
        --spdk-nobuild)
            echo "Skipping SPDK build"
            do_spdk_build=false
            ;;
        --xfs-nobuild)
            echo "Skipping XFS build"
            do_xfs_build=false
            ;;
        --fio-nobuild)
            echo "Skipping FIO build"
            do_fio_build=false
            ;;

        --compilebench-build)
            do_compilebench_build=true
            ;;
        --dbench-build)
            do_dbench_build=true
            ;;
        --fsmark-build)
            do_fsmark_build=true
            ;;
        --iozone-build)
            do_iozone_build=true
            ;;
        --spdk-build)
            do_spdk_build=true
            ;;
        --xfs-build)
            do_xfs_build=true
            ;;
        --fio-build)
            do_fio_build=true
            ;;

        --xfs-ts-dir=*)
            export XFS_TS_DIR=${1#--xfs-ts-dir=};;
        --cosim-log-tlps)
            unset COSIM_TLP_LOG0
            unset COSIM_TLP_LOG1
            ;;
        --no-cosim-log-tlps)
            export COSIM_TLP_LOG0="/dev/null"
            export COSIM_TLP_LOG1="/dev/null"
            ;;
        --mc-fw-flash=*)
            export MC_FW_FLASH_NAME="${1#--mc-fw-flash=}";;
        --mc-fw-3blk)
            export MC_FW_FLASH_NAME="${MC_FW_FLASH_DIR}/cmod_ef100p0_3x_vblk.mcs";;
        --mc-fw-2blk)
            export MC_FW_FLASH_NAME="${MC_FW_FLASH_DIR}/cmod_ef100p0_vnetp0_2x_vblk.mcs";;
        --mc-fw-blk)
            export MC_FW_FLASH_NAME="${MC_FW_FLASH_DIR}/cmod_ef100p0_ef100p0_vnetp0_vblk.mcs";;
        --mc-fw-pciid=*)
            export MC_FW_PCIID=${1#--mc-fw-pciid=}
            unset MC_FW_PCIID_TYPE
            ;;
        --virtio-modern)
            export MC_FW_PCIID_TYPE=modern;;
        --virtio-transitional)
            export MC_FW_PCIID_TYPE=transitional;;
        --cmod-multihost)
            check_is_run_mode_set
            RUN_OPTS+="--opts=run/$CFG.cmod "
            RUN_OPTS+="--opts=run/cmod "
            RUN_OPTS+="--script=env/cmod-multihost ";;
        --cmod-singlehost)
            check_is_run_mode_set
            RUN_OPTS+="--opts=run/$CFG.cmod "
            RUN_OPTS+="--opts=run/cmod "
            RUN_OPTS+="--script=env/cmod-singlehost ";;
        --bp-logflags=*)
            export TE_BP_LOGFLAGS=${1#--bp-logflags=}
             ;;
        --ool-param)
             export ${2%=*}=${2#*=}
             shift
             ;;
        --nullblock)
            check_is_run_mode_set
            BACKEND_MODE=nullblock;;
        --ceph-qemu)
            check_is_ceph_mode_set "ceph-qemu"
            BACKEND_MODE=ceph
            TE_BACKEND_SRV_TYPE="qemu"
            RUN_OPTS+="--opts=run/$CFG.ceph-qemu "
            RUN_OPTS+="--script=env/ceph-qemu ";;
        --ceph-container)
            check_is_ceph_mode_set "ceph-container"
            BACKEND_MODE=ceph
            TE_BACKEND_SRV_TYPE="container"
            RUN_OPTS+="--opts=run/$CFG.ceph-container " ;;
        --ceph-connect)
            check_is_ceph_mode_set "ceph-connect"
            BACKEND_MODE=ceph
            TE_BACKEND_SRV_TYPE="connect"
            RUN_OPTS+="--opts=run/$CFG.ceph-connect "
            RUN_OPTS+="--script=scripts/get_ceph_info.sh " ;;
        --nvme)
            check_is_run_mode_set
            BACKEND_MODE=nvme
            RUN_OPTS+="--opts=run/$CFG.nvme "
            ;;
        --nvme-connect)
            check_is_run_mode_set
            BACKEND_MODE=nvme
            TE_BACKEND_SRV_TYPE="connect"
            RUN_OPTS+="--opts=run/$CFG.nvme-connect "
            ;;
        --onload)
            export TE_USE_ONLOAD=true
            RUN_OPTS+="--script=env/onload ";;
        --ceph-osd-num=*)
            export TE_CEPH_OSD_NUM=${1#--ceph-osd-num=};;
        --ceph-stor-size=*)
            export TE_CEPH_STOR_SIZE_GB=${1#--ceph-stor-size=};;
        --ceph-enable-logging)
            RUN_OPTS+="--script=env/ceph-logging ";;
        --enable-plugin)
            export TE_USE_ONLOAD=true
            RUN_OPTS+="--script=env/plugin "
            ;;

        --arm-mtu=*)
            export TE_ARM_MTU=${1#--arm-mtu=};;

        --old-spdk)
            export TE_OLD_SPDK=true
            ;;

        --perf-short)
            export PERF_SHORT=true
            ;;

        --sigusr2-cold-reboot)
            export TSAPI_SIGUSR2_COLD_REBOOT=true
            RUN_OPTS+="--script=env/power_ctl "
            ;;
        *)
            RUN_OPTS+="${1} ";;
    esac
    shift 1
done

# Add test suite default options after configuration specifics
RUN_OPTS+="--opts=opts.ts "

if [[ "$WITH_BROKEN" != "true" ]]; then
    RUN_OPTS+="--tester-req=\!BROKEN "
fi

if [[ $MC_FW_FLASH_NAME == *"ef100_vblk"* ]]; then
    RUN_OPTS+="--tester-req=\!MULTI_BLOCK "
fi

guess_post_opts

# Make TE workspace directory.
# Nothing will be done if TE_WORKSPACE_DIR isn't exported,
# /tmp directory will be used as default in this case.
make_te_workspace_dirs "${TS_TOPDIR}/conf/env/$CFG"
if [[ "$ceph_mode" != "none" ]]; then
    make_te_workspace_dirs "${TS_TOPDIR}/conf/env/$CFG.$ceph_mode"
fi

$do_compilebench_build || unset COMPILEBENCH_SRC
$do_dbench_build || unset DBENCH_SRC
$do_fsmark_build || unset FSMARK_SRC
$do_iozone_build || unset IOZONE_SRC
$do_spdk_build || unset SPDK_SRC
$do_fio_build|| unset FIO_SRC
$do_xfs_build|| unset XFS_TS_DIR
$do_fio_build || unset FIO_SRC

CONF_DIRS=
CONF_DIRS+="\"${TS_TOPDIR}\"/conf:"
CONF_DIRS+="\"${TS_TOPDIR}\"/conf/matrices:"
CONF_DIRS+="${SF_TS_CONFDIR}"

DEFAULT_OPTS=
DEFAULT_OPTS+="--conf-dirs=${CONF_DIRS} "

DEFAULT_OPTS+="--trc-db=\"${TS_TOPDIR}\"/conf/trc.xml "
DEFAULT_OPTS+="--trc-html=trc-full.html "

DEFAULT_OPTS+="--log-html=html "

DEFAULT_OPTS+="--build-parallel "

# Add TRC tags based on environment and run.sh options
RUN_OPTS+="$(ts_trc_tags_build) "

# Make sure that permitions for keys is correct
if [[ -d $TE_SSH_KEY_DIR ]]; then
    chmod 600 $TE_SSH_KEY_DIR/*
fi

eval "${TE_BASE}/dispatcher.sh ${DEFAULT_OPTS} ${RUN_OPTS}"
RESULT=$?

if test ${RESULT} -ne 0 ; then
    echo FAIL
    echo ""
    exit ${RESULT}
fi
