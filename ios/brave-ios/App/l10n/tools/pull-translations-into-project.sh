#!/bin/bash
#
# Pull new translations into project
#
# Error Codes:
#   0 = Success
#   1 = USERNAME must be set
#   2 = PASSWORD must be set
#   3 = Failed to create "translated-xliffs" directory
#   4 = Failed to download translations from Transifex
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
  mkdir "translated-xliffs" >>output.log 2>&1
  if [ $? != 0 ] ; then
    report_error 3 "ERROR: Failed to create \"translated-xliffs\" directory, please see output.log"
  fi
fi

echo "Downloading translations from Transifex..."
USERNAME="${USERNAME}" PASSWORD="${PASSWORD}" swift ./download-translations-from-transifex.swift
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
