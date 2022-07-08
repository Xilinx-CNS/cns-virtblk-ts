#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.

export TS_TOPDIR="$(cd "$(dirname "$(which "$0")")"; pwd -P)"
if [ "$(basename ${TS_TOPDIR})" == "scripts" ]; then
    export TS_TOPDIR=$(dirname ${TS_TOPDIR})
fi

SSH_DEFAULT_OPTS="-o BatchMode=yes -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"

CONFIG_NAME=$1
shift

CONFIG_FILE=${TS_TOPDIR}/conf/env/${CONFIG_NAME}

check_failed() {
    echo "------------ CHECK FAILED -----------" >&2
    exit 1
}

try(){
    echo "--> Checking $*"
    if ! $*
    then
      echo "Failed: $*" >&2
      check_failed
    fi
}

echo "Checking configuration $CONFIG_NAME"

# Hack, cause we should look at run/ and read env name from there, but why bother
if ! [ -f ${CONFIG_FILE} ] ; then
    echo "No configuration file '${TS_TOPDIR}/conf/env/${CONFIG_NAME}'" >&2
    check_failed
fi

. ${CONFIG_FILE}

for agt in TE_DUT_TST{1,2} ; do
    agt_host=$(eval echo \$${agt})
    agt_name=$(eval echo \$${agt}_TA_NAME)
    agt_user=$(eval echo \$${agt}_SSH_USER)
    [ -z ${agt_user} ] && agt_user=${USER}

    case $agt_name in
        Agt_ARM)
            try ssh ${SSH_DEFAULT_OPTS} ${agt_user}@${agt_host} true
            ;;
        Agt_host)
            try ssh ${SSH_DEFAULT_OPTS} ${agt_user}@${agt_host} which fio
            try ssh ${SSH_DEFAULT_OPTS} ${agt_user}@${agt_host} which stress
            ;;
        *)
            echo "Nothing to check for agent $agt_name on $agt_host"
            ;;
    esac
done
