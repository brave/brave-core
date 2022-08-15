#!/usr/bin/env bash

set -eux -o pipefail

PR_NUM=14619
JQ="jq -e -r"

# gh_pr_view=$(gh pr view 14619 --json baseRefName,headRefName,headRepository,headRepositoryOwner,id,isCrossRepository,url)
gh_pr_view='{"baseRefName":"master","headRefName":"fork-dummy","headRepository":{"id":"R_kgDOHbZlRA","name":"brave-core"},"headRepositoryOwner":{"id":"MDQ6VXNlcjU5NjY3","name":"Ahmed Kamal","login":"kim0"},"id":"PR_kwDOBpF-cM49K3VG","isCrossRepository":true,"url":"https://github.com/brave/brave-core/pull/14619"}'

baseRefName=$($JQ ".baseRefName" <<<"$gh_pr_view")
headRefName=$($JQ ".headRefName" <<<"$gh_pr_view")
headRepositoryName=$($JQ ".headRepository.name" <<<"$gh_pr_view")
headRepositoryOwnerLogin=$($JQ ".headRepositoryOwner.login" <<<"$gh_pr_view")
isCrossRepository=$($JQ ".isCrossRepository" <<<"$gh_pr_view")
url=$($JQ ".url" <<<"$gh_pr_view")

[[ "$isCrossRepository" = "true" ]] || { echo "PR is not cross repo. Exiting!"; exit 1; }

git remote add contributor "https://github.com/$headRepositoryOwnerLogin/$headRepositoryName.git" || :
contribHeadRefName="contributor-$headRepositoryOwnerLogin-$headRefName"
git fetch contributor +"$headRefName":"$contribHeadRefName"
git push upstream "$contribHeadRefName" # TODO rename upstream => origin
# TODO remove `-d` draft
gh pr create -d -F ".github/PULL_REQUEST_TEMPLATE.md" -B "$baseRefName" -H "$contribHeadRefName" -t "Don't merge - this is a CI run for $url"
