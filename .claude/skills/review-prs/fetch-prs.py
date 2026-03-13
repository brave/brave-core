#!/usr/bin/env python3
"""Fetch and filter PRs for review.

Handles all PR fetching, filtering, and cache checking in one script
so the LLM doesn't burn tokens on this logic.

Usage: fetch-prs.py [days|page<N>|#<PR>] [open|closed|all] [--reviewer-priority <username>] [--max-prs <N>]

Examples:
  fetch-prs.py              # Default: 5 days, open PRs
  fetch-prs.py 3            # Last 3 days, open PRs
  fetch-prs.py page2        # Page 2 (PRs 21-40), open PRs
  fetch-prs.py 7 closed     # Last 7 days, closed PRs
  fetch-prs.py page1 all    # Page 1, all states
  fetch-prs.py #12345       # Single PR by number
  fetch-prs.py 12345        # Single PR by number (large numbers treated as PR#)
  fetch-prs.py 1 open --reviewer-priority user  # Prioritize PRs requesting review from user
  fetch-prs.py 1 open --max-prs 10              # Limit to 10 PRs per batch

Output: JSON with "prs" array and "summary" stats.
"""

import json
import os
import subprocess
import sys
from datetime import datetime, timedelta, timezone

# Add scripts/lib to path for config module
_script_dir = os.path.dirname(os.path.abspath(__file__))
_bot_dir = os.path.join(_script_dir, "..", "..", "..")
sys.path.insert(0, os.path.join(_bot_dir, "scripts"))
from lib.load_config import load_config, require_config

_config = load_config()
PR_REPO = require_config(_config, "project.prRepository")

CACHE_PATH = ".ignore/review-prs-cache.json"
ORG_MEMBERS_PATH = ".ignore/org-members.txt"
SKIP_PREFIXES = ["CI run for", "Backport", "Update l10n"]
SKIP_CONTAINS = ["uplift to", "Just to test CI"]


def parse_args():
    mode = "days"
    days = 5
    page = None
    pr_number = None
    state = "open"
    reviewer_priority = None
    max_prs = None

    args = sys.argv[1:]
    i = 0
    while i < len(args):
        arg = args[i]
        if arg == "--reviewer-priority" and i + 1 < len(args):
            reviewer_priority = args[i + 1]
            i += 2
            continue
        elif arg == "--max-prs" and i + 1 < len(args):
            max_prs = int(args[i + 1])
            i += 2
            continue
        elif arg.startswith("#"):
            mode = "single"
            pr_number = int(arg[1:])
        elif arg.startswith("page"):
            mode = "page"
            page = int(arg[4:])
        elif arg in ("open", "closed", "all"):
            state = arg
        else:
            try:
                num = int(arg)
                # Large numbers (>365) are PR numbers, not day counts
                if num > 365:
                    mode = "single"
                    pr_number = num
                else:
                    days = num
            except ValueError:
                pass
        i += 1

    return mode, days, page, pr_number, state, reviewer_priority, max_prs


def has_any_approval(pr):
    """Check if a PR has any approval, even if reviewDecision isn't APPROVED."""
    if pr.get("reviewDecision") == "APPROVED":
        return True
    for review in pr.get("latestReviews", []):
        if review.get("state") == "APPROVED":
            return True
    return False


def fetch_single_pr(pr_number):
    fields = "number,title,updatedAt,author,isDraft,headRefOid,reviewDecision,latestReviews,reviewRequests"
    result = subprocess.run(
        [
            "gh",
            "pr",
            "view",
            str(pr_number),
            "--repo",
            PR_REPO,
            "--json",
            fields,
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    return [json.loads(result.stdout)]


def is_requested_reviewer(pr, username):
    """Check if the given username is a requested reviewer on the PR."""
    if not username:
        return False
    for req in pr.get("reviewRequests", []):
        login = req.get("login", "")
        if not login:
            login = req.get("name", "")
        if login.lower() == username.lower():
            return True
    return False


def fetch_prs(mode, days, page, pr_number, state):
    if mode == "single":
        return fetch_single_pr(pr_number)

    fields = "number,title,updatedAt,author,isDraft,headRefOid,reviewDecision,latestReviews,reviewRequests"
    base_cmd = [
        "gh",
        "pr",
        "list",
        "--repo",
        PR_REPO,
        "--state",
        state,
        "--json",
        fields,
    ]

    if mode == "page":
        limit = page * 20
        result = subprocess.run(
            base_cmd + ["--limit", str(limit)],
            capture_output=True,
            text=True,
            check=True,
        )
        prs = json.loads(result.stdout)
        prs.sort(key=lambda p: p.get("updatedAt", ""), reverse=True)
        start = (page - 1) * 20
        return prs[start : start + 20]
    else:
        result = subprocess.run(
            base_cmd + ["--limit", "500"],
            capture_output=True,
            text=True,
            check=True,
        )
        prs = json.loads(result.stdout)
        # Sort by updatedAt descending (newest first) since --sort is
        # not available in all gh versions
        prs.sort(key=lambda p: p.get("updatedAt", ""), reverse=True)
        return prs


def load_cache():
    try:
        with open(CACHE_PATH) as f:
            return json.load(f)
    except (FileNotFoundError, json.JSONDecodeError):
        return {}


def load_org_members():
    """Load Brave org member logins from the cached file."""
    try:
        with open(ORG_MEMBERS_PATH) as f:
            return set(line.strip() for line in f if line.strip())
    except FileNotFoundError:
        print(
            f"WARNING: org members file not found at {ORG_MEMBERS_PATH}",
            file=sys.stderr,
        )
        return set()


def should_skip_title(title):
    for prefix in SKIP_PREFIXES:
        if title.startswith(prefix):
            return True
    for pattern in SKIP_CONTAINS:
        if pattern in title:
            return True
    return False


def get_cutoff(mode, days, cache):
    """Determine the cutoff time for filtering PRs.

    Uses the last successful run timestamp from the cache if available,
    falling back to N days ago. This prevents gaps if a cron run is missed.
    """
    if mode != "days":
        return None

    last_run = cache.get("_last_run")
    if last_run:
        return datetime.fromisoformat(last_run)

    return datetime.now(timezone.utc) - timedelta(days=days)


def filter_prs(prs, mode, days, cache, org_members, reviewer_priority=None):
    cutoff = get_cutoff(mode, days, cache)
    approved = set(cache.get("_approved", []))

    to_review = []
    cached_prs = []
    skipped_filtered = 0
    skipped_cached = 0
    skipped_approved = 0
    skipped_external = 0

    for pr in prs:
        if pr.get("isDraft"):
            skipped_filtered += 1
            continue

        if should_skip_title(pr.get("title", "")):
            skipped_filtered += 1
            continue

        # Skip PRs from external contributors (non-org members)
        # Exception: allow through if bot is a requested reviewer on the PR
        author = pr.get("author", {}).get("login", "")
        if org_members and author not in org_members:
            if reviewer_priority and is_requested_reviewer(pr, reviewer_priority):
                pass  # Bot was asked to review this contributor PR
            else:
                skipped_external += 1
                continue

        if cutoff and mode == "days":
            updated = datetime.fromisoformat(pr["updatedAt"].replace("Z", "+00:00"))
            if updated < cutoff:
                # Don't filter out PRs where the bot is explicitly requested
                if not (reviewer_priority and is_requested_reviewer(pr, reviewer_priority)):
                    skipped_filtered += 1
                    continue

        pr_num = str(pr["number"])

        # Bot previously approved this PR — don't come back
        if pr_num in approved:
            skipped_approved += 1
            continue

        head_sha = pr.get("headRefOid", "")
        if cache.get(pr_num) == head_sha:
            # If the bot is a requested reviewer, force a full re-review
            # even if the SHA hasn't changed (explicit re-request)
            if reviewer_priority and is_requested_reviewer(pr, reviewer_priority):
                pass  # Fall through to to_review
            else:
                skipped_cached += 1
                cached_prs.append(pr)
                continue

        to_review.append(pr)

    return (
        to_review,
        cached_prs,
        skipped_filtered,
        skipped_cached,
        skipped_approved,
        skipped_external,
    )


def main():
    mode, days, page, pr_number, state, reviewer_priority, max_prs = parse_args()
    prs = fetch_prs(mode, days, page, pr_number, state)

    org_members = load_org_members()

    if mode == "single":
        # Skip all filtering for single PR review
        to_review = prs
        cached_prs = []
        skipped_filtered = 0
        skipped_cached = 0
        skipped_approved = 0
        skipped_external = 0
    else:
        cache = load_cache()
        (
            to_review,
            cached_prs,
            skipped_filtered,
            skipped_cached,
            skipped_approved,
            skipped_external,
        ) = filter_prs(prs, mode, days, cache, org_members, reviewer_priority)

    # Sort PRs so those requesting review from the priority user come first
    if reviewer_priority:
        to_review.sort(
            key=lambda pr: 0 if is_requested_reviewer(pr, reviewer_priority) else 1
        )
        cached_prs.sort(
            key=lambda pr: 0 if is_requested_reviewer(pr, reviewer_priority) else 1
        )

    # Apply max-prs limit after sorting (so priority PRs are kept first)
    skipped_max_prs = 0
    if max_prs is not None and len(to_review) > max_prs:
        skipped_max_prs = len(to_review) - max_prs
        to_review = to_review[:max_prs]

    def pr_entry(pr):
        author = pr.get("author", {}).get("login", "unknown")
        entry = {
            "number": pr["number"],
            "title": pr["title"],
            "headRefOid": pr["headRefOid"],
            "author": author,
            "hasApproval": has_any_approval(pr),
        }
        if reviewer_priority:
            entry["isRequestedReviewer"] = is_requested_reviewer(pr, reviewer_priority)
        if org_members and author not in org_members:
            entry["isExternalContributor"] = True
        return entry

    output = {
        "prs": [pr_entry(pr) for pr in to_review],
        "cached_prs": [pr_entry(pr) for pr in cached_prs],
        "summary": {
            "total_fetched": len(prs),
            "to_review": len(to_review),
            "cached_with_possible_threads": len(cached_prs),
            "skipped_filtered": skipped_filtered,
            "skipped_cached": skipped_cached,
            "skipped_approved": skipped_approved,
            "skipped_external": skipped_external,
            "skipped_max_prs": skipped_max_prs,
        },
    }

    json.dump(output, sys.stdout, indent=2)
    print()


if __name__ == "__main__":
    main()
