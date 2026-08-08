#!/usr/bin/env bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set -xeEo pipefail
shopt -s inherit_errexit

pr_number="${PR_NUMBER:?}"

read -ra relevant_pr_labels <<<"$(gh pr view -R "${GITHUB_REPOSITORY:?}" "$pr_number" \
  --json labels -q ".labels[].name" | grep -E "${LABELS// /|}" | tr '\n' '\t')"

if [[ "${relevant_pr_labels[*]}" ]]; then
  read -ra pr_issues <<<"$(gh api graphql -q \
    ".data.repository.pullRequest.closingIssuesReferences.edges[].node.url" -f query="{
      repository(owner: \"${GITHUB_REPOSITORY_OWNER:?}\", name: \"${GITHUB_REPOSITORY##*/}\") {
        pullRequest(number: ${pr_number:?}) { closingIssuesReferences (first: 100) { edges { node { url } } } }
      } }" | tr '\n' '\t')"

  for label in "${relevant_pr_labels[@]}"; do
    for issue in "${pr_issues[@]}"; do
      if [[ "$(gh label list -R "$(echo "${issue:?}" | cut -d/ -f4-5)" \
        -S "${label:?}" --json name -q ".[]|select(.name==\"$label\").name")" ]]; then
        gh issue edit "$issue" --add-label "$label"
      fi
    done
  done
fi
