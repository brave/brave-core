#!/bin/bash
#
# Update search engines
#
# Error Codes:
#   0 = Success
#   1 = Failed to export search engines from Git repository
#   2 = Failed to add untracked changes to the local Git branch
#   3 = Failed to commit changes to the local Git branch
#   4 = Failed to push changes to the remote Git repository

search_plugins_directory="Client/Assets/SearchPlugins"

if [ -d $search_plugins_directory ] ; then
  rm -Rf $search_plugins_directory
fi

echo "Exporting search plugins from Brave-IOS-SearchPlugins Git repository"
if ! svn export https://github.com/brave/Brave-iOS-SearchPlugins/trunk/iOSFinalResult $search_plugins_directory >>output.log 2>&1; then
  echo "Failed to export search plugins from Brave-IOS-SearchPlugins Git repository, please see output.log"
  exit 1
fi

echo "Adding untracked changes to the local Git branch..."
if ! git add $search_plugins_directory >>output.log 2>&1 ; then
  echo "Failed to add untracked changes to the local Git branch, please see output.log"
  exit 2
fi

if git diff --cached --exit-code >> /dev/null ; then
  echo "There are no untracked changes"
  exit 0
fi

echo "Committing changes to the local Git branch..."
if ! git commit --quiet -m "Updated search plugins" >>output.log 2>&1 ; then
  echo "Failed to commit changes to the local Git branch, please see output.log"
  exit 3
fi

echo "Pushing changes to the remote Git repository..."
if ! git push >>output.log 2>&1 ; then
  echo "Failed to push changes to the remote Git repository, please see output.log"
  exit 4
fi

echo "Successfully updated search plugins"
