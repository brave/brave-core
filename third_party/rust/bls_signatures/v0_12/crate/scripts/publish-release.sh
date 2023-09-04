#!/usr/bin/env bash

RELEASE_BRANCH="master"
RELEASE_NAME="$CIRCLE_PROJECT_REPONAME-`uname`"
RELEASE_FILE="/tmp/$RELEASE_NAME.tar.gz"
RELEASE_TAG="${CIRCLE_SHA1:0:16}"

# make sure we're on the sanctioned branch
if [ "$CIRCLE_BRANCH" != "$RELEASE_BRANCH" ]; then
  echo "not on branch \"$RELEASE_BRANCH\", skipping publish"
  exit 0
fi

# make sure we have a token set, api requests won't work otherwise
if [ -z $GITHUB_TOKEN ]; then
  echo "\$GITHUB_TOKEN not set, publish failed"
  exit 1
fi

echo "preparing release file"

`dirname $0`/package-release.sh "$RELEASE_FILE"

echo "release file created: $RELEASE_FILE"

# see if the release already exists by tag
RELEASE_RESPONSE=`
  curl \
    --header "Authorization: token $GITHUB_TOKEN" \
    "https://api.github.com/repos/$CIRCLE_PROJECT_USERNAME/$CIRCLE_PROJECT_REPONAME/releases/tags/$RELEASE_TAG"
`

RELEASE_ID=`echo $RELEASE_RESPONSE | jq '.id'`

if [ "$RELEASE_ID" = "null" ]; then
  echo "creating release"

  RELEASE_DATA="{
    \"tag_name\": \"$RELEASE_TAG\",
    \"target_commitish\": \"$CIRCLE_SHA1\",
    \"name\": \"$RELEASE_TAG\",
    \"body\": \"\"
  }"

  # create it if it doesn't exist yet
  RELEASE_RESPONSE=`
    curl \
      --request POST \
      --header "Authorization: token $GITHUB_TOKEN" \
      --header "Content-Type: application/json" \
      --data "$RELEASE_DATA" \
      "https://api.github.com/repos/$CIRCLE_PROJECT_USERNAME/$CIRCLE_PROJECT_REPONAME/releases"
  `
else
  echo "release already exists"
fi

RELEASE_UPLOAD_URL=`echo $RELEASE_RESPONSE | jq -r '.upload_url' | cut -d'{' -f1`

curl \
  --request POST \
  --header "Authorization: token $GITHUB_TOKEN" \
  --header "Content-Type: application/octet-stream" \
  --data-binary "@$RELEASE_FILE" \
  "$RELEASE_UPLOAD_URL?name=$(basename $RELEASE_FILE)"

echo "release file uploaded"

