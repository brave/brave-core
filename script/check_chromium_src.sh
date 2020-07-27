#!/bin/bash
#
# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */
#
# This script looks for problems in chromium_src.
#
# 1. Look for files in chromium_src that don't have a corresponding
#    file under src tree (or grit).
#
# 2. Look for #defines that redefine symbols that aren't present in
#    the overriden file.
#
# 3. Look for #include statements of the overriden file that have
#    incorrect number of ../ in the path.
#
# !!! This script does return false positives, but better check a
# !!! few of those than not check at all.

# set -x

# Checks if an override file uses the correct number of ".." in the
# include statement for the original file.
check_includes() {
    local OVERRIDE=${1}
    # Calculate the expected number of parent dirs.
    local OVERRIDE_PATH=$(dirname "${OVERRIDE}")
    IFS='/' read -a OVERRIDE_PATH_PARTS <<< "${OVERRIDE_PATH}"
    # -1 for the dir where the file is.
    # +3 to get to chromium_src->brave->src.
    local EXPECTED_COUNT=$(( ${#OVERRIDE_PATH_PARTS[@]} - 1 + 3 ))
    # Locate the #include for the original file.
    local OVERRIDE_FILENAME=`basename ${OVERRIDE}`
    local INCLUDE=
    grep "^#include \"\.\./.*${OVERRIDE_FILENAME}" ${OVERRIDE} | while read -r INCLUDE ; do
        # Extract path from the #include.
        local INCLUDE_PATH=`echo ${INCLUDE} | cut -d'"' -f2`
        # Calculate the number of parent dirs in the path.
        IFS='/' read -a INCLUDE_PATH_PARTS <<< "${INCLUDE_PATH}"
        local COUNT=0
        for i in "${INCLUDE_PATH_PARTS[@]}"
        do
            if [ "$i" == ".." ]; then
                COUNT=$(( ${COUNT} + 1 ))
            else
                break
            fi
        done
        # Check actual vs expected.
        if [ ${COUNT} != ${EXPECTED_COUNT} ]; then
            echo "ERROR: while processing ${OVERRIDE}"
            # Sanity check
            pushd ${OVERRIDE_PATH} > /dev/null
            if [ ! -f ${INCLUDE_PATH} ]; then
                echo "       File ${INCLUDE_PATH} doesn't exist"
            fi
            popd > /dev/null
            echo "       Expected ${EXPECTED_COUNT} \"..\""
            echo "       Found ${COUNT} \"..\""
            echo "-------------------------"
        fi
    done
}

# Finds `#define TARGET REPLACEMENT` statements in the OVERRIDE file
# and attempts to find the TARGET in the original file.
check_defines() {
    local OVERRIDE=${1}
    local ORIGINAL=${2}
    local DEF=
    grep '^#define' ${OVERRIDE} | while read -r DEF ; do
        local REPLACEMENT=`echo ${DEF} | cut -d' ' -f3`
        if [ ! -z "${REPLACEMENT}" ]; then
            local TARGET=`echo ${DEF} | cut -d' ' -f2`
            if [[ ${TARGET} = BUILDFLAG_INTERNAL_* ]]; then
                TARGET=${TARGET#"BUILDFLAG_INTERNAL_"}
                TARGET=${TARGET%"()"}
            fi
            local FOUND=`grep ${TARGET} ${ORIGINAL}`
            if [ -z "$FOUND" ]; then
                echo "ERROR: Unable to find symbol ${TARGET} in ${ORIGINAL}"
                echo "-------------------------"
            fi
        fi
    done
}

# Checks that each path in the passed in list ($1) exists in the passed
# in directory ($2).
check_overrides() {
    echo "--------------------------------------------------"
    echo "Checking overrides in ${2} ..."
    echo "--------------------------------------------------"
    for FILE in ${1}; do
        local ORIGINAL_FILE=${2}/${FILE}
        if [ ! -f ${ORIGINAL_FILE} ]; then
            echo "WARNING: No source for override ${FILE}"
            echo "-------------------------"
        else
            check_defines "${FILE}" "${ORIGINAL_FILE}"
            if [ "${3}" = "true" ]; then
                check_includes "${FILE}"
            fi
        fi
    done
}

# Usage
usage() {
    local SCRIPT=`basename $0`
    echo "Usage: ${SCRIPT} [Component|Static|Debug|Release] [target_os= [target_arch=]]"
    exit 1
}


# Path to src
SRC="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../../" >/dev/null 2>&1 && pwd )"
if [ ! -d ${SRC} ]; then
    echo "ERROR: ${SRC} is not a valid directory."
    usage
fi

# Build type, target_os, target_arch (used when looking for GRIT overrides).
# $1, $2, $3 Optional.
BUILD=Component
TARGET_OS=
TARGET_ARCH=

# Args
set_target_os_and_arch() {
    if [ ! -z "${1}" ]; then
        if [[ ${1} != target_os=* ]]; then
            echo "Invalid argument: ${1}."
            usage
        else
            TARGET_OS=${1#"target_os="}_
            if [ ! -z "${2}" ]; then
                if [[ ${2} != target_arch=* ]]; then
                    echo "Invalid argument: ${2}."
                    usage
                else
                    TARGET_ARCH=_${2#"target_arch="}
                fi
            fi
        fi
    fi
}

if [ ! -z "$1" ]; then
    if [ "$1" != "Debug" -a "$1" != "Release" -a "$1" != "Component" -a "$1" != "Static" ]; then
        set_target_os_and_arch "$1" "$2"
    else
        BUILD=$1
        set_target_os_and_arch "$2" "$3"
    fi
fi

# Derive paths from SRC root.
CHROMIUM_SRC=${SRC}/brave/chromium_src
GEN=${SRC}/out/${TARGET_OS}${BUILD}${TARGET_ARCH}/gen
if [ ! -d ${GEN} ]; then
    echo "ERROR: ${GEN} is not a valid directory."
    usage
fi

# CD to chromium_src for convenience.
pushd ${CHROMIUM_SRC} > /dev/null

# Add files known to not have targets.
EXCLUDES=(
    './CPPLINT.cfg'
    '_\(unit\|browser\)test\(_mac\)\?.cc'
    'third_party/blink/renderer/modules/battery/navigator_batterytest.cc'
    'third_party/blink/renderer/modules/bluetooth/navigator_bluetoothtest.cc'
    'third_party/blink/renderer/modules/storage/brave_dom_window_storage.h'
    './chrome/installer/linux/common/brave-browser/chromium-browser.appdata.xml'
    './chrome/installer/linux/common/brave-browser/chromium-browser.info'
)
PATTERN=
for EXCLUDE in ${EXCLUDES[@]}; do
    if [ -z "${PATTERN}" ]; then
        PATTERN="${EXCLUDE}"
    else
        PATTERN="${PATTERN}\|${EXCLUDE}"
    fi
done;

# Check non-GRIT overrides.
SRC_OVERRIDES=`find . -type f -not -path "./python_modules/*" -not -path "*/grit/*" | grep -v "${PATTERN}" | cut -d'/' -f2-`
check_overrides "${SRC_OVERRIDES}" "${SRC}" "true"

# Check GRIT overrides.
GRIT_OVERRIDES=`find . -type f -path "*/grit/*" | cut -d'/' -f2-`
check_overrides "${GRIT_OVERRIDES}" "${GEN}" "false"

# CD back, to be polite...
popd > /dev/null
