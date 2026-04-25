#!/bin/bash

set -u

# if there is no directory for this $CI_COMMIT_REF_NAME/$CI_JOB_NAME
# create such directory and
# copy recursively all the files from the newest dir which has $CI_JOB_NAME, if it exists

# cache lives in /ci-cache/${CI_PROJECT_NAME}/${2}/${CI_COMMIT_REF_NAME}/${CI_JOB_NAME}

function prepopulate {
  if [[ ! -d $1 ]]; then
    mkdir -p "/ci-cache/$CI_PROJECT_NAME/$2/$CI_COMMIT_REF_NAME";
    FRESH_CACHE=$(find "/ci-cache/$CI_PROJECT_NAME/$2" -mindepth 2 -maxdepth 2 \
      -type d -name "$CI_JOB_NAME"  -exec stat --printf="%Y\t%n\n" {} \; |sort -n -r |head -1 |cut -f2);
    if [[ -d $FRESH_CACHE ]]; then
      echo "____Using" "$FRESH_CACHE" "to prepopulate the cache____";
      time cp -r "$FRESH_CACHE" "$1";
    else
      echo "_____No such $2 dir, proceeding from scratch_____";
    fi
  else
    echo "____No need to prepopulate $2 cache____";
  fi
}

prepopulate "$CARGO_TARGET_DIR" targets
