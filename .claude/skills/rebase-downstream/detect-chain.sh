#!/usr/bin/env bash
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
# detect-chain.sh - Detect downstream branch tree from a starting branch.
#
# Usage: detect-chain.sh [starting-branch]
#
# Outputs "branch:parent" per line in rebase order (depth-first pre-order).
# Handles trees with sibling branches (forks).
#
# Compatible with bash 3+ (no associative arrays).

set -euo pipefail

start_branch="${1:-$(git branch --show-current)}"

if [[ -z "$start_branch" ]]; then
  echo "Error: No branch specified and not on a branch." >&2
  exit 1
fi

# Use temp files to store parent and children maps (bash 3 compat)
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

parent_dir="$tmpdir/parents"
children_dir="$tmpdir/children"
mkdir -p "$parent_dir" "$children_dir"

# Sanitize branch name for use as filename (replace / with __)
sanitize() {
  echo "$1" | sed 's|/|__|g'
}

set_parent() {
  local branch="$1" parent="$2"
  echo "$parent" > "$parent_dir/$(sanitize "$branch")"
}

get_parent() {
  local f="$parent_dir/$(sanitize "$1")"
  [[ -f "$f" ]] && cat "$f" || true
}

has_parent() {
  [[ -f "$parent_dir/$(sanitize "$1")" ]]
}

add_child() {
  local parent="$1" child="$2"
  echo "$child" >> "$children_dir/$(sanitize "$parent")"
}

get_children() {
  local f="$children_dir/$(sanitize "$1")"
  [[ -f "$f" ]] && cat "$f" || true
}

# Collect all local branches
all_branches=$(git for-each-ref --format='%(refname:short)' refs/heads/)

# is_ancestor_of: check if $1 (or any historical tip of $1) is an
# ancestor of $2. Handles rebased branches by walking the reflog.
is_ancestor_of() {
  local ref="$1" target="$2"
  if git merge-base --is-ancestor "$ref" "$target" 2>/dev/null; then
    return 0
  fi
  # Fallback: if the local branch was rewritten (e.g., rebased with new
  # hashes), the old commits still exist on origin/<branch>.
  if git show-ref --verify --quiet "refs/remotes/origin/$ref" 2>/dev/null; then
    if git merge-base --is-ancestor "origin/$ref" "$target" 2>/dev/null; then
      return 0
    fi
  fi
  # Fallback: walk the reflog of $ref to find old tips that are still
  # ancestors of $target. This catches cases where both local and origin
  # have been rebased (new commits) but the downstream branch still has
  # the old commits.
  #
  # IMPORTANT: Skip reflog entries that are ancestors of the current tip.
  # After "git reset --hard <base> && git commit", the base SHA appears
  # in the reflog and is an ancestor of every branch, causing every
  # local branch to be falsely detected as a descendant. Only entries
  # representing rewritten-away history (old tips not reachable from the
  # current tip) should be checked.
  local ref_tip old_sha
  ref_tip=$(git rev-parse "$ref" 2>/dev/null) || return 1
  while IFS= read -r old_sha; do
    [[ -z "$old_sha" ]] && continue
    # Skip if this SHA is reachable from current tip (not rewritten-away)
    if git merge-base --is-ancestor "$old_sha" "$ref_tip" 2>/dev/null; then
      continue
    fi
    if git merge-base --is-ancestor "$old_sha" "$target" 2>/dev/null; then
      return 0
    fi
  done < <(git reflog show "$ref" --format='%H' 2>/dev/null \
    | awk '!seen[$0]++' | head -50)
  return 1
}

# Find all descendant branches of start_branch using ancestor checks.
descendants=()
for branch in $all_branches; do
  [[ "$branch" == "$start_branch" ]] && continue

  if is_ancestor_of "$start_branch" "$branch"; then
    descendants+=("$branch")
  fi
done

# For each descendant, find its closest parent among start_branch + other
# descendants. The closest parent is the one whose HEAD is an ancestor of
# the branch AND is not an ancestor of any other ancestor candidate.
for branch in "${descendants[@]+"${descendants[@]}"}"; do
  closest_parent="$start_branch"
  for other in "${descendants[@]+"${descendants[@]}"}"; do
    [[ "$other" == "$branch" ]] && continue
    # If other is an ancestor of branch AND other is a descendant of
    # our current closest_parent, then other is a closer parent.
    if is_ancestor_of "$other" "$branch" && \
       is_ancestor_of "$closest_parent" "$other"; then
      closest_parent="$other"
    fi
  done
  set_parent "$branch" "$closest_parent"
done

# Build children map from parent map
for f in "$parent_dir"/*; do
  [[ -f "$f" ]] || continue
  branch=$(basename "$f" | sed 's|__|/|g')
  parent=$(cat "$f")
  add_child "$parent" "$branch"
done

# Walk the tree starting from start_branch (depth-first pre-order)
walk_tree() {
  local current="$1"
  local kids
  kids=$(get_children "$current")

  if [[ -z "$kids" ]]; then
    return
  fi

  # Read children into an array
  local kid_array=()
  while IFS= read -r k; do
    [[ -n "$k" ]] && kid_array+=("$k")
  done <<< "$kids"

  # Visit each child and recurse (parents always before children)
  for child in "${kid_array[@]}"; do
    echo "$child:$current"
    walk_tree "$child"
  done
}

walk_tree "$start_branch"
