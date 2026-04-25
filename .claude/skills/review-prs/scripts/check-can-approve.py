# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Gate check before the bot approves a PR.

The bot should ONLY approve when it has zero outstanding comments
of any kind — inline threads or body-level. A clean first review
with no violations (zero prior comments) is a valid approval.

Exit code 0 + "can_approve": true  → safe to approve
Exit code 1 + "can_approve": false → do NOT approve

This script is the SINGLE SOURCE OF TRUTH for approval decisions.
The LLM must not approve without exit code 0 from this script.

Usage:
    python3 check-can-approve.py <pr-number> <bot-username>
"""

import argparse
import json
import os
import subprocess
import sys

_script_dir = os.path.dirname(os.path.abspath(__file__))
_repo_dir = os.path.normpath(os.path.join(_script_dir, "..", "..", "..", ".."))
PR_REPO = "brave/brave-core"

REVIEW_CACHE_PATH = os.path.join(_repo_dir, ".ignore", "review-prs-cache.json")


def gh_api(endpoint, method="GET", input_data=None):
    """Call a GitHub API endpoint via gh CLI."""
    cmd = ["gh", "api", endpoint]
    if method != "GET":
        cmd += ["--method", method]
    if input_data:
        cmd += ["--input", "-"]
    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        timeout=30,
        input=json.dumps(input_data) if input_data else None,
        check=False,
    )
    if result.returncode != 0:
        return None
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return None


def gh_graphql(query, variables):
    """Call the GitHub GraphQL API via gh CLI."""
    cmd = ["gh", "api", "graphql", "-f", f"query={query}"]
    for key, value in variables.items():
        if isinstance(value, int):
            cmd += ["-F", f"{key}={value}"]
        else:
            cmd += ["-f", f"{key}={value}"]
    result = subprocess.run(cmd,
                            capture_output=True,
                            text=True,
                            timeout=30,
                            check=False)
    if result.returncode != 0:
        return None
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return None


def fetch_pr_data(pr_number,
                  bot_username,
                  repo_owner="brave",
                  repo_name="brave-core"):
    """Fetch all bot review data: inline threads + reviews.

    Returns (head_sha, threads_info, body_comments, already_approved)
    or (None, ...) on API failure.
    """
    # 1. Inline review threads via GraphQL
    query = """
    query($owner: String!, $name: String!, $number: Int!) {
      repository(owner: $owner, name: $name) {
        pullRequest(number: $number) {
          headRefOid
          reviewThreads(first: 100) {
            nodes {
              id
              isResolved
              comments(first: 1) {
                nodes {
                  author { login }
                }
              }
            }
          }
        }
      }
    }
    """
    data = gh_graphql(
        query,
        {
            "owner": repo_owner,
            "name": repo_name,
            "number": pr_number,
        },
    )
    if not data:
        return None, None, None, None

    try:
        pr = data["data"]["repository"]["pullRequest"]
        head_sha = pr["headRefOid"]
        threads = pr["reviewThreads"]["nodes"]
    except (KeyError, TypeError):
        return None, None, None, None

    total_threads = 0
    unresolved_threads = 0
    unresolved_ids = []
    for thread in threads:
        first_comments = thread.get("comments", {}).get("nodes", [])
        if not first_comments:
            continue
        author = (first_comments[0].get("author") or {}).get("login", "")
        if author == bot_username:
            total_threads += 1
            if not thread.get("isResolved", False):
                unresolved_threads += 1
                unresolved_ids.append(thread["id"])

    threads_info = {
        "total_bot_threads": total_threads,
        "unresolved_bot_threads": unresolved_threads,
        "unresolved_thread_ids": unresolved_ids,
    }

    # 2. Reviews via REST (for body-level comments and approval check)
    reviews = gh_api(
        f"repos/{repo_owner}/{repo_name}/pulls/{pr_number}/reviews")
    if not reviews:
        reviews = []

    # Body text patterns that are just bot signatures, not actual concerns.
    # These should not block approval.
    HARMLESS_BODY_PATTERNS = {
        "review via brave-dev-bot",
    }

    body_comments = []
    already_approved = False
    for review in reviews:
        if review.get("user", {}).get("login") != bot_username:
            continue
        state = review.get("state", "")
        # Body-level comments: any COMMENTED review with non-empty body,
        # excluding known harmless bot signatures.
        if state == "COMMENTED" and review.get("body", "").strip():
            body_text = review["body"].strip()
            if body_text.lower() not in HARMLESS_BODY_PATTERNS:
                body_comments.append({
                    "review_id": review.get("id"),
                    "body_preview": body_text[:120],
                })
        # Already approved at current SHA
        if state == "APPROVED" and review.get("commit_id") == head_sha:
            already_approved = True

    return head_sha, threads_info, body_comments, already_approved


def fail(reason, **extra):
    """Print failure JSON and exit 1."""
    result = {"can_approve": False, "reason": reason, **extra}
    print(json.dumps(result, indent=2))
    sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description="Gate check: can the bot approve this PR?")
    parser.add_argument("pr_number", type=int, help="PR number")
    parser.add_argument("bot_username", help="Bot's GitHub username")
    parser.add_argument("--repo", default=PR_REPO, help="owner/repo for PRs")
    args = parser.parse_args()

    if "/" in args.repo:
        repo_owner, repo_name = args.repo.split("/", 1)

    head_sha, threads_info, body_comments, already_approved = fetch_pr_data(
        args.pr_number, args.bot_username, repo_owner, repo_name)
    if head_sha is None:
        fail("Failed to fetch PR data from GitHub API")

    # How much feedback has the bot left on this PR?
    has_body_comments = len(body_comments) > 0

    # Bot must have NO outstanding body-level comments.
    # These have no resolution mechanism — they always block.
    if has_body_comments:
        fail(
            f"Bot has {len(body_comments)} outstanding body-level review "
            f"comment(s). These are not trackable as review threads and "
            f"cannot be resolved. The bot must not approve while it has "
            f"unresolved comments of any kind",
            body_comments=body_comments,
            head_sha=head_sha,
            **threads_info,
        )

    # Bot must have NO unresolved inline threads.
    if threads_info["unresolved_bot_threads"] > 0:
        fail(
            f"{threads_info['unresolved_bot_threads']} of "
            f"{threads_info['total_bot_threads']} bot review threads "
            f"are still unresolved",
            head_sha=head_sha,
            **threads_info,
        )

    # Bot must not have already approved at this SHA.
    if already_approved:
        fail(
            "Bot already approved at this SHA",
            head_sha=head_sha,
            **threads_info,
        )

    # Also check the local review cache — prevents double approval within
    # the same run if the GitHub API hasn't propagated the first approval yet.
    try:
        with open(REVIEW_CACHE_PATH) as f:
            cache = json.load(f)
        if str(args.pr_number) in set(cache.get("_approved", [])):
            fail(
                "PR already marked as approved in local cache",
                head_sha=head_sha,
                **threads_info,
            )
    except (FileNotFoundError, json.JSONDecodeError, IOError):
        pass  # No cache — that's fine, skip this check

    # All clear — no outstanding comments of any kind.
    # This covers both: all prior threads resolved, or clean first
    # review with no comments.
    total = threads_info["total_bot_threads"]
    if total > 0:
        reason = f"All {total} bot threads resolved, no outstanding comments"
    else:
        reason = "Clean review — no prior comments, no outstanding issues"
    result = {
        "can_approve": True,
        "reason": reason,
        "head_sha": head_sha,
        **threads_info,
    }
    print(json.dumps(result, indent=2))
    sys.exit(0)


if __name__ == "__main__":
    main()
