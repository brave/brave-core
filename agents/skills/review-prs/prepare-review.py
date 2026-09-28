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
        [--reviewer-priority] [--max-prs N] [--full]

--full reviews every file in each PR's diff, not only the ones that changed
since the bot last reviewed it.
"""

import functools
import hashlib
import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
# Repo root: agents/skills/review-prs -> agents/skills -> agents -> repo root
_REPO_DIR = os.path.normpath(os.path.join(_SCRIPT_DIR, "..", "..", ".."))

sys.path.insert(0, os.path.join(_SCRIPT_DIR, "scripts"))
sys.path.insert(0, _SCRIPT_DIR)

from lib.repo_lock import repo_lock

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
UPDATE_CACHE = os.path.join(_SCRIPT_DIR, "update-cache.py")
# Reads PR sources from this tree instead of a worktree at the PR head. For the
# eval harness, whose fake PR head cannot be fetched.
SOURCE_PATH_OVERRIDE = os.environ.get("REVIEW_PRS_SOURCE_PATH")


def log(msg):
    print(msg, file=sys.stderr)


def work_dir_base():
    """Directory the per-run work directory is created under.

    Each PR gets a full worktree, which can overfill a tmpfs /tmp.
    """
    base = os.environ.get("REVIEW_PRS_WORK_DIR") or "/var/tmp/review-prs"
    os.makedirs(base, exist_ok=True)
    return base


# Older than any run can last, so a dir this old lost its cleanup.
STALE_WORK_DIR_AGE_S = 24 * 60 * 60


def prune_stale_work_dirs():
    """Remove work dirs and worktrees left behind by runs that died.

    Age-gated so a concurrent run's directory is never touched.
    """
    base = work_dir_base()
    cutoff = time.time() - STALE_WORK_DIR_AGE_S
    removed = 0
    for name in os.listdir(base):
        if not name.startswith("review-prs-"):
            continue
        path = os.path.join(base, name)
        try:
            if os.path.getmtime(path) > cutoff:
                continue
            shutil.rmtree(path, ignore_errors=True)
            removed += 1
        except OSError as e:
            log(f"  WARNING: could not remove stale work dir {path}: {e}")

    subprocess.run(
        ["git", "-C", TARGET_REPO_PATH, "worktree", "prune"],
        capture_output=True,
        text=True,
        timeout=120,
        check=False,
    )
    if removed:
        log(f"Pruned {removed} stale work "
            f"director{'y' if removed == 1 else 'ies'}.")


# Cross-process, so overlapping runs don't collide on git's ref locks.
def _git_fetch_lock():
    return repo_lock(TARGET_REPO_PATH)


# A brave-core checkout is 63k files and five run at once; this guards against
# a wedged git, it is not a time budget.
WORKTREE_ADD_TIMEOUT_S = 900

# Measured from acquiring the lock, which every PR fetch shares.
PR_HEAD_FETCH_TIMEOUT_S = 180


@functools.lru_cache(maxsize=1)
def pr_remote():
    """Name of the remote PR_REPO is fetched from.

    refs/pull/*/head only exist there. Push URLs are ignored: a fork remote
    can push to PR_REPO while fetching from the fork.
    """
    result = subprocess.run(
        ["git", "-C", TARGET_REPO_PATH, "remote", "-v"],
        capture_output=True,
        text=True,
        timeout=30,
        check=False,
    )
    if result.returncode == 0:
        pattern = re.compile(rf"[:/]{re.escape(PR_REPO)}(\.git)?$")
        for line in result.stdout.splitlines():
            parts = line.split()
            if (len(parts) >= 3 and parts[2] == "(fetch)"
                    and pattern.search(parts[1])):
                return parts[0]
    log(f"  WARNING: no remote fetches from {PR_REPO}; falling back to "
        "'origin'. PR head fetches will likely fail.")
    return "origin"


def fetch_and_create_worktree(pr_number, head_sha, worktree_path):
    """Fetch the PR head and create a detached worktree at it.

    Returns worktree_path on success, None on failure.
    """
    with _git_fetch_lock():
        try:
            result = subprocess.run(
                [
                    "git",
                    "-C",
                    TARGET_REPO_PATH,
                    "fetch",
                    pr_remote(),
                    f"pull/{pr_number}/head",
                    "--no-tags",
                ],
                capture_output=True,
                text=True,
                timeout=PR_HEAD_FETCH_TIMEOUT_S,
                check=False,
            )
        except subprocess.TimeoutExpired:
            log(f"  WARNING: fetch for PR #{pr_number} timed out after "
                f"{PR_HEAD_FETCH_TIMEOUT_S}s")
            return None
        if result.returncode != 0:
            log(f"  WARNING: fetch for PR #{pr_number} failed: "
                f"{result.stderr.strip()}")
            return None

    try:
        result = subprocess.run(
            [
                "git",
                "-C",
                TARGET_REPO_PATH,
                "worktree",
                "add",
                worktree_path,
                "--detach",
                head_sha,
            ],
            capture_output=True,
            text=True,
            timeout=WORKTREE_ADD_TIMEOUT_S,
            check=False,
        )
    except subprocess.TimeoutExpired:
        log(f"  WARNING: worktree add for PR #{pr_number} timed out after "
            f"{WORKTREE_ADD_TIMEOUT_S}s")
        discard_partial_worktree(worktree_path)
        return None
    if result.returncode != 0:
        log(f"  WARNING: worktree add for PR #{pr_number} failed: "
            f"{result.stderr.strip()}")
        discard_partial_worktree(worktree_path)
        return None

    return worktree_path


def discard_partial_worktree(worktree_path):
    """Drop a worktree whose creation was killed or failed partway.

    No `worktree prune` here: other threads are adding worktrees concurrently.
    """
    try:
        subprocess.run(
            [
                "git",
                "-C",
                TARGET_REPO_PATH,
                "worktree",
                "remove",
                "--force",
                worktree_path,
            ],
            capture_output=True,
            text=True,
            timeout=300,
            check=False,
        )
    except subprocess.TimeoutExpired:
        log(f"  WARNING: could not unregister worktree {worktree_path}")
    shutil.rmtree(worktree_path, ignore_errors=True)


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
    full = False
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
        elif arg == "--full":
            full = True
        elif arg == "--max-prs" and i + 1 < len(args):
            max_prs = int(args[i + 1])
            i += 1
        else:
            fetch_args.append(arg)
        i += 1

    return auto_mode, reviewer_priority, full, max_prs, fetch_args


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
def is_feature_branch(base_ref):
    """Return True if base_ref is a non-default, non-version feature branch."""
    if not base_ref:
        return False
    if base_ref == DEFAULT_BRANCH:
        return False
    if re.match(_fp_mod.VERSION_BRANCH_RE, base_ref):
        return False
    return True


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
            m = re.search(r"\+(\d+)(?:,(\d+))?", line)
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


# ---------------------------------------------------------------------------
# Per-file diff sections
# ---------------------------------------------------------------------------
def split_diff(diff_text):
    """Split a unified diff into {path: section}, in diff order."""
    sections = {}
    path = None
    lines = []
    for line in diff_text.split("\n"):
        if line.startswith("diff --git "):
            if path is not None:
                sections[path] = "\n".join(lines).rstrip("\n")
            m = re.search(r" b/(.+)$", line)
            path = m.group(1) if m else line
            lines = [line]
        elif path is not None:
            lines.append(line)
    if path is not None:
        sections[path] = "\n".join(lines).rstrip("\n")
    return sections


def join_sections(sections):
    sections = list(sections)
    return "\n".join(sections) + "\n" if sections else ""


def section_hash(section):
    """Hash of one file's change, ignoring blob ids and hunk offsets."""
    kept = []
    for line in section.split("\n"):
        if line.startswith("index "):
            continue
        kept.append("@@" if line.startswith("@@") else line)
    return hashlib.sha256("\n".join(kept).encode()).hexdigest()[:16]


# No rule reads these, yet every subagent paid for them; each becomes one stub
# line.
_LOCKFILES = frozenset({
    "package-lock.json",
    "npm-shrinkwrap.json",
    "yarn.lock",
    "pnpm-lock.yaml",
    "cargo.lock",
    "go.sum",
    "gemfile.lock",
    "poetry.lock",
    "podfile.lock",
    "composer.lock",
    "uv.lock",
    "package.resolved",
})
_OMITTED_SUFFIXES = (
    ".png",
    ".jpg",
    ".jpeg",
    ".gif",
    ".webp",
    ".avif",
    ".bmp",
    ".ico",
    ".tiff",
    ".pdf",
    ".woff",
    ".woff2",
    ".ttf",
    ".otf",
    ".eot",
    ".mp3",
    ".mp4",
    ".mov",
    ".wav",
    ".ogg",
    ".webm",
    ".zip",
    ".gz",
    ".tgz",
    ".svg",
    ".icon",
    ".xtb",
    ".min.js",
    ".map",
)


def omitted_reason(path, section):
    """Why a file's diff is left out of the prompts, or None to include it."""
    fl = path.lower()
    if os.path.basename(fl) in _LOCKFILES:
        return "lockfile"
    if re.search(r"^(Binary files |GIT binary patch)", section, re.MULTILINE):
        return "binary"
    if fl.endswith(_OMITTED_SUFFIXES):
        return "asset"
    if re.search(r"/res/drawable[^/]*/[^/]+\.xml$", fl):
        return "drawable"
    return None


def section_stub(path, section, reason):
    """One line standing in for a file whose diff is not shown."""
    added = removed = 0
    for line in section.split("\n"):
        if line.startswith("+") and not line.startswith("+++"):
            added += 1
        elif line.startswith("-") and not line.startswith("---"):
            removed += 1
    if re.search(r"^new file mode", section, re.MULTILINE):
        kind = "new file"
    elif re.search(r"^deleted file mode", section, re.MULTILINE):
        kind = "deleted file"
    else:
        kind = "modified"
    return (f"{path} ({kind}, +{added}/-{removed} lines, {reason}; "
            "content not shown)")


# ---------------------------------------------------------------------------
# File classification
# ---------------------------------------------------------------------------
def _is_cpp(path):
    return path.lower().endswith((".cc", ".h", ".mm"))


def _is_test(path):
    fl = path.lower()
    return (fl.endswith((
        "_test.cc",
        "_browsertest.cc",
        "_unittest.cc",
        ".test.ts",
        ".test.tsx",
        ".filter",
    )) or "test/filters/" in fl)


def _is_chromium_src(path):
    return "chromium_src/" in path


def _is_build(path):
    fl = path.lower()
    return os.path.basename(fl) in ("build.gn", "deps") or fl.endswith(".gni")


def _is_frontend(path):
    return path.lower().endswith(
        (".ts", ".tsx", ".js", ".jsx", ".mjs", ".html", ".css"))


def _is_android(path):
    return path.lower().endswith((".java", ".kt")) or "android/" in path


def _is_ios(path):
    return path.lower().endswith(".swift") or "ios/" in path


def _is_patch(path):
    return path.lower().endswith(".patch") or "patches/" in path


def _is_nala(path):
    return (re.search(r"/res/drawable/", path) is not None
            or re.search(r"/res/values/", path) is not None
            or re.search(r"/res/values-night/", path) is not None
            or "components/vector_icons/" in path or path.lower().endswith(
                (".icon", ".svg")))


def _is_localization(path):
    fl = path.lower()
    return (fl.endswith((".grd", ".grdp", ".xtb")) or "l10n/" in path
            or "strings/" in path)


# The conditions discover-best-practices.py tags a document with.
CONDITION_PREDICATES = {
    "has_cpp_files": _is_cpp,
    "has_test_files": _is_test,
    "has_chromium_src": _is_chromium_src,
    "has_build_files": _is_build,
    "has_frontend_files": _is_frontend,
    "has_android_files": _is_android,
    "has_ios_files": _is_ios,
    "has_patch_files": _is_patch,
    "has_nala_files": _is_nala,
    "has_localization_files": _is_localization,
}


def classify_files(files):
    return {
        condition: any(pred(f) for f in files)
        for condition, pred in CONDITION_PREDICATES.items()
    }


# A family's doc skips other families' source but keeps neutral files
# (BUILD.gn, .grd).
_FAMILY_PREDICATES = {
    "has_cpp_files": lambda f: f.lower().endswith(
        (".cc", ".h", ".mm", ".c", ".cpp", ".mojom")),
    "has_android_files": _is_android,
    "has_ios_files": _is_ios,
    "has_frontend_files": _is_frontend,
}


def files_in_scope(condition, paths):
    """The files in `paths` a doc tagged with `condition` is checked against."""
    own = _FAMILY_PREDICATES.get(condition)
    if own is None:
        return list(paths)
    return [
        p for p in paths
        if own(p) or not any(pred(p) for pred in _FAMILY_PREDICATES.values())
    ]


def docs_for_flags(docs, file_flags):
    """Drop docs whose condition no changed file meets.

    Discovery returns every doc when no condition is met.
    """
    return [d for d in docs if file_flags.get(d.get("condition"), True)]


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
        log(f"WARNING: resolve-bot-threads failed for #{pr_number}: "
            f"{result.stderr.strip()}")
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
            "gh",
            "api",
            f"repos/{PR_REPO}/pulls/{pr_number}/reviews",
            "--method",
            "POST",
            "--input",
            "-",
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
        log(f"WARNING: extract-pr-images failed for #{pr_number}: "
            f"{result.stderr.strip()}")
        return []
    try:
        data = json.loads(result.stdout)
        return [{
            "abs_path": img.get("abs_path", img.get("path", "")),
            "source": img.get("source", ""),
            "alt": img.get("alt", ""),
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
# Subagent prompts: detect reads the diff only; one validator per PR reads the
# source
# ---------------------------------------------------------------------------

# pylint: disable=line-too-long
_DETECT_RULES = """\
Review Rules:
- You work from the diff. A validator reads the source tree afterwards and drops what the source disproves, so report what the diff shows. When a finding depends on code the diff does not show (a deps list, an include elsewhere in the file, a caller, an upstream class), say what to check in `issue`.
- Only flag violations in ADDED lines (+ lines), not existing code.
- Only flag violations of the rules below. A bug that breaks none of them is another reviewer's job.
- Only comment on things the PR author introduced. If a dependency, pattern, or architectural issue already existed before this PR, do not flag it — even if it violates a best practice. Do NOT claim a PR "adds a dependency" or "introduces a pattern" unless the + lines show it.
- Do not suggest renaming imported symbols defined outside the PR. When a + line imports or calls a function/class/variable whose definition is NOT in a file changed by the PR, do not comment on its name. Only flag naming issues on symbols defined or renamed within the PR's changed files.
- Respect the intent of the PR. If a PR is moving, renaming, or refactoring files, do not suggest restructuring dependencies, changing public_deps vs deps, or reorganizing code that was simply carried over from the old location. Only flag issues that are actual bugs introduced by the move (e.g., broken paths, missing deps that cause build failures), not "while you're here, you should also fix X" improvements.
- Security-sensitive areas (wallet, crypto, sync, credentials) deserve extra scrutiny — type mismatches, truncation, and correctness issues should use stronger language.
- If you notice a violation in surrounding context lines (lines without + prefix) or in unchanged code visible in the diff, do NOT comment on it -- unless the changes directly affect or break that surrounding code.
- Do NOT flag: existing code the PR isn't changing, template functions defined in headers, simple inline getters in headers, style preferences not in the documented best practices, include/import ordering (formatting tools and linters handle it).
- Every claim must be backed by a rule below. Do NOT make claims based on general knowledge about what "should" be a best practice, and do NOT claim an API is "deprecated" or a pattern is "banned" unless a rule below says so. Hallucinated rules erode trust and waste developer time. When in doubt, do not comment.
- A claim about what upstream Chromium code does must name the upstream file in `issue`, so the validator can read it. Upstream behavior claims are a common hallucination vector.
- Comment style: short (1-3 sentences), targeted, acknowledge context. Use "nit:" for genuinely minor/stylistic issues (including missing comments/documentation). Substantive issues (test reliability, correctness, banned APIs) should be direct without "nit:" prefix."""

_CORRECTNESS_RULES = """\
Your job: find bugs this change introduces. For example a wrong or inverted condition, an off-by-one, a null or dangling pointer, a use after move, a missing return, a string concatenation missing its separator, a duplicate DEPS entry, code inside the wrong #if guard, a path a file move broke, an observer never removed, a race on shared state.
- Only report bugs in ADDED lines (+ lines), or bugs the added lines cause in the code around them.
- Only report what would misbehave at runtime or break the build. Style, naming, missing comments and best practices are other reviewers' job: do not report them.
- Every finding has severity "high" and no rule_link. Set `rule` to a short name for the bug, such as "Use after move".
- You work from the diff. A validator reads the source tree afterwards and drops what the source disproves, so report what the diff shows. When a finding depends on code the diff does not show, say what to check in `issue`.
- When in doubt, leave it out. A wrong bug report costs the author more time than a missed one costs the bot.
- Comment style: short (1-3 sentences), direct."""

_BEST_PRACTICE_LINK_REQUIREMENT = """\
Best practice link requirement: each rule has a stable ID anchor (e.g., <a id="CS-001"></a>) on the line before its heading. Every violation MUST carry a direct link using that ID:
  {bp_link_base}/<doc>.md#<ID>
For example, if the heading has <a id="CS-042"></a> above it, the link is ...coding-standards.md#CS-042.

CRITICAL: The rule_link fragment MUST be an exact <a id="..."> value from the rules below, copied verbatim. Do NOT invent IDs, guess ID numbers, or construct anchors from heading text. A finding you cannot link to a rule below is not a finding for this review: leave it out."""

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
Work through the rules heading by heading, checking every ## rule against the diff, and put one entry per ## heading in `audit`:
- "PASS: <heading>" — checked the diff, no violation found
- "N/A: <heading>" — the rule doesn't apply to the types of changes in this diff
- "FAIL: <heading>" — violation found; it must have an entry in `violations`"""

_SEVERITY_GUIDE = """\
Severity guide:
- high: Correctness bugs, use-after-free, security issues, banned APIs, test reliability problems (e.g., RunUntilIdle)
- medium: Substantive best practice violations (wrong container type, missing error handling, architectural issues)
- low: Nits, style preferences, missing docs, naming suggestions, minor cleanup"""

_NEVER_POST = (
    "Never post reviews, comments, or approvals to GitHub. All posting is done "
    "by a script after you finish; a review you post yourself is a duplicate.")


def _detect_output(candidates_file, chunk_id, rules):
    audit = (
        '  "audit": ["PASS: <rule heading>", "N/A: <rule heading>", "FAIL: <rule heading>"],\n'
        if rules else '  "audit": [],\n')
    link = ', "rule_link": "<full URL>"' if rules else ""
    rule = "<rule heading>" if rules else "<short name of the bug>"
    severity = "high|medium|low" if rules else "high"
    return f"""\
Output:
Write your findings with the Write tool to: {candidates_file}
{{
{audit}  "skipped_prior": [{{"file": "<path>", "issue": "<brief description>", "reason": "<why not re-raised>"}}],
  "violations": [
    {{"file": "<path>", "line": <number>, "severity": "{severity}", "rule": "{rule}"{link}, "issue": "<brief description>", "draft_comment": "<1-3 sentence comment to post>"}}
  ]
}}
Write the file even when every list is empty.

CRITICAL: `line` MUST fall within one of the valid line ranges above; a comment outside them fails to post. If the code you flag is on a context line, use the nearest + line in the same hunk.

{_SEVERITY_GUIDE}

{_NEVER_POST} When the file is written, reply with one line and nothing else: `{chunk_id}: <number of violations> candidates`."""


# pylint: enable=line-too-long


def _prompt_header(ctx, base_note=True):
    parts = [
        f"Today's date is {datetime.now(timezone.utc).strftime('%Y-%m-%d')}.",
        "",
        f"PR #{ctx['number']} in {PR_REPO}: {ctx['title']}",
    ]
    base_ref = ctx.get("base_ref")
    if base_note and base_ref and is_feature_branch(base_ref):
        parts.append(
            f"This PR targets `{base_ref}`, not `{DEFAULT_BRANCH}`. Code "
            f"that `{base_ref}` added is in neither this diff nor the source "
            "tree, so never claim a symbol, file, include or dependency is "
            "missing.")
    if ctx.get("has_approval"):
        parts.append(
            "The PR is already approved: report only high-severity findings.")
    if ctx.get("rereview_note"):
        parts.append(ctx["rereview_note"])
    parts.append("")
    return parts


def _diff_parts(diff_text, stubs, ranges):
    parts = []
    if diff_text:
        parts += [
            "Here is the PR diff:", "```diff",
            diff_text.rstrip("\n"), "```", ""
        ]
    if stubs:
        parts.append(
            "These files changed too. Their content is left out because no "
            "rule reads it; what matters is that they changed:")
        parts += [f"- {s}" for s in stubs]
        parts.append("")
    if ranges:
        parts += [
            "## Valid Line Ranges for Inline Comments",
            "These are the only lines where GitHub allows inline review "
            "comments.",
            "```",
            format_diff_line_ranges(ranges),
            "```",
            "",
        ]
    return parts


def _prior_parts(ctx):
    if not ctx.get("prior_comments"):
        return []
    return [
        "## Prior Review Comments",
        "",
        f"The bot's GitHub username is `{ctx['bot_username']}`. Comments from "
        "this user are the bot's own previous comments.",
        "",
        ctx["prior_comments"],
        "",
    ]


def build_detect_prompt(ctx, chunk, diff_text, stubs, ranges, candidates_file,
                        chunk_id):
    """The prompt for one rule chunk: which rules this diff breaks."""
    parts = _prompt_header(ctx)
    parts += _diff_parts(diff_text, stubs, ranges)
    parts += _prior_parts(ctx)
    parts += [
        f"Here are the best practice rules to check ({chunk['doc']}, chunk "
        f"{chunk['chunk_index'] + 1} of {chunk['total_chunks']}):",
        "```markdown",
        chunk["content"],
        "```",
        "",
        _DETECT_RULES,
        "",
        _BEST_PRACTICE_LINK_REQUIREMENT.format(bp_link_base=BP_LINK_BASE),
        "",
    ]
    if ctx.get("prior_comments"):
        parts += [_PRIOR_COMMENTS_RULES, ""]
    parts += [_SYSTEMATIC_AUDIT_REQUIREMENT, ""]
    parts.append(_detect_output(candidates_file, chunk_id, rules=True))
    return "\n".join(parts)


def build_correctness_prompt(ctx, diff_text, ranges, candidates_file,
                             chunk_id):
    """The prompt for the bugs no rule names."""
    parts = _prompt_header(ctx)
    parts += _diff_parts(diff_text, [], ranges)
    parts += _prior_parts(ctx)
    parts += [_CORRECTNESS_RULES, ""]
    if ctx.get("prior_comments"):
        parts += [_PRIOR_COMMENTS_RULES, ""]
    parts.append(_detect_output(candidates_file, chunk_id, rules=False))
    return "\n".join(parts)


# pylint: disable=line-too-long
_VALIDATE_INSTRUCTIONS = """\
Validation:
The source tree at the PR head is at: {source_path}
File paths are relative to it.{base_note}

Reviewers who read only the diff proposed the candidates above. For each one:
- Read the source file around the flagged line with the Read tool, and the context the claim rests on: the enclosing function and class, the includes, the namespace, the BUILD.gn deps list when the candidate is about dependencies.
- Verify the claim. If it says "use X instead of Y", confirm X exists and fits. If it says something is missing, confirm it is missing from the whole file, not just the diff. A deprecation claim needs the header read.
- Drop it if the PR did not introduce it: the flagged code is on a context line, or the dependency or pattern already existed before this PR.
- Drop a rule candidate the cited rule text does not support, a naming suggestion for a symbol defined outside the PR's changed files, and a "while you're here" suggestion on code a move or rename carried over unchanged.
- A claim about what upstream Chromium code does needs the upstream file read. If you cannot find and read it, drop the candidate.
- A candidate with no rule_link is a bug report: keep it only if the bug is real, the change introduces it, and it would misbehave at runtime or break the build.
- Check the surrounding code for a justification — a comment, a TODO, or the same pattern used nearby.
- Sanitize @mentions: keep only logins of actual PR participants.
- Keep what survives, tightening draft_comment where the source gives better context. Do not add findings of your own. Keep file, line, severity, rule and rule_link unless the source shows they are wrong, and keep `line` within the valid line ranges.
- Log each candidate:
  - VALIDATED: <file>:<line> — confirmed, <note>
  - VALIDATED_ENHANCED: <file>:<line> — improved with <context>
  - VALIDATED_DROP: <file>:<line> — <reason>

Write the results with the Write tool to: {results_file}
{{
  "violations": [
    {{"file": "path/to/file.cc", "line": 42, "severity": "high", "rule": "Rule heading", "rule_link": "https://...", "issue": "brief description", "draft_comment": "1-3 sentence comment to post"}}
  ],
  "validation_log": ["VALIDATED: file.cc:42 — confirmed", "VALIDATED_DROP: bar.cc:10 — false positive"]
}}
Write the file even when every candidate is dropped.

{never_post} {gh_note}When the file is written, reply with one line and nothing else: `PR #{pr_number}: <kept> of <total> candidates kept`."""
# pylint: enable=line-too-long


def build_validate_prompt(ctx, candidates, cited_rules, diff_text, ranges,
                          images, source_path, results_file):
    """The prompt for one PR's validator: which candidates the source holds."""
    parts = _prompt_header(ctx, base_note=False)
    parts += [
        "## Candidates",
        "```json",
        json.dumps(candidates, indent=2),
        "```",
        "",
    ]
    if cited_rules:
        parts.append("## Rules the candidates cite")
        parts.append("")
        for link, text in cited_rules:
            parts += [
                f"### {link}", "```markdown",
                text.strip("\n"), "```", ""
            ]
    parts += _diff_parts(diff_text, [], ranges)
    if images:
        parts.append(
            "This PR includes screenshots/images. Read the ones that bear on a "
            "candidate for visual context:")
        for img in images:
            parts.append(f'- {img["abs_path"]} (from: {img["source"]}, '
                         f'alt: "{img["alt"]}")')
        parts.append("")
    parts += _prior_parts(ctx)
    if ctx.get("prior_comments"):
        parts += [_PRIOR_COMMENTS_RULES, ""]
    base_ref = ctx.get("base_ref")
    if base_ref and is_feature_branch(base_ref):
        lookup = f"gh api repos/{PR_REPO}/contents/<path>?ref={base_ref}"
        base_note = (
            f"\nThis PR targets `{base_ref}`, not `{DEFAULT_BRANCH}`; the diff "
            f"does not show what `{base_ref}` added. Before calling something "
            f"missing, look it up with `{lookup}`; if it is there, drop the "
            "candidate.")
        gh_note = f"The only `gh` command you may run is `{lookup}`. "
    else:
        base_note = ""
        gh_note = "Do not run `gh` at all. "
    parts.append(
        _VALIDATE_INSTRUCTIONS.format(
            source_path=source_path,
            base_note=base_note,
            results_file=results_file,
            never_post=_NEVER_POST,
            gh_note=gh_note,
            pr_number=ctx["number"],
        ))
    return "\n".join(parts)


# ---------------------------------------------------------------------------
# Process a single PR (for ThreadPoolExecutor)
# ---------------------------------------------------------------------------
def _prompt_entry(kind,
                  chunk_id,
                  prompt_file,
                  results_file,
                  prompt,
                  chunk=None):
    entry = {
        "kind": kind,
        "chunk_id": chunk_id,
        "prompt_file": prompt_file,
        "results_file": results_file,
        "cost_estimate": {
            "prompt_chars": len(prompt),
            "prompt_tokens_approx": len(prompt) // 4,
        },
    }
    if chunk is not None:
        entry.update({
            "doc": chunk["doc"],
            "chunk_index": chunk["chunk_index"],
            "total_chunks": chunk["total_chunks"],
            "rule_count": chunk["rule_count"],
            "headings": chunk["headings"],
        })
    return entry


def _scoped_diff(paths, sections, omitted):
    """The diff text, stub lines and line ranges for one set of files."""
    shown = [sections[p] for p in paths if not omitted.get(p)]
    stubs = [
        section_stub(p, sections[p], omitted[p]) for p in paths
        if omitted.get(p)
    ]
    diff_text = join_sections(shown)
    return diff_text, stubs, parse_diff_line_ranges(diff_text)


def process_pr(pr,
               bot_username,
               org_members,
               work_dir,
               auto_mode=False,
               prior_hashes=None):
    """Write the detect prompts for one PR. Returns (manifest entry, error)."""
    pr_number = pr["number"]
    pr_title = pr["title"]
    head_sha = pr["headRefOid"]
    author = pr["author"]
    base_ref = pr.get("baseRefName", "")
    has_approval = pr.get("hasApproval", False)
    is_external = pr.get("isExternalContributor", False)

    log(f"  Processing PR #{pr_number}: {pr_title}")

    try:
        diff_text = fetch_diff(pr_number)
    except Exception as e:
        return None, {
            "pr_number": pr_number,
            "stage": "fetch_diff",
            "error": str(e)
        }

    sections = split_diff(diff_text)
    hashes = {path: section_hash(s) for path, s in sections.items()}
    pr_work_dir = os.path.join(work_dir, f"pr_{pr_number}")
    os.makedirs(pr_work_dir, exist_ok=True)
    hashes_file = os.path.join(pr_work_dir, "file_hashes.json")
    with open(hashes_file, "w") as f:
        json.dump(hashes, f, indent=2, sort_keys=True)

    if prior_hashes:
        changed = [p for p in sections if prior_hashes.get(p) != hashes[p]]
    else:
        changed = list(sections)
    if prior_hashes and not changed:
        log(f"  PR #{pr_number}: no file changed since the last review")
        return {
            "unchanged": True,
            "number": pr_number,
            "title": pr_title,
            "headRefOid": head_sha,
            "file_hashes_file": hashes_file,
        }, None

    rereview_note = None
    if prior_hashes:
        rereview_note = (
            f"The bot reviewed this PR before. Only the {len(changed)} files "
            f"whose changes differ since then are shown; the other "
            f"{len(sections) - len(changed)} were reviewed already, so say "
            "nothing about them.")
        log(f"  PR #{pr_number}: re-review of {len(changed)} of "
            f"{len(sections)} files")

    omitted = {p: omitted_reason(p, sections[p]) for p in changed}
    file_flags = classify_files(changed)

    try:
        include_author = author if is_external else None
        prior_comments, has_bot_comments = fetch_prior_comments(
            pr_number, org_members, include_author=include_author)
    except Exception as e:
        prior_comments = None
        has_bot_comments = False
        log(f"  WARNING: prior comments failed for #{pr_number}: {e}")

    try:
        images = extract_images(pr_number)
    except Exception as e:
        images = []
        log(f"  WARNING: image extraction failed for #{pr_number}: {e}")

    try:
        thread_resolution = resolve_bot_threads(pr_number, bot_username)
    except Exception as e:
        thread_resolution = {
            "resolved": 0,
            "unresolved_bot_threads": 0,
            "total_bot_threads": 0,
        }
        log(f"  WARNING: thread resolution failed for #{pr_number}: {e}")

    try:
        applicable_docs = docs_for_flags(discover_best_practices(file_flags),
                                         file_flags)
    except Exception as e:
        applicable_docs = []
        log(f"  WARNING: discover best practices failed for #{pr_number}: {e}")

    # At the PR head, so the validator's line numbers agree with the diff.
    worktree_path = None
    if SOURCE_PATH_OVERRIDE:
        log(f"  Reading PR #{pr_number} sources from {SOURCE_PATH_OVERRIDE}")
    else:
        worktree_path = fetch_and_create_worktree(
            pr_number, head_sha, os.path.join(pr_work_dir, "source"))
    if worktree_path:
        log(f"  Worktree created for PR #{pr_number}: {worktree_path}")
    elif SOURCE_PATH_OVERRIDE:
        pass
    elif auto_mode:
        # The fallback checkout is not at the PR head, and in auto mode nobody
        # checks the findings before they post.
        return None, {
            "pr_number": pr_number,
            "stage": "worktree",
            "error": ("no worktree at the PR head, so the review would read "
                      f"{DEFAULT_BRANCH} instead; refusing in auto mode"),
        }
    else:
        log(f"  WARNING: worktree unavailable for PR #{pr_number}, "
            f"falling back to {TARGET_REPO_PATH}")

    diff_file = os.path.join(pr_work_dir, "diff.patch")
    with open(diff_file, "w") as f:
        f.write(join_sections(sections[p] for p in changed))
    prior_comments_file = os.path.join(pr_work_dir, "prior_comments.md")
    with open(prior_comments_file, "w") as f:
        f.write(prior_comments or "")

    ctx = {
        "number": pr_number,
        "title": pr_title,
        "base_ref": base_ref,
        "has_approval": has_approval,
        "rereview_note": rereview_note,
        "prior_comments": prior_comments,
        "bot_username": bot_username,
    }

    subagent_prompts = []

    def write_prompt(chunk_id, build):
        prompt_file = os.path.join(pr_work_dir, f"{chunk_id}_prompt.txt")
        results_file = os.path.join(pr_work_dir, f"{chunk_id}_candidates.json")
        prompt = build(results_file)
        with open(prompt_file, "w") as f:
            f.write(prompt)
        return prompt_file, results_file, prompt

    for doc_info in applicable_docs:
        scope = files_in_scope(doc_info.get("condition"), changed)
        if not scope:
            continue
        doc_diff, stubs, ranges = _scoped_diff(scope, sections, omitted)
        try:
            chunks = chunk_doc(doc_info["path"])
        except Exception as e:
            log(f"  WARNING: chunking failed for {doc_info['doc']}: {e}")
            continue
        for chunk in chunks:
            chunk_id = f"{doc_info['doc']}_{chunk['chunk_index']}"
            prompt_file, results_file, prompt = write_prompt(
                chunk_id,
                lambda rf, chunk=chunk, chunk_id=chunk_id: build_detect_prompt(
                    ctx, chunk, doc_diff, stubs, ranges, rf, chunk_id),
            )
            subagent_prompts.append(
                _prompt_entry("rules", chunk_id, prompt_file, results_file,
                              prompt, chunk))

    code_diff, _, code_ranges = _scoped_diff(changed, sections, omitted)
    if code_diff:
        prompt_file, results_file, prompt = write_prompt(
            "correctness",
            lambda rf: build_correctness_prompt(ctx, code_diff, code_ranges,
                                                rf, "correctness"),
        )
        subagent_prompts.append(
            _prompt_entry("correctness", "correctness", prompt_file,
                          results_file, prompt))

    pr_result = {
        "number": pr_number,
        "title": pr_title,
        "headRefOid": head_sha,
        "baseRefName": base_ref,
        "author": author,
        "hasApproval": has_approval,
        "isExternalContributor": is_external,
        "has_bot_comments": has_bot_comments,
        "images": images,
        "thread_resolution": thread_resolution,
        "subagent_prompts": subagent_prompts,
        "worktree_path": worktree_path,
        "source_path": worktree_path or SOURCE_PATH_OVERRIDE
        or TARGET_REPO_PATH,
        "file_hashes_file": hashes_file,
        "diff_file": diff_file,
        "prior_comments_file": prior_comments_file,
        "files_reviewed": len(changed),
        "files_total": len(sections),
        "rereview": bool(prior_hashes),
    }

    total_prompt_chars = sum(sp["cost_estimate"]["prompt_chars"]
                             for sp in subagent_prompts)
    log(f"  COST PR #{pr_number}: {len(changed)} of {len(sections)} files, "
        f"diff={len(join_sections(sections[p] for p in changed)):,} chars, "
        f"prior_comments={len(prior_comments or ''):,} chars, "
        f"{len(subagent_prompts)} detect prompts, "
        f"total_prompt={total_prompt_chars:,} chars "
        f"(~{total_prompt_chars // 4:,} tokens)")
    return pr_result, None


def record_review(pr_number, head_sha, hashes_file):
    """Cache a PR as reviewed at head_sha, with its file hashes."""
    result = subprocess.run(
        [
            sys.executable,
            UPDATE_CACHE,
            str(pr_number),
            head_sha,
            f"--file-hashes={hashes_file}",
        ],
        capture_output=True,
        text=True,
        timeout=30,
        check=False,
    )
    if result.returncode != 0:
        log(f"WARNING: cache update failed for PR #{pr_number}: {result.stderr}"
            )
    return result.returncode == 0


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
            "total_bot_threads": 0,
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
    auto_mode, reviewer_priority, full, max_prs, fetch_args = parse_args()

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
    mode, days, page, pr_number, state, _, _ = _fp_mod.parse_args()
    sys.argv = old_argv

    raw_prs = _fp_mod.fetch_prs(mode, page, pr_number, state)
    org_members = load_org_members()
    cache = _fp_mod.load_cache()
    prior_files = {} if full else cache.get("_files", {})

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
    def pr_entry(pr):
        author = pr.get("author", {}).get("login", "unknown")
        entry = {
            "number": pr["number"],
            "title": pr["title"],
            "headRefOid": pr["headRefOid"],
            "baseRefName": pr.get("baseRefName", ""),
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
        progress_lines.append(f"Skipped {fetch_summary['skipped_approved']} "
                              "PRs (already approved).")
    if fetch_summary["skipped_external"]:
        progress_lines.append(f"Skipped {fetch_summary['skipped_external']} "
                              "PRs (external contributors).")

    log("\n".join(progress_lines))

    prune_stale_work_dirs()

    # Create work directory for prompt/result files
    work_dir = tempfile.mkdtemp(prefix="review-prs-", dir=work_dir_base())
    log(f"Work directory: {work_dir}")

    # 4. Process each PR in parallel
    errors = []
    processed_prs = []
    unchanged_prs = []

    if prs_to_process:
        log(f"\nProcessing {len(prs_to_process)} PRs in parallel...")
        with ThreadPoolExecutor(max_workers=5) as executor:
            futures = {
                executor.submit(
                    process_pr,
                    pr,
                    bot_username,
                    org_members,
                    work_dir,
                    auto_mode,
                    prior_files.get(str(pr["number"])),
                ): pr
                for pr in prs_to_process
            }
            for future in as_completed(futures):
                pr = futures[future]
                try:
                    result, error = future.result()
                    if error:
                        errors.append(error)
                    if result and result.get("unchanged"):
                        unchanged_prs.append((pr, result))
                    elif result:
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

    # 5. New commits but no changed file (a rebase) is handled like a cached PR.
    for pr, result in unchanged_prs:
        record_review(pr["number"], pr["headRefOid"],
                      result["file_hashes_file"])
    unchanged_numbers = {pr["number"] for pr, _ in unchanged_prs}
    cached_to_process += [pr for pr, _ in unchanged_prs]

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
                    if pr["number"] in unchanged_numbers:
                        result[
                            "reason"] = "no file changed since the last review"
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

    # 6. Write manifest.json (file paths only, no diffs or prompts)
    output = {
        "bot_username": bot_username,
        "pr_repo": PR_REPO,
        "target_repo_path": TARGET_REPO_PATH,
        "auto_mode": auto_mode,
        "reviewer_priority": reviewer_priority,
        "full": full,
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
    log(f"PRs with no changed file: {len(unchanged_prs)}")
    log(f"Total detect prompts: {total_prompts}")
    log(f"Total prompt size: {total_prompt_chars:,} chars "
        f"(~{total_prompt_tokens:,} tokens)")
    if total_prompts > 0:
        avg_chars = total_prompt_chars // total_prompts
        log(f"Average prompt size: {avg_chars:,} chars "
            f"(~{avg_chars // 4:,} tokens)")
    log(f"Cached PRs processed: {len(processed_cached)}")
    log(f"Errors: {len(errors)}")
    # Per-PR breakdown
    for pr in processed_prs:
        pr_chars = sum(
            sp.get("cost_estimate", {}).get("prompt_chars", 0)
            for sp in pr.get("subagent_prompts", []))
        log(f"  PR #{pr['number']}: {pr['files_reviewed']} of "
            f"{pr['files_total']} files, "
            f"{len(pr.get('subagent_prompts', []))} detect prompts, "
            f"{pr_chars:,} chars (~{pr_chars // 4:,} tokens)")
    log(f"{'=' * 60}")

    log(f"\nDone. {len(processed_prs)} PRs processed, "
        f"{total_prompts} total detect prompts, "
        f"{len(processed_cached)} cached PRs processed, "
        f"{len(errors)} errors.")

    # Output just the work_dir path to stdout (tiny — the LLM only needs this)
    print(json.dumps({"work_dir": work_dir, "manifest": manifest_path}))


if __name__ == "__main__":
    main()
