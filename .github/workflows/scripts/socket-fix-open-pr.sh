#!/usr/bin/env bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set -eEo pipefail
shopt -s inherit_errexit

if [ -z "$(git status --porcelain)" ]; then
  echo "::error::No changes produced by socket fix."
  exit 1
fi

BRANCH="socket-fix/$(echo "$GHSA_IDS" | tr ', ' '-' | tr -s '-' | cut -c1-60)"
HEAD_SHA=$(gh api "/repos/${GITHUB_REPOSITORY}/git/refs/heads/master" -q '.object.sha')

if gh api "repos/${GITHUB_REPOSITORY}/git/refs/heads/$BRANCH" -q .ref > /dev/null 2>&1; then
  gh api --method PATCH "/repos/${GITHUB_REPOSITORY}/git/refs/heads/$BRANCH" \
    --field sha="$HEAD_SHA" --field force=true
else
  gh api --method POST /repos/${GITHUB_REPOSITORY}/git/refs \
    --field ref="refs/heads/$BRANCH" --field sha="$HEAD_SHA"
fi

TREE_JSON="[]"
while IFS= read -r -d $'\0' line; do
  XY="${line:0:2}"
  FILE="${line:3}"
  # For rename/copy entries the old path follows as the next NUL entry;
  # emit a deletion for it so the source path is removed from the tree.
  if [[ "${XY:0:1}" =~ [RC] ]]; then
    IFS= read -r -d $'\0' _old_path
    if [[ "${XY:0:1}" == "R" ]]; then
      TREE_JSON=$(jq --arg path "$_old_path" \
        '. += [{"path": $path, "mode": "100644", "type": "blob", "sha": null}]' \
        <<< "$TREE_JSON")
    fi
  fi
  if [[ "${XY:1:1}" == "D" ]]; then
    TREE_JSON=$(jq --arg path "$FILE" \
      '. += [{"path": $path, "mode": "100644", "type": "blob", "sha": null}]' \
      <<< "$TREE_JSON")
  else
    MODE=$(git ls-files -s -- "$FILE" | awk 'NR==1 { print $1 }')
    [[ -z "$MODE" ]] && MODE="100644"
    if [[ "$MODE" == "120000" ]]; then
      BLOB_SHA=$(gh api --method POST /repos/${GITHUB_REPOSITORY}/git/blobs \
        --field content=@<(printf '%s' "$(readlink -- "$FILE")" | base64 -w 0) \
        --field encoding=base64 \
        -q .sha)
    else
      BLOB_SHA=$(gh api --method POST /repos/${GITHUB_REPOSITORY}/git/blobs \
        --field content=@<(base64 -w 0 -- "$FILE") \
        --field encoding=base64 \
        -q .sha)
    fi
    TREE_JSON=$(jq --arg path "$FILE" --arg mode "$MODE" --arg sha "$BLOB_SHA" \
      '. += [{"path": $path, "mode": $mode, "type": "blob", "sha": $sha}]' \
      <<< "$TREE_JSON")
  fi
done < <(git status --porcelain=v1 -z)

BASE_TREE=$(gh api /repos/${GITHUB_REPOSITORY}/git/commits/"$HEAD_SHA" -q .tree.sha)

NEW_TREE_SHA=$(jq -n \
  --arg base_tree "$BASE_TREE" \
  --argjson tree "$TREE_JSON" \
  '{"base_tree": $base_tree, "tree": $tree}' | \
  gh api --method POST /repos/${GITHUB_REPOSITORY}/git/trees --input - -q .sha)

NEW_COMMIT_SHA=$(jq -n \
  --arg tree "$NEW_TREE_SHA" \
  --arg parent "$HEAD_SHA" \
  '{"message": "fix: address security advisories", "tree": $tree, "parents": [$parent]}' | \
  gh api --method POST /repos/${GITHUB_REPOSITORY}/git/commits --input - -q .sha)

gh api --method PATCH /repos/${GITHUB_REPOSITORY}/git/refs/heads/"$BRANCH" \
  --field sha="$NEW_COMMIT_SHA"

BODY="Addresses: $GHSA_IDS"
IFS=',' read -ra LINKS <<< "$ISSUE_LINK"
for link in "${LINKS[@]}"; do
  BODY="$BODY"$'\n\n'"closes ${link// /}"
done
if [[ -n "$WDP_REF" ]]; then
  WDP_PR=$(gh api "repos/brave/web-discovery-project/commits/$WDP_REF/pulls" -q '.[0].html_url' 2>/dev/null || true)
  [[ -n "$WDP_PR" ]] && BODY="$BODY"$'\n\n'"WDP: $WDP_PR"
fi
if [[ -n "$LEO_REF" ]]; then
  LEO_PR=$(gh api "repos/brave/leo/commits/$LEO_REF/pulls" -q '.[0].html_url' 2>/dev/null || true)
  [[ -n "$LEO_PR" ]] && BODY="$BODY"$'\n\n'"Leo: $LEO_PR"
fi
if ! gh pr list --repo "${GITHUB_REPOSITORY}" --head "$BRANCH" --json number -q '.[].number' | grep -q .; then
  gh pr create \
    --repo "${GITHUB_REPOSITORY}" \
    --title "fix: address security advisories" \
    --body "$BODY" \
    --head "$BRANCH" \
    --base master \
    --label "CI/skip" \
    --label "security"
fi
