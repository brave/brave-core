#!/bin/bash
#
# Push string changes to Transifex
#
# Error Codes:
#   0 = Success
#   1 = USERNAME must be set
#   2 = PASSWORD must be set
#   3 = Unauthorized access
#   4 = Failed to export strings from Xcode project
#   5 = Failed to cleanup strings
#   6 = Failed to push string changes to Transifex

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

if [ "${USERNAME}" = "" ] ; then
  report_error 1 "USERNAME environment variable must be set to \"api\" or your Transifex username"
fi

if [ "${PASSWORD}" = "" ] ; then
  if [[ $(tr "[:upper:]" "[:lower:]" <<<"$USERNAME") = \
  $(tr "[:upper:]" "[:lower:]" <<<"api") ]] ; then
    report_error 2 "PASSWORD environment variable must be set to your Transifex API token"
  else
    report_error 2 "PASSWORD environment variable must be set to your Transifex password"
  fi
fi

cd $(dirname "$0")

echo "Exporting strings from Xcode project..."
(cd ../../ && xcodebuild -exportLocalizations SWIFT_EMIT_LOC_STRINGS=NO) >>output.log 2>&1
if [ $? != 0 ] ; then
  report_error 4 "ERROR: Failed to export strings from Xcode project, please see output.log"
fi

mv -f ../../Client/en.xcloc/Localized\ Contents/en.xliff . >>output.log 2>&1

if [ $? != 0 ] ; then
  report_error 4 "ERROR: Failed to export strings from Xcode project, please see output.log"
fi

echo "Cleaning up strings..."
./xliff-cleanup.py en.xliff
if [ $? != 0 ] ; then
  report_error 5 "ERROR: Failed to cleanup strings, please see output.log"
fi

sed -i '' 's/Shared\/Supporting Files/brave/' en.xliff >>output.log 2>&1
if [ $? != 0 ] ; then
  report_error 5 "ERROR: Failed to cleanup strings, please see output.log"
fi

base64xliff=$(cat en.xliff | base64)

echo "Pushing string changes to Transifex..."
http_status_code=$(curl --silent --write-out %{http_code} --output transifex.log \
  -X "POST" "https://rest.api.transifex.com/resource_strings_async_uploads" \
  -H "Authorization: Bearer ${PASSWORD}" \
  -H 'Content-Type: application/vnd.api+json' \
  --data-binary @- << EOF
    {
      "data": {
        "attributes": {
          "callback_url": null,
          "replace_edited_strings": false,
          "content": "${base64xliff}",
          "content_encoding": "base64"
        },
        "relationships": {
          "resource": {
            "data": {
              "type": "resources",
              "id": "o:brave:p:brave-ios:r:bravexliff"
            }
          }
        },
        "type": "resource_strings_async_uploads"
      }
    }
EOF
)

if [ $http_status_code == 401 ] ; then
  report_error 3 "ERROR: Unauthorized access, please see output.log"
fi

if [ $http_status_code != 202 ] ; then
  report_error 6 "ERROR: Failed to push string changes to Transifex, please see output.log"
fi

cleanup

echo "Successfully pushed string changes to Transifex"
