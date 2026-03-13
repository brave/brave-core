# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Phase 1 pre-work for the review-prs skill.

Produces a work directory with prompt files and a lightweight manifest.
Zero prompt construction tokens required from the LLM — subagent prompts
are written to files, not embedded in JSON.

Usage:
    python3 prepare-review.py [days|page<N>|#<PR>]
        [open|closed|all] [--auto]
        [--reviewer-priority] [--max-prs N]
"""

import importlib.util
import json
import os
import re
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
# Repo root: .claude/skills/review-prs -> .claude/skills -> .claude -> repo root
_REPO_DIR = os.path.normpath(os.path.join(_SCRIPT_DIR, "..", "..", ".."))

# Add to path for imports
sys.path.insert(0, _SCRIPT_DIR)

# Import fetch-prs functions (the module uses if __name__ guard)

_fp_spec = importlib.util.spec_from_file_location(
    "fetch_prs", os.path.join(_SCRIPT_DIR, "fetch-prs.py"))
_fp_mod = importlib.util.module_from_spec(_fp_spec)
_fp_spec.loader.exec_module(_fp_mod)

# Import chunk-best-practices functions
_cb_spec = importlib.util.spec_from_file_location(
    "chunk_best_practices", os.path.join(_SCRIPT_DIR,
                                         "chunk-best-practices.py"))
_cb_mod = importlib.util.module_from_spec(_cb_spec)
_cb_spec.loader.exec_module(_cb_mod)

# Import extract-pr-images functions
_ei_spec = importlib.util.spec_from_file_location(
    "extract_pr_images",
    os.path.join(_SCRIPT_DIR, "scripts", "extract-pr-images.py"))
_ei_mod = importlib.util.module_from_spec(_ei_spec)
_ei_spec.loader.exec_module(_ei_mod)

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------
PR_REPO = "brave/brave-core"
DEFAULT_BRANCH = "master"
CACHE_PATH = os.path.join(_REPO_DIR, ".ignore", "review-prs-cache.json")
BP_DIR = os.path.join(_REPO_DIR, "docs", "best-practices")
BP_LINK_BASE = (f"https://github.com/{PR_REPO}/tree/"
                f"{DEFAULT_BRANCH}/docs/best-practices")
TARGET_REPO_PATH = _REPO_DIR


def log(msg):
    print(msg, file=sys.stderr)


# ---------------------------------------------------------------------------
# Org members / trusted reviewers
# ---------------------------------------------------------------------------
def load_org_members():
    org_members_path = os.environ.get(
        "BRAVE_ORG_MEMBERS_PATH",
        os.path.join(_REPO_DIR, ".ignore", "org-members.txt"),
    )
    if not os.path.isfile(org_members_path):
        log(f"ERROR: org members file not found at {org_members_path}")
        log("Set BRAVE_ORG_MEMBERS_PATH to the correct location.")
        sys.exit(1)
    with open(org_members_path) as f:
        members = set(line.strip() for line in f if line.strip())
    trusted_reviewers_path = os.path.join(_SCRIPT_DIR, "scripts",
                                          "trusted-reviewers.txt")
    try:
        with open(trusted_reviewers_path) as f:
            members |= set(line.strip() for line in f if line.strip())
    except FileNotFoundError:
        pass
    return members


# ---------------------------------------------------------------------------
# CLI parsing
# ---------------------------------------------------------------------------
def parse_args():
    auto_mode = False
    reviewer_priority = False
    max_prs = None
    fetch_args = []

    args = sys.argv[1:]
    i = 0
    while i < len(args):
        arg = args[i]
        if arg == "--auto":
            auto_mode = True
        elif arg == "--reviewer-priority":
            reviewer_priority = True
        elif arg == "--max-prs" and i + 1 < len(args):
            max_prs = int(args[i + 1])
            i += 1
        else:
            fetch_args.append(arg)
        i += 1

    return auto_mode, reviewer_priority, max_prs, fetch_args


# ---------------------------------------------------------------------------
# Bot username
# ---------------------------------------------------------------------------
def resolve_bot_username():
    result = subprocess.run(
        ["gh", "api", "user", "--jq", ".login"],
        capture_output=True,
        text=True,
        timeout=15,
        check=False,
    )
    if result.returncode != 0:
        log(f"ERROR: failed to resolve bot username: {result.stderr}")
        sys.exit(1)
    return result.stdout.strip()


# ---------------------------------------------------------------------------
# Diff fetching
# ---------------------------------------------------------------------------
def fetch_diff(pr_number):
    result = subprocess.run(
        ["gh", "pr", "diff", "--repo", PR_REPO,
         str(pr_number)],
        capture_output=True,
        text=True,
        timeout=120,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(f"Failed to fetch diff: {result.stderr.strip()}")
    return result.stdout


# ---------------------------------------------------------------------------
# File classification from diff
# ---------------------------------------------------------------------------
def parse_diff_line_ranges(diff_text):
    """Parse diff to extract valid new-side line ranges per file.

    Returns {file_path: [(start, end), ...]} where each tuple is an
    inclusive range of lines present in the diff on the RIGHT (new) side.
    These are the only lines where GitHub allows inline review comments.
    """
    ranges = {}
    current_file = None
    for line in diff_text.split("\n"):
        if line.startswith("+++ b/"):
            current_file = line[6:]
            if current_file not in ranges:
                ranges[current_file] = []
        elif line.startswith("@@ ") and current_file:
            m = re.search(r'\+(\d+)(?:,(\d+))?', line)
            if m:
                start = int(m.group(1))
                count = int(m.group(2)) if m.group(2) else 1
                if count > 0:
                    ranges[current_file].append((start, start + count - 1))
    return ranges


def format_diff_line_ranges(diff_ranges):
    """Format diff line ranges as a readable string for subagent prompts."""
    lines = []
    for file_path, file_ranges in sorted(diff_ranges.items()):
        range_strs = [f"{s}-{e}" for s, e in file_ranges]
        lines.append(f"  {file_path}: {', '.join(range_strs)}")
    return "\n".join(lines)


def classify_files(diff_text):
    files = []
    for line in diff_text.splitlines():
        if line.startswith("diff --git"):
            # Extract b/ path
            m = re.search(r" b/(.+)$", line)
            if m:
                files.append(m.group(1))

    flags = {
        "has_cpp_files": False,
        "has_test_files": False,
        "has_chromium_src": False,
        "has_build_files": False,
        "has_frontend_files": False,
        "has_android_files": False,
        "has_ios_files": False,
        "has_patch_files": False,
        "has_nala_files": False,
        "has_localization_files": False,
    }

    for f in files:
        fl = f.lower()
        base = os.path.basename(fl)

        # C++ files
        if fl.endswith((".cc", ".h", ".mm")):
            flags["has_cpp_files"] = True

        # Test files
        if (fl.endswith("_test.cc") or fl.endswith("_browsertest.cc")
                or fl.endswith("_unittest.cc") or fl.endswith(".test.ts")
                or fl.endswith(".test.tsx")):
            flags["has_test_files"] = True

        # chromium_src
        if "chromium_src/" in f:
            flags["has_chromium_src"] = True

        # Build files
        if base in ("build.gn", "deps") or fl.endswith(".gni"):
            flags["has_build_files"] = True

        # Frontend files
        if fl.endswith((".ts", ".tsx", ".html", ".css")):
            flags["has_frontend_files"] = True

        # Android
        if fl.endswith((".java", ".kt")) or "android/" in f:
            flags["has_android_files"] = True

        # iOS
        if fl.endswith(".swift") or "ios/" in f:
            flags["has_ios_files"] = True

        # Patch files
        if fl.endswith(".patch") or "patches/" in f:
            flags["has_patch_files"] = True

        # Nala files
        if (re.search(r"/res/drawable/", f) or re.search(r"/res/values/", f)
                or re.search(r"/res/values-night/", f)
                or "components/vector_icons/" in f or fl.endswith(".icon")
                or fl.endswith(".svg")):
            flags["has_nala_files"] = True

        # Localization files
        if (fl.endswith((".grd", ".grdp", ".xtb")) or "l10n/" in f
                or "strings/" in f):
            flags["has_localization_files"] = True

    return flags


# ---------------------------------------------------------------------------
# Prior comments (reimplementation of filter-pr-reviews.sh in Python)
# ---------------------------------------------------------------------------
def _gh_api_paginated(endpoint):
    """Fetch paginated GitHub API results."""
    result = subprocess.run(
        ["gh", "api", endpoint, "--paginate"],
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )
    if result.returncode != 0:
        log(f"WARNING: gh api {endpoint} failed: {result.stderr.strip()}")
        return []
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return []


def _gh_api(endpoint):
    """Fetch a single GitHub API result (no pagination)."""
    result = subprocess.run(
        ["gh", "api", endpoint],
        capture_output=True,
        text=True,
        timeout=30,
        check=False,
    )
    if result.returncode != 0:
        log(f"WARNING: gh api {endpoint} failed: {result.stderr.strip()}")
        return None
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return None


def fetch_prior_comments(pr_number, org_members, include_author=None):
    """Reimplement filter-pr-reviews.sh: fetch PR data, reviews, review
    comments, issue comments. Filter by org membership. Return markdown."""

    repo = PR_REPO
    _, _ = repo.split("/", 1)

    # Fetch PR data
    pr_data = _gh_api(f"repos/{repo}/pulls/{pr_number}")
    if not pr_data:
        return None, False

    pr_title = pr_data.get("title", "")
    pr_author = (pr_data.get("user") or {}).get("login", "")
    pr_state = pr_data.get("state", "")
    pr_merged = pr_data.get("merged", False)
    pr_mergeable = pr_data.get("mergeable", "")

    # Fetch reviews, review comments, issue comments
    reviews = _gh_api_paginated(f"repos/{repo}/pulls/{pr_number}/reviews")
    review_comments = _gh_api_paginated(
        f"repos/{repo}/pulls/{pr_number}/comments")
    issue_comments = _gh_api_paginated(
        f"repos/{repo}/issues/{pr_number}/comments")

    # Get latest push timestamp
    head_sha = (pr_data.get("head") or {}).get("sha", "")
    latest_push_ts = ""
    if head_sha:
        commit_data = _gh_api(f"repos/{repo}/commits/{head_sha}")
        if commit_data:
            latest_push_ts = ((commit_data.get("commit")
                               or {}).get("committer") or {}).get("date", "")

    # Find latest reviewer activity from org members
    latest_reviewer_ts = ""

    def _is_org(username):
        return username in org_members

    for review in reviews:
        user = (review.get("user") or {}).get("login", "")
        if _is_org(user):
            ts = review.get("submitted_at", "")
            if ts > latest_reviewer_ts:
                latest_reviewer_ts = ts

    for comment in review_comments:
        user = (comment.get("user") or {}).get("login", "")
        if _is_org(user):
            ts = comment.get("created_at", "")
            if ts > latest_reviewer_ts:
                latest_reviewer_ts = ts

    for comment in issue_comments:
        user = (comment.get("user") or {}).get("login", "")
        if _is_org(user):
            ts = comment.get("created_at", "")
            if ts > latest_reviewer_ts:
                latest_reviewer_ts = ts

    # Determine who went last
    if not latest_reviewer_ts:
        who_went_last = "bot"
    elif latest_reviewer_ts > latest_push_ts:
        who_went_last = "reviewer"
    else:
        who_went_last = "bot"

    # Build markdown output
    lines = []
    lines.append(f"# PR #{pr_number}: {pr_title}")
    lines.append("")
    author_status = "(Brave org member)" if _is_org(
        pr_author) else "(EXTERNAL)"
    lines.append(f"**Author:** @{pr_author} {author_status}")
    lines.append(f"**State:** {pr_state}")
    lines.append(f"**Merged:** {pr_merged}")
    lines.append(f"**Mergeable:** {pr_mergeable}")
    lines.append("")

    # Include PR body for external contributor PRs if include_author matches
    if include_author and include_author == pr_author:
        pr_body = pr_data.get("body") or ""
        if pr_body:
            lines.append(
                f"## PR Description (from external contributor @{pr_author})")
            lines.append("")
            lines.append(pr_body)
            lines.append("")

    lines.append("## Timestamp Analysis")
    lines.append("")
    lines.append(f"**Latest Push:** {latest_push_ts}")
    lines.append(
        f"**Latest Reviewer Activity:** {latest_reviewer_ts or 'None'}")
    lines.append(f"**Who Went Last:** {who_went_last}")
    lines.append("")

    # Reviews section
    lines.append("## Reviews")
    lines.append("")
    if not reviews:
        lines.append("No reviews yet.")
        lines.append("")
    else:
        for review in reviews:
            user = (review.get("user") or {}).get("login", "")
            state = review.get("state", "")
            submitted = review.get("submitted_at", "")
            body = review.get("body") or ""
            if _is_org(user):
                lines.append(
                    f"### @{user} (Brave org member) - {state} - {submitted}")
                lines.append("")
                if body:
                    lines.append(body)
                lines.append("")
            else:
                lines.append(f"### @{user} (EXTERNAL) - {state} - {submitted}")
                lines.append("")
                lines.append("[Review filtered - external user]")
                lines.append("")

    # Review comments (inline code)
    lines.append("## Review Comments (Code)")
    lines.append("")
    if not review_comments:
        lines.append("No review comments.")
        lines.append("")
    else:
        for comment in review_comments:
            user = (comment.get("user") or {}).get("login", "")
            path = comment.get("path", "")
            created = comment.get("created_at", "")
            body = comment.get("body") or ""
            if _is_org(user):
                lines.append(f"### @{user} (Brave org member) - {created}")
                lines.append(f"**File:** {path}")
                lines.append("")
                lines.append(body)
                lines.append("")
            else:
                lines.append(f"### @{user} (EXTERNAL) - {created}")
                lines.append(f"**File:** {path}")
                lines.append("")
                lines.append("[Comment filtered - external user]")
                lines.append("")

    # Issue comments (discussion)
    lines.append("## Discussion Comments")
    lines.append("")
    if not issue_comments:
        lines.append("No discussion comments.")
        lines.append("")
    else:
        for comment in issue_comments:
            user = (comment.get("user") or {}).get("login", "")
            created = comment.get("created_at", "")
            body = comment.get("body") or ""
            if _is_org(user):
                lines.append(f"### @{user} (Brave org member) - {created}")
                lines.append("")
                lines.append(body)
                lines.append("")
            else:
                lines.append(f"### @{user} (EXTERNAL) - {created}")
                lines.append("")
                lines.append("[Comment filtered - external user]")
                lines.append("")

    markdown = "\n".join(lines)

    # Determine if there are any bot comments (will be used to decide
    # whether to run resolve-bot-threads)
    has_any_comment = bool(reviews) or bool(review_comments) or bool(
        issue_comments)
    return markdown if has_any_comment else None, has_any_comment


# ---------------------------------------------------------------------------
# Resolve bot threads (subprocess — it uses argparse)
# ---------------------------------------------------------------------------
def resolve_bot_threads(pr_number, bot_username):
    result = subprocess.run(
        [
            sys.executable,
            os.path.join(_SCRIPT_DIR, "scripts", "resolve-bot-threads.py"),
            str(pr_number),
            bot_username,
        ],
        capture_output=True,
        text=True,
        timeout=60,
        cwd=_REPO_DIR,
        check=False,
    )
    if result.returncode != 0:
        log(f"WARNING: resolve-bot-threads failed for "
            f"#{pr_number}: {result.stderr.strip()}")
        return {
            "resolved": 0,
            "unresolved_bot_threads": 0,
            "total_bot_threads": 0
        }
    try:
        data = json.loads(result.stdout)
        return {
            "resolved": len(data.get("resolved", [])),
            "unresolved_bot_threads": data.get("unresolved_bot_threads", 0),
            "total_bot_threads": data.get("total_bot_threads", 0),
        }
    except json.JSONDecodeError:
        return {
            "resolved": 0,
            "unresolved_bot_threads": 0,
            "total_bot_threads": 0
        }


# ---------------------------------------------------------------------------
# Check-can-approve (subprocess — exits non-zero when can't approve)
# ---------------------------------------------------------------------------
def check_can_approve(pr_number, bot_username):
    result = subprocess.run(
        [
            sys.executable,
            os.path.join(_SCRIPT_DIR, "scripts", "check-can-approve.py"),
            str(pr_number),
            bot_username,
        ],
        capture_output=True,
        text=True,
        timeout=30,
        cwd=_REPO_DIR,
        check=False,
    )
    try:
        data = json.loads(result.stdout)
    except json.JSONDecodeError:
        data = {"can_approve": False, "reason": "Failed to parse output"}
    return {
        "result": data.get("can_approve", False),
        "reason": data.get("reason", "unknown"),
    }


# ---------------------------------------------------------------------------
# Submit APPROVE review and update cache
# ---------------------------------------------------------------------------
def submit_approve(pr_number, head_sha):
    """Submit APPROVE review and mark as approved in cache."""
    # Submit APPROVE
    approve_input = json.dumps({"event": "APPROVE", "body": ""})
    result = subprocess.run(
        [
            "gh", "api", f"repos/{PR_REPO}/pulls/{pr_number}/reviews",
            "--method", "POST", "--input", "-"
        ],
        input=approve_input,
        capture_output=True,
        text=True,
        timeout=30,
        check=False,
    )
    if result.returncode != 0:
        log(f"WARNING: APPROVE failed for #{pr_number}: {result.stderr.strip()}"
            )
        return False

    # Update cache with --approve
    subprocess.run(
        [
            sys.executable,
            os.path.join(_SCRIPT_DIR, "update-cache.py"),
            str(pr_number),
            head_sha,
            "--approve",
        ],
        capture_output=True,
        text=True,
        timeout=10,
        cwd=_REPO_DIR,
        check=False,
    )
    return True


# ---------------------------------------------------------------------------
# Extract PR images (import from script)
# ---------------------------------------------------------------------------
def extract_images(pr_number):
    """Run extract-pr-images.py as subprocess and return images list."""
    result = subprocess.run(
        [
            sys.executable,
            os.path.join(_SCRIPT_DIR, "scripts", "extract-pr-images.py"),
            str(pr_number),
        ],
        capture_output=True,
        text=True,
        timeout=60,
        cwd=_REPO_DIR,
        check=False,
    )
    if result.returncode != 0:
        log(f"WARNING: extract-pr-images failed for "
            f"#{pr_number}: {result.stderr.strip()}")
        return []
    try:
        data = json.loads(result.stdout)
        return [{
            "abs_path": img.get("abs_path", img.get("path", "")),
            "source": img.get("source", ""),
            "alt": img.get("alt", "")
        } for img in data.get("images", [])]
    except json.JSONDecodeError:
        return []


# ---------------------------------------------------------------------------
# Discover best-practice docs (subprocess — uses argparse)
# ---------------------------------------------------------------------------
def discover_best_practices(file_flags):
    cmd = [
        sys.executable,
        os.path.join(_SCRIPT_DIR, "discover-best-practices.py"),
        BP_DIR,
    ]
    flag_map = {
        "has_cpp_files": "--has-cpp",
        "has_test_files": "--has-test",
        "has_chromium_src": "--has-chromium-src",
        "has_build_files": "--has-build",
        "has_frontend_files": "--has-frontend",
        "has_android_files": "--has-android",
        "has_ios_files": "--has-ios",
        "has_patch_files": "--has-patch",
        "has_nala_files": "--has-nala",
        "has_localization_files": "--has-localization",
    }
    for key, flag in flag_map.items():
        if file_flags.get(key):
            cmd.append(flag)

    result = subprocess.run(cmd,
                            capture_output=True,
                            text=True,
                            timeout=30,
                            check=False)
    if result.returncode != 0:
        log(f"WARNING: discover-best-practices failed: {result.stderr.strip()}"
            )
        return []
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return []


# ---------------------------------------------------------------------------
# Chunk best-practice docs (direct import)
# ---------------------------------------------------------------------------
def chunk_doc(doc_path):
    return _cb_mod.process_doc(doc_path)


# ---------------------------------------------------------------------------
# Subagent prompt builder
# ---------------------------------------------------------------------------

# The review rules from SKILL.md Steps 3-5, 7-8 — embedded verbatim.
# pylint: disable=line-too-long
_REVIEW_RULES = """\
Review Rules:
- Only flag violations in ADDED lines (+ lines), not existing code.
- Also flag bugs introduced by the change (e.g., missing string separators, duplicate DEPS entries, code inside wrong #if guard).
- Check surrounding context before making claims. When a violation involves dependencies, includes, or patterns, read the full file context (e.g., the BUILD.gn deps list, existing includes in the file) to verify your claim is accurate. Do NOT claim a PR "adds a dependency" or "introduces a pattern" if it already existed before the PR.
- Only comment on things the PR author introduced. If a dependency, pattern, or architectural issue already existed before this PR, do not flag it — even if it violates a best practice. The PR author is not responsible for pre-existing issues. Focus exclusively on what this PR changes or adds.
- Do not suggest renaming imported symbols defined outside the PR. When a + line imports or calls a function/class/variable from another module, and that symbol's definition is NOT in a file changed by the PR, do not comment on the symbol's naming. The PR author cannot rename it without modifying the upstream module, which is out of scope. Only flag naming issues on symbols that are defined or renamed within the PR's changed files.
- Respect the intent of the PR. If a PR is moving, renaming, or refactoring files, do not suggest restructuring dependencies, changing public_deps vs deps, or reorganizing code that was simply carried over from the old location. The author's goal is to preserve existing behavior, not to optimize the code they're moving. Only flag issues that are actual bugs introduced by the move (e.g., broken paths, missing deps that cause build failures), not "while you're here, you should also fix X" improvements.
- Security-sensitive areas (wallet, crypto, sync, credentials) deserve extra scrutiny — type mismatches, truncation, and correctness issues should use stronger language.
- Do NOT flag: existing code the PR isn't changing, template functions defined in headers, simple inline getters in headers, style preferences not in the documented best practices, include/import ordering (this is handled by formatting tools and linters, not this bot).
- Every claim must be verified in the best practices source document. Do NOT make claims based on general knowledge or assumptions about what "should" be a best practice. If the best practices docs do not contain a rule about something, do NOT flag it as a violation — even if you believe it to be true. For example, do NOT claim an API is "deprecated" or a pattern is "banned" unless the best practices doc explicitly says so. Hallucinated rules erode trust and waste developer time. When in doubt, do not comment.
- Comment style: short (1-3 sentences), targeted, acknowledge context. Use "nit:" for genuinely minor/stylistic issues (including missing comments/documentation). Substantive issues (test reliability, correctness, banned APIs) should be direct without "nit:" prefix."""

_BEST_PRACTICE_LINK_REQUIREMENT = """\
Best practice link requirement: each rule in the best practices docs has a stable ID anchor (e.g., <a id="CS-001"></a>) on the line before the heading. For each violation, you MUST include a direct link using that ID. The link format is:
  {bp_link_base}/<doc>.md#<ID>
For example, if the heading has <a id="CS-042"></a> above it, the link is ...coding-standards.md#CS-042.

CRITICAL: The rule_link fragment MUST be an exact <a id="..."> value from the rules provided in your chunk. Look for the <a id="..."></a> tag on the line before the heading you're referencing and use that ID verbatim. Do NOT invent IDs, guess ID numbers, or construct anchors from heading text. If no <a id> tag exists for the rule, or if your observation is a general bug/correctness issue that doesn't map to any specific heading, omit the rule_link field entirely.

CRITICAL: Do NOT invent rules or claim things are deprecated/banned without verification. Every best-practice violation you flag MUST correspond to an actual rule provided in your chunk. If you cannot point to the specific heading in the provided rules that contains the rule, do not flag it. Do not rely on general knowledge about Chromium conventions — only flag what is explicitly documented. A hallucinated rule (e.g., claiming an API is "deprecated" when the rules say nothing about it) erodes developer trust and is worse than no comment at all."""

_PRIOR_COMMENTS_RULES = """\
Prior comments re-review rules:
- Do NOT re-raise issues that the author or a reviewer has already explained or justified. If a prior comment thread shows the author explaining why a design choice was made (e.g., "only two subclasses will ever use this, both pass constants"), accept that explanation and do not flag the same issue again.
- Do NOT repeat your own previous comments. If a comment from the bot already raised the same point, skip it — even if the code hasn't changed. The author has already seen it.
- Do NOT flag new issues on re-review that were missed the first time. If an issue existed in the code during the first review and was not caught, do not raise it on a subsequent review — unless it is a serious correctness or security concern. Only flag issues on re-review if they were introduced in commits since the last reviewed commit.
- DO re-raise an issue only if: (a) the author's explanation is factually incorrect or introduces a real risk, OR (b) new code in the latest diff introduces a new instance of the same problem that wasn't previously discussed.
- When in doubt about whether an issue was addressed, err on the side of NOT re-raising it. Repeating resolved feedback is more disruptive than missing a marginal issue."""

_SYSTEMATIC_AUDIT_REQUIREMENT = """\
Systematic Audit Requirement:
CRITICAL — this is what prevents you from stopping after finding a few violations.

You MUST work through your chunk heading by heading, checking every ## rule against the diff. You must output an audit trail listing EVERY ## heading in the chunk with a verdict:

AUDIT:
PASS: ✅ Always Include What You Use (IWYU)
PASS: ✅ Use Positive Form for Booleans and Methods
N/A: ✅ Consistent Naming Across Layers
FAIL: ❌ Don't Use rapidjson
PASS: ✅ Use CHECK for Impossible Conditions
... (one entry per ## heading in the chunk)

Verdicts:
- PASS: Checked the diff — no violation found
- N/A: Rule doesn't apply to the types of changes in this diff
- FAIL: Violation found — must have a corresponding entry in VIOLATIONS

This forces you to explicitly consider every rule rather than satisficing after a few findings."""

_REQUIRED_OUTPUT_FORMAT = """\
Required Output Format:

You MUST return this structured format:

DOCUMENT: {doc} (chunk {chunk_num}/{total_chunks})
[PR #{pr_number}](https://github.com/{pr_repo}/pull/{pr_number}): {title}

AUDIT:
PASS: <rule heading>
N/A: <rule heading>
FAIL: <rule heading>
... (one line per ## heading in the chunk)

SKIPPED_PRIOR:
- file: <path>, issue: <brief description>, reason: <why not re-raised — e.g., "author explained in prior comment that only constant strings are passed", "already flagged in previous review">
NONE (if no prior issues were skipped)

VIOLATIONS:
- file: <path>, line: <line_number>, severity: <"high"|"medium"|"low">, rule: "<rule heading>", rule_link: <full GitHub URL to the rule heading>, issue: <brief description>, draft_comment: <1-3 sentence comment to post>
- ...
NO_VIOLATIONS (if none found)

CRITICAL: The `line` value MUST be a line number within one of the valid diff line ranges listed above. Comments on lines outside the diff will fail to post. If the violation is on a context line (no + prefix), use the nearest + line in the same hunk instead.

Severity guide:
- high: Correctness bugs, use-after-free, security issues, banned APIs, test reliability problems (e.g., RunUntilIdle)
- medium: Substantive best practice violations (wrong container type, missing error handling, architectural issues)
- low: Nits, style preferences, missing docs, naming suggestions, minor cleanup"""

_VALIDATION_INSTRUCTIONS = """\
Source Code Validation (REQUIRED):

After identifying violations from the diff, you MUST validate each one by reading the actual source code.
The target source tree is at: {target_repo_path}
File paths in violations are relative to this directory.

For each violation:
- Use the Read tool to read the actual source file at {target_repo_path}/<file_path> around the flagged line.
- Read surrounding context — functions, class definitions, includes, namespace scope.
- Verify the claim is true. If the violation says "use X instead of Y", confirm X is available and appropriate.
- If it claims something is missing, verify it's actually missing in the full file, not just absent from the diff.
- Deprecation claims require header verification — read the actual header file to confirm.
- Check surrounding context for justification — comments, TODOs, or patterns that explain the code.
- If surrounding code uses the same pattern being flagged, the violation may be invalid.
- Sanitize @mentions — validate against actual PR participants. Fix or strip hallucinated usernames.
- Drop false positives. If reading the source reveals the violation is incorrect, drop it.
- Log each result:
  - VALIDATED: <file>:<line> — confirmed, <note>
  - VALIDATED_ENHANCED: <file>:<line> — improved with <context>
  - VALIDATED_DROP: <file>:<line> — <reason>

After validation, write your final results as a JSON file to: {results_file}
Use the Write tool to create this file with the following format:
{{
  "violations": [
    {{"file": "path/to/file.cc", "line": 42, "severity": "high", "rule": "Rule heading", "rule_link": "https://...", "issue": "brief description", "draft_comment": "1-3 sentence comment to post"}}
  ],
  "validation_log": ["VALIDATED: file.cc:42 — confirmed", "VALIDATED_DROP: bar.cc:10 — false positive"]
}}

If there are no validated violations, write: {{"violations": [], "validation_log": []}}
CRITICAL: You MUST write the results JSON file even if there are no violations.

CRITICAL: NEVER post reviews, comments, or approvals to GitHub. NEVER use `gh api`, `gh pr review`, `gh pr comment`, or any other command to interact with the GitHub API. Your ONLY job is to analyze the diff against the rules and write the results JSON file. All posting is handled by a separate pipeline script after your results are collected."""
# pylint: enable=line-too-long


def build_subagent_prompt(pr_number,
                          pr_title,
                          diff_text,
                          images,
                          prior_comments,
                          bot_username,
                          chunk,
                          diff_line_ranges=None,
                          results_file=None,
                          target_repo_path=None):
    """Build a complete self-contained subagent prompt for a single chunk."""
    doc = chunk["doc"]
    chunk_index = chunk["chunk_index"]
    total_chunks = chunk["total_chunks"]
    chunk_content = chunk["content"]

    parts = []

    # 0. Current date context (so the model knows the actual year)
    current_date = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    parts.append(f"Today's date is {current_date}.")
    parts.append("")

    # 1. PR number and repo
    parts.append(f"Review PR #{pr_number} in {PR_REPO}.")
    parts.append(f"PR title: {pr_title}")
    parts.append("")

    # 2. The FULL PR diff (shared prefix for cache efficiency)
    parts.append("Here is the PR diff:")
    parts.append("```diff")
    parts.append(diff_text)
    parts.append("```")
    parts.append("")

    # 2b. Valid line ranges for inline comments
    if diff_line_ranges:
        parts.append("## Valid Line Ranges for Inline Comments")
        parts.append("CRITICAL: The `line` field in each violation "
                     "MUST fall within one of these ranges.")
        parts.append("These are the only lines where GitHub "
                     "allows inline review comments.")
        parts.append("If the code you want to flag is not on a "
                     "+ line in the diff, use the nearest + line "
                     "within the same hunk.")
        parts.append("```")
        parts.append(format_diff_line_ranges(diff_line_ranges))
        parts.append("```")
        parts.append("")

    # 3. Image paths (if any)
    if images:
        parts.append("This PR includes screenshots/images. Use the "
                     "Read tool to view each image for visual "
                     "context about what the PR changes:")
        for img in images:
            parts.append(f"- {img['abs_path']} "
                         f"(from: {img['source']}, "
                         f"alt: \"{img['alt']}\")")
        parts.append("")

    # 4. Prior comments context
    if prior_comments:
        parts.append("## Prior Review Comments")
        parts.append("")
        parts.append(f"The bot's GitHub username is "
                     f"`{bot_username}`. Comments from this user "
                     "are the bot's own previous comments.")
        parts.append("")
        parts.append(prior_comments)
        parts.append("")

    # 5. The chunk content (rules) — varies per subagent
    parts.append("Here are the best practice rules to check:")
    parts.append("```markdown")
    parts.append(chunk_content)
    parts.append("```")
    parts.append("")

    # 6. Review rules
    parts.append(_REVIEW_RULES)
    parts.append("")

    # 7. Best practice link requirement
    parts.append(
        _BEST_PRACTICE_LINK_REQUIREMENT.format(bp_link_base=BP_LINK_BASE))
    parts.append("")

    # 8. Prior comments re-review rules
    if prior_comments:
        parts.append(_PRIOR_COMMENTS_RULES)
        parts.append("")

    # 9. Systematic audit requirement
    parts.append(_SYSTEMATIC_AUDIT_REQUIREMENT)
    parts.append("")

    # 10. Required output format
    output_fmt = _REQUIRED_OUTPUT_FORMAT.format(
        doc=doc,
        chunk_num=chunk_index + 1,
        total_chunks=total_chunks,
        pr_number=pr_number,
        pr_repo=PR_REPO,
        title=pr_title,
    )
    parts.append(output_fmt)

    # 11. Validation instructions (subagent validates its own findings)
    if results_file and target_repo_path:
        parts.append("")
        parts.append(
            _VALIDATION_INSTRUCTIONS.format(
                target_repo_path=target_repo_path,
                results_file=results_file,
            ))

    return "\n".join(parts)


# ---------------------------------------------------------------------------
# Process a single PR (for ThreadPoolExecutor)
# ---------------------------------------------------------------------------
def process_pr(pr, bot_username, org_members, work_dir):
    """Process a single PR.

    Fetch diff, classify, comments, images, threads,
    chunks. Writes prompt files to work_dir. Returns a
    dict for the manifest or an error dict.
    """
    pr_number = pr["number"]
    pr_title = pr["title"]
    head_sha = pr["headRefOid"]
    author = pr["author"]
    has_approval = pr.get("hasApproval", False)
    is_external = pr.get("isExternalContributor", False)

    log(f"  Processing PR #{pr_number}: {pr_title}")

    try:
        # a. Fetch diff
        diff_text = fetch_diff(pr_number)
    except Exception as e:
        return None, {
            "pr_number": pr_number,
            "stage": "fetch_diff",
            "error": str(e)
        }

    try:
        # b. Classify files
        file_flags = classify_files(diff_text)
    except Exception as e:
        return None, {
            "pr_number": pr_number,
            "stage": "classify_files",
            "error": str(e)
        }

    try:
        # c. Fetch prior comments
        include_author = author if is_external else None
        prior_comments, has_bot_comments = fetch_prior_comments(
            pr_number, org_members, include_author=include_author)
    except Exception as e:
        prior_comments = None
        has_bot_comments = False
        log(f"  WARNING: prior comments failed for #{pr_number}: {e}")

    try:
        # d. Extract images
        images = extract_images(pr_number)
    except Exception as e:
        images = []
        log(f"  WARNING: image extraction failed for #{pr_number}: {e}")

    try:
        # e. Resolve addressed threads
        thread_resolution = resolve_bot_threads(pr_number, bot_username)
    except Exception as e:
        thread_resolution = {
            "resolved": 0,
            "unresolved_bot_threads": 0,
            "total_bot_threads": 0
        }
        log(f"  WARNING: thread resolution failed for #{pr_number}: {e}")

    try:
        # f. Discover applicable best-practice docs
        applicable_docs = discover_best_practices(file_flags)
    except Exception as e:
        applicable_docs = []
        log(f"  WARNING: discover best practices failed for #{pr_number}: {e}")

    # g. Parse diff line ranges for subagent prompts
    diff_line_ranges = parse_diff_line_ranges(diff_text)

    # h+i. Chunk each doc, build subagent prompts, write to files
    pr_work_dir = os.path.join(work_dir, f"pr_{pr_number}")
    os.makedirs(pr_work_dir, exist_ok=True)

    subagent_prompts = []
    total_prompt_chars = 0
    diff_chars = len(diff_text)
    prior_comments_chars = len(prior_comments) if prior_comments else 0

    for doc_info in applicable_docs:
        try:
            chunks = chunk_doc(doc_info["path"])
            for chunk in chunks:
                chunk_id = f"{doc_info['doc']}_{chunk['chunk_index']}"
                prompt_file = os.path.join(pr_work_dir,
                                           f"{chunk_id}_prompt.txt")
                results_file = os.path.join(pr_work_dir,
                                            f"{chunk_id}_results.json")

                prompt = build_subagent_prompt(
                    pr_number,
                    pr_title,
                    diff_text,
                    images,
                    prior_comments,
                    bot_username,
                    chunk,
                    diff_line_ranges=diff_line_ranges,
                    results_file=results_file,
                    target_repo_path=TARGET_REPO_PATH,
                )

                # Write prompt to file (not embedded in JSON)
                with open(prompt_file, "w") as f:
                    f.write(prompt)

                prompt_chars = len(prompt)
                total_prompt_chars += prompt_chars

                subagent_prompts.append({
                    "chunk_id": chunk_id,
                    "doc": doc_info["doc"],
                    "chunk_index": chunk["chunk_index"],
                    "total_chunks": chunk["total_chunks"],
                    "rule_count": chunk["rule_count"],
                    "headings": chunk["headings"],
                    "prompt_file": prompt_file,
                    "results_file": results_file,
                    "cost_estimate": {
                        "prompt_chars": prompt_chars,
                        "prompt_tokens_approx": prompt_chars // 4,
                    },
                })
        except Exception as e:
            log(f"  WARNING: chunking failed for {doc_info['doc']}: {e}")

    # Lightweight manifest entry — no diff or prior_comments
    pr_result = {
        "number": pr_number,
        "title": pr_title,
        "headRefOid": head_sha,
        "author": author,
        "hasApproval": has_approval,
        "isExternalContributor": is_external,
        "has_bot_comments": has_bot_comments,
        "images": images,
        "thread_resolution": thread_resolution,
        "subagent_prompts": subagent_prompts,
    }

    # Cost logging
    log(f"  COST PR #{pr_number}: diff={diff_chars:,} chars, "
        f"prior_comments={prior_comments_chars:,} chars, "
        f"{len(subagent_prompts)} chunks, "
        f"total_prompt={total_prompt_chars:,} chars "
        f"(~{total_prompt_chars // 4:,} tokens)")

    log(f"  Done PR #{pr_number}: {len(subagent_prompts)} subagent prompts")
    return pr_result, None


# ---------------------------------------------------------------------------
# Process a single cached PR
# ---------------------------------------------------------------------------
def process_cached_pr(pr, bot_username):
    """Process a cached PR: resolve threads, check approval gate."""
    pr_number = pr["number"]
    pr_title = pr["title"]
    head_sha = pr["headRefOid"]

    log(f"  Processing cached PR #{pr_number}: {pr_title}")

    # Resolve threads
    try:
        thread_resolution = resolve_bot_threads(pr_number, bot_username)
    except Exception as e:
        thread_resolution = {
            "resolved": 0,
            "unresolved_bot_threads": 0,
            "total_bot_threads": 0
        }
        log(f"  WARNING: thread resolution failed for cached #{pr_number}: {e}"
            )

    # Check approval gate
    try:
        can_approve = check_can_approve(pr_number, bot_username)
    except Exception as e:
        can_approve = {"result": False, "reason": str(e)}

    # If can approve, actually submit the APPROVE and update cache
    approved = False
    if can_approve["result"]:
        log(f"  Approving cached PR #{pr_number}")
        approved = submit_approve(pr_number, head_sha)
        if approved:
            log(f"  Approved PR #{pr_number}")
        else:
            log(f"  WARNING: Failed to submit APPROVE for #{pr_number}")

    return {
        "number": pr_number,
        "title": pr_title,
        "headRefOid": head_sha,
        "thread_resolution": thread_resolution,
        "can_approve": can_approve,
        "approved": approved,
    }


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    auto_mode, reviewer_priority, max_prs, fetch_args = parse_args()

    # 1. Resolve bot username
    log("Resolving bot username...")
    bot_username = resolve_bot_username()
    log(f"Bot username: {bot_username}")

    # 2. Config already loaded at module level

    # 3. Call fetch-prs logic
    log("Fetching PRs...")

    # Build argv for fetch-prs module's parse_args
    fetch_argv = list(fetch_args)
    if reviewer_priority:
        fetch_argv.extend(["--reviewer-priority", bot_username])
    if max_prs is not None:
        fetch_argv.extend(["--max-prs", str(max_prs)])

    # Temporarily override sys.argv for the fetch module
    old_argv = sys.argv
    sys.argv = ["fetch-prs.py"] + fetch_argv
    mode, days, page, pr_number, state, _rp, _mp = _fp_mod.parse_args()
    sys.argv = old_argv

    raw_prs = _fp_mod.fetch_prs(mode, days, page, pr_number, state)
    org_members = load_org_members()

    if mode == "single":
        to_review = raw_prs
        cached_prs_raw = []
        fetch_summary = {
            "total_fetched": len(raw_prs),
            "to_review": len(raw_prs),
            "cached_with_possible_threads": 0,
            "skipped_filtered": 0,
            "skipped_cached": 0,
            "skipped_approved": 0,
            "skipped_external": 0,
            "skipped_max_prs": 0,
        }
    else:
        cache = _fp_mod.load_cache()
        (
            to_review,
            cached_prs_raw,
            skipped_filtered,
            skipped_cached,
            skipped_approved,
            skipped_external,
        ) = _fp_mod.filter_prs(
            raw_prs,
            mode,
            days,
            cache,
            org_members,
            reviewer_priority=bot_username if reviewer_priority else None,
        )

        # Sort by reviewer priority
        if reviewer_priority:
            to_review.sort(key=lambda p: 0 if _fp_mod.is_requested_reviewer(
                p, bot_username) else 1)
            cached_prs_raw.sort(key=lambda p: 0 if _fp_mod.
                                is_requested_reviewer(p, bot_username) else 1)

        # Apply max-prs limit
        skipped_max_prs = 0
        if max_prs is not None and len(to_review) > max_prs:
            skipped_max_prs = len(to_review) - max_prs
            to_review = to_review[:max_prs]

        fetch_summary = {
            "total_fetched": len(raw_prs),
            "to_review": len(to_review),
            "cached_with_possible_threads": len(cached_prs_raw),
            "skipped_filtered": skipped_filtered,
            "skipped_cached": skipped_cached,
            "skipped_approved": skipped_approved,
            "skipped_external": skipped_external,
            "skipped_max_prs": skipped_max_prs,
        }

    # Build PR entry dicts for processing
    _rp_val = bot_username if reviewer_priority else None

    def pr_entry(pr):
        author = pr.get("author", {}).get("login", "unknown")
        entry = {
            "number": pr["number"],
            "title": pr["title"],
            "headRefOid": pr["headRefOid"],
            "author": author,
            "hasApproval": _fp_mod.has_any_approval(pr),
            "isExternalContributor": bool(org_members
                                          and author not in org_members),
        }
        return entry

    prs_to_process = [pr_entry(p) for p in to_review]
    cached_to_process = [pr_entry(p) for p in cached_prs_raw]

    progress_lines = [
        f"Found {len(prs_to_process)} PRs to review, "
        f"{len(cached_to_process)} cached PRs to check threads.",
    ]
    if fetch_summary["skipped_filtered"]:
        progress_lines.append(
            f"Skipped {fetch_summary['skipped_filtered']} PRs (filtered).")
    if fetch_summary["skipped_approved"]:
        progress_lines.append(f"Skipped {fetch_summary['skipped_approved']}"
                              " PRs (already approved).")
    if fetch_summary["skipped_external"]:
        progress_lines.append(f"Skipped {fetch_summary['skipped_external']}"
                              " PRs (external contributors).")

    log("\n".join(progress_lines))

    # Create work directory for prompt/result files
    work_dir = tempfile.mkdtemp(prefix="review-prs-")
    log(f"Work directory: {work_dir}")

    # 4. Process each PR in parallel
    errors = []
    processed_prs = []

    if prs_to_process:
        log(f"\nProcessing {len(prs_to_process)} PRs in parallel...")
        with ThreadPoolExecutor(max_workers=5) as executor:
            args = (bot_username, org_members, work_dir)
            futures = {
                executor.submit(process_pr, pr, *args): pr
                for pr in prs_to_process
            }
            for future in as_completed(futures):
                pr = futures[future]
                try:
                    result, error = future.result()
                    if error:
                        errors.append(error)
                    if result:
                        processed_prs.append(result)
                except Exception as e:
                    errors.append({
                        "pr_number": pr["number"],
                        "stage": "process_pr",
                        "error": str(e),
                    })

    # Sort processed PRs to match original order
    pr_order = {p["number"]: i for i, p in enumerate(prs_to_process)}
    processed_prs.sort(key=lambda p: pr_order.get(p["number"], 999999))

    # 5. Process cached PRs in parallel
    processed_cached = []
    if cached_to_process:
        log(f"\nProcessing {len(cached_to_process)} cached PRs...")
        with ThreadPoolExecutor(max_workers=5) as executor:
            futures = {
                executor.submit(process_cached_pr, pr, bot_username): pr
                for pr in cached_to_process
            }
            for future in as_completed(futures):
                pr = futures[future]
                try:
                    result = future.result()
                    processed_cached.append(result)
                except Exception as e:
                    errors.append({
                        "pr_number": pr["number"],
                        "stage": "process_cached_pr",
                        "error": str(e),
                    })

    # Sort cached PRs to match original order
    cached_order = {p["number"]: i for i, p in enumerate(cached_to_process)}
    processed_cached.sort(key=lambda p: cached_order.get(p["number"], 999999))

    # 6. Write manifest.json (lightweight — no diffs,
    # no prompts, just file paths)
    output = {
        "bot_username": bot_username,
        "pr_repo": PR_REPO,
        "auto_mode": auto_mode,
        "reviewer_priority": reviewer_priority,
        "fetch_summary": fetch_summary,
        "progress_lines": progress_lines,
        "prs": processed_prs,
        "cached_prs": processed_cached,
        "errors": errors,
    }

    manifest_path = os.path.join(work_dir, "manifest.json")
    with open(manifest_path, "w") as f:
        json.dump(output, f, indent=2)

    total_prompts = sum(
        len(p.get("subagent_prompts", [])) for p in processed_prs)
    total_prompt_chars = sum(
        sp.get("cost_estimate", {}).get("prompt_chars", 0)
        for p in processed_prs for sp in p.get("subagent_prompts", []))
    total_prompt_tokens = total_prompt_chars // 4

    # Cost summary
    log(f"\n{'=' * 60}")
    log("COST SUMMARY")
    log(f"{'=' * 60}")
    log(f"PRs to review: {len(processed_prs)}")
    log(f"Total subagent prompts: {total_prompts}")
    log(f"Total prompt size: {total_prompt_chars:,} "
        f"chars (~{total_prompt_tokens:,} tokens)")
    if total_prompts > 0:
        avg_chars = total_prompt_chars // total_prompts
        log(f"Average prompt size: {avg_chars:,} "
            f"chars (~{avg_chars // 4:,} tokens)")
    log(f"Cached PRs processed: {len(processed_cached)}")
    log(f"Errors: {len(errors)}")
    # Per-PR breakdown
    for pr in processed_prs:
        pr_chars = sum(
            sp.get("cost_estimate", {}).get("prompt_chars", 0)
            for sp in pr.get("subagent_prompts", []))
        chunks = len(pr.get('subagent_prompts', []))
        log(f"  PR #{pr['number']}: {chunks} chunks, "
            f"{pr_chars:,} chars (~{pr_chars // 4:,} tokens)")
    log(f"{'=' * 60}")

    log(f"\nDone. {len(processed_prs)} PRs processed, "
        f"{total_prompts} total subagent prompts, "
        f"{len(processed_cached)} cached PRs processed, "
        f"{len(errors)} errors.")

    # Output just the work_dir path to stdout (tiny — the LLM only needs this)
    print(json.dumps({"work_dir": work_dir, "manifest": manifest_path}))


if __name__ == "__main__":
    main()
