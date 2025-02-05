#!/bin/bash
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Push string changes to Transifex
#
# Error Codes:
#   0 = Success
#   1 = API Token must be set
#   2 = Unauthorized access
#   3 = Failed to export strings from Xcode project
#   4 = Failed to cleanup strings
#   5 = Failed to push string changes to Crowdin

report_error()
{
  echo $2
  echo $2 >>output.log 2>&1
  cleanup
  exit $1
}

cleanup()
{
  if [ -e transifex.log ] ; then
    cat transifex.log >> output.log
    echo >> output.log
    rm transifex.log
  fi

  if [ -e ../../Client/en.xcloc ]; then
    rm -R ../../Client/en.xcloc
  fi

  if [ -e en.xliff ] ; then
    rm en.xliff
  fi
}

if [ "${TOKEN}" = "" ] ; then
  report_error 1 "TOKEN environment variable must be set to \"api\""
fi

cd $(dirname "$0")

echo "Exporting strings from Xcode project..."
(cd ../../ && xcodebuild -exportLocalizations -exportLanguage en SWIFT_EMIT_LOC_STRINGS=NO) >>output.log 2>&1
if [ $? != 0 ] ; then
  report_error 4 "ERROR: Failed to export strings from Xcode project, please see output.log"
fi

mv -f ../../Client/en.xcloc/Localized\ Contents/en.xliff . >>output.log 2>&1

if [ $? != 0 ] ; then
  report_error 3 "ERROR: Failed to export strings from Xcode project, please see output.log"
fi

echo "Cleaning up strings..."
./xliff-cleanup.py en.xliff
if [ $? != 0 ] ; then
  report_error 4 "ERROR: Failed to cleanup strings, please see output.log"
fi

sed -i '' 's/Shared\/Supporting Files/brave/' en.xliff >>output.log 2>&1
if [ $? != 0 ] ; then
  report_error 4 "ERROR: Failed to cleanup strings, please see output.log"
fi

crowdin_project_id=33
crowdin_storage_id="null"

add_storage_id()
{
  FILE_PATH="en.xliff"
  result=$(curl --silent \
    -X "POST" "https://brave-software.crowdin.com/api/v2/storages" \
    -H "Authorization: Bearer ${TOKEN}" \
    -H 'Content-Type: application/octet-stream' \
    -H "Crowdin-API-FileName: en.xliff" \
    --data-binary @"$FILE_PATH"
  )
  storage_id=$(echo "$result" | jq '.data.id')
  echo $storage_id
}

delete_storage_id()
{
  http_status_code=$(curl --silent -o /dev/null -w "%{http_code}" \
  -X "DELETE" "https://brave-software.crowdin.com/api/v2/storages/$1" \
  -H "Authorization: Bearer ${TOKEN}" \
  -H "Content-Type: application/json"
  )
  echo $http_status_code
}

add_file()
{
  http_status_code=$(curl --silent -o /dev/null -w "%{http_code}" \
    -X "POST" "https://brave-software.crowdin.com/api/v2/projects/$crowdin_project_id/files" \
    -H "Authorization: Bearer ${TOKEN}" \
    -H 'Content-Type: application/json' \
    --data @- << EOF
      {
        "storageId": $crowdin_storage_id,
        "name": "en.xliff",
        "type": "xliff"
      }
EOF
  )
  echo $http_status_code
}

delete_file()
{
  http_status_code=$(curl --silent -o /dev/null -w "%{http_code}" \
    -X "DELETE" "https://brave-software.crowdin.com/api/v2/projects/$crowdin_project_id/files/$1" \
    -H "Authorization: Bearer ${TOKEN}" \
    -H 'Content-Type: application/json' \
  )
  echo $http_status_code
}

echo "Creating a storage id in Crowdin..."
crowdin_storage_id=$(add_storage_id)
if [ "$crowdin_storage_id" == "null" ]
then
  report_error 5 "ERROR: Failed to create a storage id in Crowdin"
else
  echo "Crowdin Storage ID: $crowdin_storage_id"
fi

echo "Checking if there is already a file id for our project in Crowdin..."
file_result=$(curl --silent \
  -X "GET" "https://brave-software.crowdin.com/api/v2/projects/$crowdin_project_id/files" \
  -H "Authorization: Bearer ${TOKEN}" \
  -H "Content-Type: application/json"
)
file_id=$(echo "$file_result" | jq '.data.[0].data.id')
if [ "$file_id" == "null" ]
then
  echo "No, there is no file. Adding a new file..."
  add_file_http_code=$(add_file)
  if [ $add_file_http_code != 201 ]
  then
    report_error 5 "ERROR: Failed to add a file to the project in Crowdin"
  else
    echo "The new source file has been added to the project in Crowdin"
  fi
else
  echo "Yes, there is a file id: $file_id. Deleting this file ..."
  delete_file_http_code=$(delete_file $file_id)
  if [ $delete_file_http_code != 204 ]
  then
    report_error 5 "ERROR: Failed to delete the existed file in Crowdin"
  else 
    echo "Now, adding a new file ..."
    add_file_http_code=$(add_file)
    if [ $add_file_http_code != 201 ]
    then
      report_error 5 "ERROR: Failed to add a file to the project in Crowdin"
    else
      echo "The new source file has added to the project in Crowdin"
    fi
  fi
fi

cleanup

echo "Successfully pushed string changes to Crowdin"
