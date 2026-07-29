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

status() {
  echo "::${1:?}::${2:?}"
  echo "$2" >>"${GITHUB_STEP_SUMMARY:?}"
}

has_label() {
  gh pr view "${PR_NUMBER:?}" -R "${GITHUB_REPOSITORY:?}" --json labels -q '.labels[].name' |
    grep -qxF chromium-version-mismatch
}

label() {
  gh pr edit "${PR_NUMBER:?}" -R "${GITHUB_REPOSITORY:?}" "--${1:?}-label" chromium-version-mismatch >/dev/null
}

pr_ver="$(chromium_ver "${PR_SHA:?}")"
target_ver="$(chromium_ver "${GITHUB_BASE_REF:?}")"

comment() {
  gh pr comment "${PR_NUMBER:?}" -R "${GITHUB_REPOSITORY:?}" \
    -b "Chromium major version is behind target branch ($pr_ver vs $target_ver). Please rebase."
}

success() {
  status notice "${1:?}: CI ✅ | Merge ✅"
  label remove
}

failure() {
  local ci
  case "${1:?}" in
    runci) ci=✅; label remove ;;
    *) has_label || comment; ci=🚫; label add ;;
  esac
  status error "${2:?}: CI $ci | Merge 🚫"
  status error "Please rebase."
  exit 2
}

echo "::notice::PR branch: ${pr_ver:?}, target branch: ${target_ver:?}"

if [[ "$pr_ver" == "$target_ver" ]]; then
  success "Chromium version full match"
elif [[ "$(sort -V <<<"$target_ver"$'\n'"$pr_ver" | tail -n1)" == "$pr_ver" ]]; then
  success "Chromium version is newer in the PR"
elif [[ "${pr_ver%%.*}" == "${target_ver%%.*}" ]]; then
  success "Chromium major version match"
else
  failure blockci "Chromium version mismatch"
fi
