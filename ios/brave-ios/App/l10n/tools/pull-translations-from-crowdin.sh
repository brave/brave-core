#!/bin/bash
# Copyright (c) 2018 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Pull new translations into project
#
# Error Codes:
#   0 = Success
#   1 = TOKEN must be set
#   2 = Failed to build project in Crowdin
#   3 = Failed to create "translated-xliffs" directory
#   4 = Failed to download translations from Crowdin
#   5 = Failed to import translations into Xcode project

report_error()
{
  echo $2
  echo $2 >>output.log 2>&1
  cleanup
  exit $1
}

cleanup()
{
  if [ -d translated-xliffs ] ; then
    rm -Rf translated-xliffs
  fi
}

get_build_status() 
{
  result=$(curl --silent \
    -X "GET" "https://brave-software.crowdin.com/api/v2/projects/$crowdin_project_id/translations/builds/$1" \
    -H "Authorization: Bearer ${TOKEN}" \
    -H 'Content-Type: application/json'
  )
  http_status=$(echo "$response" | head -n 1 | awk '{print $2}')
  if [ $http_status != 200 ] ; then
    report_error 4 "Failed to check build status in Crowdin"
  fi
  build_status=$(echo "$result" | jq '.data.status')
  echo $build_status
}

start_build()
{
  result=$(curl --silent \
    -X "POST" "https://brave-software.crowdin.com/api/v2/projects/$crowdin_project_id/translations/builds" \
    -H "Authorization: Bearer ${TOKEN}" \
    -H 'Content-Type: application/json'
  )
  http_status=$(echo "$result" | head -n 1 | awk '{print $2}')
  if [ $http_status != 201 ] ; then
    report_error 4 "Failed to start a build in Crowdin"
  fi
  build_id=$(echo "$result" | jq '.data.id')
  echo $build_id
}

get_download_url()
{
  result=$(curl --silent \
    -X "GET" "https://brave-software.crowdin.com/api/v2/projects/$crowdin_project_id/translations/builds/${BUILD_ID}/download" \
    -H "Authorization: Bearer ${TOKEN}" \
    -H 'Content-Type: application/json'
  )
  http_status=$(echo "$result" | head -n 1 | awk '{print $2}')
  if [ $http_status != 200 ] ; then
    report_error 4 "Failed to get a download link from Crowdin."
  fi
  download_link=$(echo "$result" | jq '.data.url')
  echo $download_link
}

supported_languages()
{
  result=$(curl --silent \
    -X "GET" "https://brave-software.crowdin.com/api/v2/languages?limit=500" \
    -H "Authorization: Bearer ${TOKEN}" \
    -H 'Content-Type: application/json'
  )
  echo $result
  http_status=$(echo "$result" | head -n 1 | awk '{print $2}')
  if [ $http_status != 200 ] ; then
    report_error 4 "Failed to get a list of supported languages from Crowdin."
  fi
  echo $result
}

if [ "${TOKEN}" = "" ] ; then
  report_error 1 "TOKEN environment variable must be set to \"api\""
fi

crowdin_project_id=33
crowdin_build_id="null"

supported_languages

cd $(dirname "$0")
echo "Creating a temporary directory..."
if [ ! -d "translated-xliffs" ] ; then
  mkdir "translated-xliffs" >>output.log 2>&1
  if [ $? != 0 ] ; then
    report_error 3 "ERROR: Failed to create \"translated-xliffs\" directory, please see output.log"
  fi
fi

if [ "${BUILD_ID}" = "" ]
then
  echo "BUILD_ID is NOT provided. Starting a new build..."
  crowdin_build_id=$(start_build)
  if [ "$crowdin_build_id" == "null" ]
  then
    report_error 4 "Failed to get a build id"
  else
    echo "New BUILD_ID: $crowdin_build_id"
    echo "Please run this script again with this BUILD_ID when this build is finished."
  fi
else
  echo "Checking BUILD_ID: ${BUILD_ID} status..."
  crowdin_build_status=$(get_build_status ${BUILD_ID})
  if [ $crowdin_build_status != "finished" ]
  then
    report_error 4 "BUILD_ID: ${BUILD_ID} has not yet finished. Please run this script again when build is finished."
  else
    echo "Build has finished. Downloading project translations..."
    translated_download_url=$(get_download_url)

    TOKEN="${TOKEN}" swift ./download-translations-from-transifex.swift
    if [ $? != 0 ] ; then
      report_error 4 "ERROR: Failed to download translations from Transifex, please see output.log"
    fi

    echo "Importing translations into Xcode project..."
      for path in translated-xliffs/*.xliff ; do
        (cd ../../ && xcodebuild -importLocalizations -localizationPath "l10n/tools/$path" SWIFT_EMIT_LOC_STRINGS=NO) >>output.log 2>&1
      done
    if [ $? != 0 ] ; then
      report_error 5 "ERROR: Failed to import translations into Xcode project, please see output.log"
    fi

    cleanup

    echo "Successfully imported translations into Xcode project"
    fi
fi
