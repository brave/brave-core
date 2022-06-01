#!/bin/bash
#
# Download translations from Transifex
#
# Error Codes:
#   0 = Success
#   1 = USERNAME must be set
#   2 = PASSWORD must be set
#   3 = Unauthorized access
#   4 = Failed to download translations from Transifex

report_error()
{
  echo $2
  echo $2 >>output.log 2>&1
  cleanup
  exit $1
}

cleanup()
{
  if [ -e translation.xliff ] ; then
    cat translation.xliff >> output.log
    echo >> output.log
    rm translation.xliff
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

if [ ! -d "translated-xliffs" ] ; then
  report_error 4 "ERROR: \"translated-xliffs\" directory not found"
fi

language_codes=(fr pl ru de zh zh_TW id_ID it ja ko_KR ms pt_BR es uk nb sv tr)

for language_code in ${language_codes[@]} ; do
  echo "Downloading translations for language code \""$language_code"\""

  http_status_code=$(curl --silent --write-out %{http_code} --output translation.xliff --user ${USERNAME}:${PASSWORD} -X GET \
  https://www.transifex.com/api/2/project/brave-ios/resource/bravexliff/translation/${language_code}/?file)

  if [ $http_status_code == 401 ] ; then
    report_error 3 "ERROR: Unauthorized access"
  fi

  if [ $((http_status_code / 100)) != 2 ] ; then
    report_error 4 "ERROR: HTTP Status Code: $http_status_code"
  fi

  if [ -e translation.xliff ] ; then
    ./xliff-cleanup.py ./translation.xliff
    mv translation.xliff translated-xliffs/$language_code.xliff
  else
    report_error 4 "ERROR: \"translation.xliff\" file not found"
  fi
done

cleanup
