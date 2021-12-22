#!/usr/bin/env bash

set -Exeo pipefail

main() {
    if [[ -z "$1" ]]
    then
        (>&2 echo '[build-release/main] Error: script requires a library name, e.g. "filecoin" or "snark"')
        exit 1
    fi

    if [[ -z "$2" ]]
    then
        (>&2 echo '[build-release/main] Error: script requires a toolchain, e.g. ./build-release.sh +nightly-2019-04-19')
        exit 1
    fi

    if [[ -z "$3" ]]
    then
        (>&2 echo '[build-release/main] Error: script requires a build action, e.g. ./build-release.sh [build|lipo]')
        exit 1
    fi

    # temporary place for storing build output (cannot use 'local', because
    # 'trap' is not going to have access to variables scoped to this function)
    #
    __build_output_log_tmp=$(mktemp)

    # clean up temp file on exit
    #
    trap '{ rm -f $__build_output_log_tmp; }' EXIT

    # build with RUSTFLAGS configured to output linker flags for native libs
    #
    local __rust_flags="--print native-static-libs ${RUSTFLAGS}"

    RUSTFLAGS="${__rust_flags}" \
        cargo +$2 $3 \
        --release ${@:4} 2>&1 | tee ${__build_output_log_tmp}

    # parse build output for linker flags
    #
    local __linker_flags=$(cat ${__build_output_log_tmp} \
        | grep native-static-libs\: \
        | head -n 1 \
        | cut -d ':' -f 3)

    echo "Linker Flags: ${__linker_flags}"
    if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "x86_64" ]; then
        # With lipo enabled, this replacement may not be necessary,
        # but leaving it in doesn't hurt as it does nothing if not
        # needed
        __linker_flags=$(echo ${__linker_flags} | sed 's/-lOpenCL/-framework OpenCL/g')
        echo "Using Linker Flags: ${__linker_flags}"

        find . -type f -name "lib$1.a"
        rm -f ./target/aarch64-apple-darwin/release/libfilcrypto.a
        rm -f ./target/x86_64-apple-darwin/release/libfilcrypto.a
        echo "Eliminated non-universal binary libraries"
        find . -type f -name "lib$1.a"
    fi

    # generate pkg-config
    #
    sed -e "s;@VERSION@;$(git rev-parse HEAD);" \
        -e "s;@PRIVATE_LIBS@;${__linker_flags};" "$1.pc.template" > "$1.pc"

    # ensure header file was built
    #
    find -L . -type f -name "$1.h" | read

    # ensure the archive file was built
    #
    find -L . -type f -name "lib$1.a" | read
}

main "$@"; exit
