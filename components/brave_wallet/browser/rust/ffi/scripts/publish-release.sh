#!/usr/bin/env bash

set -Exeuo pipefail

main() {
    if [[ -z "$1" ]]
    then
        (>&2 echo '[publish-release/main] Error: script requires a release (gzipped) tarball path, e.g. "/tmp/filecoin-ffi-Darwin-standard.tar.tz"')
        exit 1
    fi

    if [[ -z "$2" ]]
    then
        (>&2 echo '[publish-release/main] Error: script requires a release name, e.g. "filecoin-ffi-Darwin-standard" or "filecoin-ffi-Linux-standard"')
        exit 1
    fi

    local __release_file=$1
    local __release_name=$2
    local __release_tag="${CIRCLE_SHA1:0:16}"

    # make sure we have a token set, api requests won't work otherwise
    if [ -z $GITHUB_TOKEN ]; then
        (>&2 echo "[publish-release/main] \$GITHUB_TOKEN not set, publish failed")
        exit 1
    fi

    # see if the release already exists by tag
    local __release_response=`
        curl \
            --header "Authorization: token $GITHUB_TOKEN" \
            "https://api.github.com/repos/$CIRCLE_PROJECT_USERNAME/$CIRCLE_PROJECT_REPONAME/releases/tags/$__release_tag"
    `

    local __release_id=`echo $__release_response | jq '.id'`

    if [ "$__release_id" = "null" ]; then
        (>&2 echo '[publish-release/main] creating release')

        RELEASE_DATA="{
            \"tag_name\": \"$__release_tag\",
            \"target_commitish\": \"$CIRCLE_SHA1\",
            \"name\": \"$__release_tag\",
            \"body\": \"\"
        }"

        # create it if it doesn't exist yet
        #
        __release_response=`
            curl \
                --request POST \
                --header "Authorization: token $GITHUB_TOKEN" \
                --header "Content-Type: application/json" \
                --data "$RELEASE_DATA" \
                "https://api.github.com/repos/$CIRCLE_PROJECT_USERNAME/$CIRCLE_PROJECT_REPONAME/releases"
        `
    else
        (>&2 echo '[publish-release/main] release already exists')
    fi

    __release_upload_url=`echo $__release_response | jq -r '.upload_url' | cut -d'{' -f1`

    curl \
        --request POST \
        --header "Authorization: token $GITHUB_TOKEN" \
        --header "Content-Type: application/octet-stream" \
        --data-binary "@$__release_file" \
        "$__release_upload_url?name=$(basename $__release_file)"

    (>&2 echo '[publish-release/main] release file uploaded')
}

main "$@"; exit
