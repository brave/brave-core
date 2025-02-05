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
  result=$(curl -s -w "\n%{http_code}" --silent \
    -X "GET" "https://brave-software.crowdin.com/api/v2/projects/$crowdin_project_id/translations/builds/$1" \
    -H "Authorization: Bearer ${TOKEN}" \
    -H 'Content-Type: application/json'
  )
  http_code=$(tail -n1 <<< "$result")
  if [ "$http_code" != 200 ] ; then
    report_error 4 "Failed to check build status in Crowdin"
  fi
  response=$(sed '$ d' <<< "$result")
  build_status=$(echo "$response" | jq '.data.status')
  echo $build_status
}

start_build()
{
  result=$(curl -s -w "\n%{http_code}" --silent \
    -X "POST" "https://brave-software.crowdin.com/api/v2/projects/$crowdin_project_id/translations/builds" \
    -H "Authorization: Bearer ${TOKEN}" \
    -H 'Content-Type: application/json'
  )
  http_code=$(tail -n1 <<< "$result")
  if [ "$http_code" != 201 ] ; then
    report_error 4 "Failed to start a build in Crowdin"
  fi
  response=$(sed '$ d' <<< "$result")
  build_id=$(echo "$response" | jq '.data.id')
  echo $build_id
}

get_download_url()
{
  result=$(curl -s -w "\n%{http_code}" --silent \
    -X "GET" "https://brave-software.crowdin.com/api/v2/projects/$crowdin_project_id/translations/builds/${BUILD_ID}/download" \
    -H "Authorization: Bearer ${TOKEN}" \
    -H 'Content-Type: application/json'
  )
  http_code=$(tail -n1 <<< "$result")
  if [ "$http_code" != 200 ] ; then
    report_error 4 "Failed to get a download link from Crowdin."
  fi
  response=$(sed '$ d' <<< "$result")
  download_link=$(echo "$response" | jq '.data.url')
  echo $download_link | xargs
}

if [ "${TOKEN}" = "" ] ; then
  report_error 1 "TOKEN environment variable must be set to \"api\""
fi

crowdin_project_id=33
crowdin_build_id="null"

cd $(dirname "$0")
echo "Creating a temporary directory..."
if [ ! -d "translated-xliffs" ] ; then
  mkdir "translated-xliffs" >>output.log 2>&1
  if [ $? != 0 ] ; then
    report_error 3 "ERROR: Failed to create \"translated-xliffs\" directory, please see output.log"
  fi
fi

echo "Starting a new build..."
crowdin_build_id=$(start_build)
if [ "$crowdin_build_id" == "null" ]
then
  report_error 4 "Failed to get a build id"
else
  echo "New BUILD_ID: $crowdin_build_id"
  BUILD_ID=$crowdin_build_id
fi

echo "Checking BUILD_ID: ${BUILD_ID} status..."
crowdin_build_status=$(get_build_status ${BUILD_ID})
until [ "$crowdin_build_status" == '"finished"' ]
do
  echo "Current status: $crowdin_build_status"
  echo "..."
  sleep 3
  crowdin_build_status=$(get_build_status ${BUILD_ID})
done

echo "Build has finished. Downloading project translations..."
translated_download_url=$(get_download_url)
download_http_code=$(curl -o translated-xliffs/translation.zip -w "%{http_code}" "$translated_download_url")
if [ "$download_http_code" != 200 ]
then
  report_error 4 "ERROR: Failed to download translations from Crowdin, please see output.log"
else  
  unzip -d translated-xliffs/ translated-xliffs/translation.zip && rm translated-xliffs/translation.zip
fi

echo "Importing translations into Xcode project..."
for path in translated-xliffs/*.xliff ; do
  echo "Importing "$path" ..."
  (cd ../../ && xcodebuild -importLocalizations -localizationPath "l10n/tools/$path" SWIFT_EMIT_LOC_STRINGS=NO) >>output.log 2>&1
done
if [ $? != 0 ] ; then
  report_error 5 "ERROR: Failed to import translations into Xcode project, please see output.log"
fi

cleanup

echo "Successfully imported translations into Xcode project"