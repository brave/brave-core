#!/usr/bin/env bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set -eEo pipefail
shopt -s inherit_errexit

chromium_ver() {
  gh api -H "Accept: application/vnd.github.raw+json" \
    "https://api.github.com/repos/${GITHUB_REPOSITORY:?}/contents/package.json?ref=${1:?}" |
    jq -r .config.projects.chrome.tag
}

pr_ver="$(chromium_ver "${PR_SHA:?}")"
target_ver="$(chromium_ver "${TARGET_SHA:?}")"

echo "::notice::PR branch: ${GITHUB_HEAD_REF:?} (${pr_ver:?}), target branch: ${GITHUB_BASE_REF:?} (${target_ver:?})"

if [[ "${pr_ver%%.*}" != "${target_ver%%.*}" ]]; then
  echo "::notice::Rerunning compare-chromium-versions in PRs targeting ${GITHUB_BASE_REF:?}"
  while read -r pr_number head_sha; do
    run_id="$(gh api "/repos/$GITHUB_REPOSITORY/actions/workflows/compare-chromium-versions.yml/runs?head_sha=${head_sha:?}" -q '.workflow_runs[0].id')"
    pr_url="https://github.com/brave/brave-core/pull/${pr_number:?}"
    if [[ -n "$run_id" ]]; then
      echo "Rerunning $run_id for $pr_url"
      # TODO: Workflows older than 30 days will fail to rerun (by design), let's filter them out (using `|| true` for now)
      # https://docs.github.com/en/actions/managing-workflow-runs-and-deployments/managing-workflow-runs/re-running-workflows-and-jobs
      gh -R "$GITHUB_REPOSITORY" run rerun "$run_id" || true
    else
      echo "No run found for $pr_url"
    fi
    sleep 1
  done < <(gh -R "$GITHUB_REPOSITORY" pr list --limit 1000 --state open --base "$GITHUB_BASE_REF" \
    --json number,headRefOid -q '.[]|"\(.number)\t\(.headRefOid)"')
else
  echo "::notice::Chromium major versions match, nothing to do"
fi
