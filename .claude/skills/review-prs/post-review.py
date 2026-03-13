# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Phase 3 post-processing for review-prs: posting, dedup, cache, notifications.

Handles all post-review operations without LLM token usage:
- Cache updates
- Violation prioritization and capping
- Rule link validation
- Deduplication against existing comments
- Posting inline reviews (with fallbacks)
- Approval gate
- Summary output

Usage:
  python3 post-review.py --pr-repo <repo>
      --bot-username <username> [--auto] < input.json
  python3 post-review.py --pr-repo <repo>
      --bot-username <username> [--auto] --input <file>
"""

import argparse
import json
import os
import re
import subprocess
import sys
from datetime import datetime, timezone

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", ".."))

CACHE_PATH = os.path.join(REPO_DIR, ".ignore", "review-prs-cache.json")
MANAGE_BP_IDS = os.path.join(REPO_DIR, "script", "manage-bp-ids.py")
CHECK_CAN_APPROVE = os.path.join(SCRIPT_DIR, "scripts", "check-can-approve.py")
UPDATE_CACHE = os.path.join(SCRIPT_DIR, "update-cache.py")

MAX_COMMENTS_PER_PR = 5
NITS_THRESHOLD = 3  # Drop nits if >= this many higher-severity comments

SEVERITY_ORDER = {"high": 0, "medium": 1, "low": 2}

# Diff line range cache: {pr_number: {file_path: [(start, end), ...]}}
_diff_line_cache = {}


def log(msg):
    """Print to stderr."""
    print(msg, file=sys.stderr)


def run_cmd(cmd, input_data=None, timeout=30, check=False):
    """Run a command and return (returncode, stdout, stderr)."""
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
            input=input_data,
            cwd=REPO_DIR,
            check=False,
        )
        if check and result.returncode != 0:
            raise subprocess.CalledProcessError(result.returncode, cmd,
                                                result.stdout, result.stderr)
        return result.returncode, result.stdout.strip(), result.stderr.strip()
    except subprocess.TimeoutExpired:
        return -1, "", "timeout"
    except FileNotFoundError:
        return -1, "", f"command not found: {cmd[0]}"


def fetch_diff_line_ranges(repo, pr_number):
    """Fetch PR diff and parse valid new-side line ranges per file.

    Returns {file_path: [(start, end), ...]} where each tuple is an
    inclusive range of lines present in the diff on the RIGHT side.
    """
    if pr_number in _diff_line_cache:
        return _diff_line_cache[pr_number]

    rc, out, err = run_cmd(
        ["gh", "pr", "diff", "--repo", repo,
         str(pr_number)],
        timeout=120,
    )
    if rc != 0:
        log(f"WARNING: failed to fetch diff for PR #{pr_number}: {err}")
        _diff_line_cache[pr_number] = {}
        return {}

    ranges = {}
    current_file = None
    for line in out.split("\n"):
        # Track current file from diff headers
        if line.startswith("+++ b/"):
            current_file = line[6:]
            if current_file not in ranges:
                ranges[current_file] = []
        elif line.startswith("@@ ") and current_file:
            # Parse hunk header: @@ -old_start,old_count +new_start,new_count @@
            m = re.search(r'\+(\d+)(?:,(\d+))?', line)
            if m:
                start = int(m.group(1))
                count = int(m.group(2)) if m.group(2) else 1
                if count > 0:
                    ranges[current_file].append((start, start + count - 1))

    _diff_line_cache[pr_number] = ranges
    return ranges


def correct_line_for_diff(repo, pr_number, file_path, line):
    """If line is not within any diff hunk for the file,
    find the nearest valid line.

    Returns the corrected line number, or None if the file isn't in the diff.
    """
    ranges = fetch_diff_line_ranges(repo, pr_number)
    file_ranges = ranges.get(file_path)
    if not file_ranges:
        return None

    # Check if line is already in a valid range
    for start, end in file_ranges:
        if start <= line <= end:
            return line

    # Find the nearest valid line
    best_line = None
    best_dist = float("inf")
    for start, end in file_ranges:
        for candidate in (start, end):
            dist = abs(candidate - line)
            if dist < best_dist:
                best_dist = dist
                best_line = candidate

    if best_line is not None:
        log(f"LINE_CORRECTED: {file_path}:{line} -> "
            f"{file_path}:{best_line} (nearest diff line)")
    return best_line


def update_cache(pr_number, head_ref_oid, approve=False):
    """Update the review cache for a PR."""
    cmd = ["python3", UPDATE_CACHE, str(pr_number), head_ref_oid]
    if approve:
        cmd.append("--approve")
    rc, _out, err = run_cmd(cmd)
    if rc != 0:
        log(f"WARNING: cache update failed for PR #{pr_number}: {err}")
    return rc == 0


def prioritize_violations(violations, has_approval):
    """Sort and cap violations per the rules.

    Returns (kept, dropped_count).
    """
    if not violations:
        return [], 0

    # Sort by severity
    violations.sort(
        key=lambda v: SEVERITY_ORDER.get(v.get("severity", "low"), 2))

    if has_approval:
        # Approved PRs: high-severity only
        kept = [v for v in violations if v.get("severity") == "high"]
        dropped = len(violations) - len(kept)
        if dropped:
            log(f"CAPPED: dropped {dropped} "
                "medium/low violations "
                "(PR has approval)")
        return kept[:MAX_COMMENTS_PER_PR], dropped

    high = [v for v in violations if v.get("severity") == "high"]
    medium = [v for v in violations if v.get("severity") == "medium"]
    low = [v for v in violations if v.get("severity") == "low"]

    kept = list(high)
    remaining_slots = MAX_COMMENTS_PER_PR - len(kept)

    # Fill with medium
    if remaining_slots > 0:
        kept.extend(medium[:remaining_slots])
        remaining_slots = MAX_COMMENTS_PER_PR - len(kept)

    # Only include low (nits) if fewer than
    # NITS_THRESHOLD higher-severity comments
    higher_count = len(high) + min(len(medium),
                                   MAX_COMMENTS_PER_PR - len(high))
    if higher_count < NITS_THRESHOLD and remaining_slots > 0:
        kept.extend(low[:remaining_slots])

    total_input = len(violations)
    dropped = total_input - len(kept)
    if dropped > 0:
        log(f"CAPPED: dropped {dropped} violations "
            f"(kept {len(kept)} most important)")
    return kept, dropped


def validate_rule_link(violation):
    """Validate a violation's rule_link. Returns True if valid or no link.

    If invalid, strips the link from draft_comment and returns False.
    """
    rule_link = violation.get("rule_link")
    if not rule_link:
        return True  # No link to validate

    if not os.path.isfile(MANAGE_BP_IDS):
        return True  # Can't validate, skip

    # Extract fragment ID and doc from URL
    # URL format: https://github.com/.../docs/best-practices/<doc>.md#<ID>
    match = re.search(r'/([^/]+\.md)#([A-Za-z0-9_-]+)$', rule_link)
    if not match:
        return True  # Can't parse, leave as-is

    doc_name = match.group(1)
    fragment_id = match.group(2)

    rc, _out, _err = run_cmd([
        "python3", MANAGE_BP_IDS, "--check-link", fragment_id, "--doc",
        doc_name
    ])
    if rc != 0:
        # Invalid link — strip it from draft_comment
        file_path = violation.get("file", "?")
        line = violation.get("line", "?")
        log(f"INVALID_LINK: stripped broken link "
            f"#{fragment_id} from {file_path}:{line}")
        # Strip [best practice](...) link pattern from draft_comment
        draft = violation.get("draft_comment", "")
        draft = re.sub(r'\[best practice\]\([^)]*\)', '', draft).strip()
        violation["draft_comment"] = draft
        return False
    return True


def filter_violations_by_rule_link(violations):
    """Drop violations missing rule_link.

    Unless they are high-severity bug/correctness.
    Returns filtered list.
    """
    kept = []
    for v in violations:
        if v.get("rule_link"):
            kept.append(v)
        elif v.get("severity") == "high":
            # High-severity bug/correctness findings can skip rule_link
            kept.append(v)
        else:
            file_path = v.get("file", "?")
            line = v.get("line", "?")
            snippet = (v.get("draft_comment", ""))[:60]
            log(f'DROPPED: no rule_link for {file_path}:{line} — "{snippet}"')
    return kept


def fetch_existing_comments(repo, pr_number):
    """Fetch existing review comments on a PR.

    Returns list of {path, line, body, user}.
    """
    rc, out, _err = run_cmd([
        "gh",
        "api",
        f"repos/{repo}/pulls/{pr_number}/comments",
        "--paginate",
        "--jq",
        '[.[] | {path, line, body, user: .user.login}]',
    ],
                            timeout=60)
    if rc != 0 or not out:
        return []
    try:
        # gh --paginate with --jq may output multiple JSON arrays
        # Concatenate them
        comments = []
        for chunk in re.split(r'\]\s*\[', out):
            chunk = chunk.strip()
            if not chunk.startswith('['):
                chunk = '[' + chunk
            if not chunk.endswith(']'):
                chunk = chunk + ']'
            try:
                comments.extend(json.loads(chunk))
            except json.JSONDecodeError:
                pass
        return comments
    except Exception:
        return []


def deduplicate_violations(violations, existing_comments):
    """Remove violations where any existing comment exists on same file+line.

    Returns filtered list.
    """
    # Build set of (path, line) from existing comments
    existing = set()
    comment_authors = {}  # (path, line) -> user
    for c in existing_comments:
        path = c.get("path", "")
        line = c.get("line")
        if path and line is not None:
            key = (path, line)
            existing.add(key)
            comment_authors[key] = c.get("user", "unknown")

    kept = []
    for v in violations:
        key = (v.get("file", ""), v.get("line"))
        if key in existing:
            user = comment_authors.get(key, "unknown")
            log(f"DEDUP: skipped {v.get('file')}:"
                f"{v.get('line')} — already "
                f"commented by {user}")
        else:
            kept.append(v)
    return kept


def post_batch_review(repo, pr_number, violations, head_sha):
    """Post violations as a single inline review.

    Returns (review_url, posted_count).

    Corrects line numbers that fall outside diff hunks before posting.
    Falls back to individual comments if batch fails.
    """
    if not violations:
        return None, 0

    # Correct line numbers against the actual diff before posting
    corrected_violations = []
    for v in violations:
        corrected_line = correct_line_for_diff(repo, pr_number, v["file"],
                                               v["line"])
        if corrected_line is None:
            log(f"DROPPED: {v['file']}:{v['line']} — file not in diff")
            continue
        v["line"] = corrected_line
        corrected_violations.append(v)

    if not corrected_violations:
        return None, 0
    violations = corrected_violations

    comments = []
    for v in violations:
        comments.append({
            "path": v["file"],
            "line": v["line"],
            "side": "RIGHT",
            "body": v["draft_comment"],
        })

    payload = json.dumps({
        "event": "COMMENT",
        "body": "",
        "comments": comments,
    })

    rc, out, _err = run_cmd(
        [
            "gh", "api", f"repos/{repo}/pulls/{pr_number}/reviews", "--method",
            "POST", "--input", "-"
        ],
        input_data=payload,
        timeout=60,
    )

    if rc == 0:
        try:
            resp = json.loads(out)
            return resp.get("html_url", ""), len(comments)
        except json.JSONDecodeError:
            return "", len(comments)

    # Batch failed — fall back to individual comments
    log(f"WARNING: batch review failed for PR "
        f"#{pr_number}, falling back to individual comments")
    posted = 0
    review_url = ""
    for v in violations:
        individual_payload = json.dumps({
            "body": v["draft_comment"],
            "commit_id": head_sha,
            "path": v["file"],
            "line": v["line"],
            "side": "RIGHT",
        })
        rc2, out2, err2 = run_cmd(
            [
                "gh", "api", f"repos/{repo}/pulls/{pr_number}/comments",
                "--method", "POST", "--input", "-"
            ],
            input_data=individual_payload,
            timeout=30,
        )
        if rc2 == 0:
            posted += 1
            if not review_url:
                try:
                    resp2 = json.loads(out2)
                    review_url = resp2.get("html_url", "")
                except json.JSONDecodeError:
                    pass
        else:
            log(f"ERROR: failed to post inline comment "
                f"for {v['file']}:{v['line']}: {err2}")

    return review_url, posted


def submit_approval(repo, pr_number):
    """Submit an APPROVE review. Returns html_url or None."""
    payload = json.dumps({"event": "APPROVE", "body": ""})
    rc, out, _err = run_cmd(
        [
            "gh", "api", f"repos/{repo}/pulls/{pr_number}/reviews", "--method",
            "POST", "--input", "-"
        ],
        input_data=payload,
        timeout=30,
    )
    if rc == 0:
        try:
            resp = json.loads(out)
            return resp.get("html_url", "")
        except json.JSONDecodeError:
            return ""
    log(f"ERROR: failed to submit approval for PR #{pr_number}: {err}")
    return None


def check_can_approve(pr_number, bot_username):
    """Run the approval gate script. Returns True if approval is allowed."""
    rc, _out, _err = run_cmd(
        ["python3", CHECK_CAN_APPROVE,
         str(pr_number), bot_username],
        timeout=60,
    )
    return rc == 0


def pr_url(repo, number):
    return f"https://github.com/{repo}/pull/{number}"


def pr_link(repo, number, title=None):
    url = pr_url(repo, number)
    link = f"[PR #{number}]({url})"
    if title:
        link += f" ({title})"
    return link


def process_pr(pr_data, repo, bot_username, auto_mode):
    """Process a single PR. Returns result dict."""
    number = pr_data["number"]
    title = pr_data.get("title", "")
    head_sha = pr_data.get("headRefOid", "")
    has_approval = pr_data.get("hasApproval", False)
    violations = list(pr_data.get("violations", []))
    link = pr_link(repo, number, title)

    result = {
        "number": number,
        "status": "skipped",
        "comments_posted": 0,
        "review_url": "",
    }

    # 1. Always update cache
    update_cache(number, head_sha)

    try:
        # 2. Filter violations missing rule_link (unless high-severity)
        violations = filter_violations_by_rule_link(violations)

        # 3. Prioritize and cap
        violations, _dropped = prioritize_violations(violations, has_approval)

        # 4. Validate rule links
        for v in violations:
            validate_rule_link(v)

        # 5. Deduplicate against existing comments
        existing_comments = fetch_existing_comments(repo, number)
        violations = deduplicate_violations(violations, existing_comments)

        if not violations:
            # No violations — attempt approval
            if check_can_approve(number, bot_username):
                approval_url = submit_approval(repo, number)
                if approval_url is not None:
                    update_cache(number, head_sha, approve=True)
                    result["status"] = "approved"
                    result["review_url"] = approval_url
                    log(f"APPROVE: {link} - no violations, approved")
                else:
                    result["status"] = "skipped"
                    log(f"AUTO: {link} - SKIPPED: approval submission failed")
            else:
                # Can't approve but no violations to post
                result["status"] = "approved"  # Effectively clean
                log(f"AUTO: {link} - no violations")
            return result

        if auto_mode:
            # Post violations
            review_url, posted = post_batch_review(repo, number, violations,
                                                   head_sha)
            result["status"] = "posted"
            result["comments_posted"] = posted
            result["review_url"] = review_url or ""

            detail_lines = []
            for v in violations:
                detail_lines.append(
                    f"  - {v['file']}:{v['line']} ({v.get('rule', 'unknown')})"
                )
            details = "\n".join(detail_lines)
            log(f"AUTO: {link} - posted {posted} "
                f"comments - {review_url}\n{details}")
        else:
            # Non-auto: output violations for LLM to present
            result["status"] = "pending"
            result["violations"] = violations
            log(f"AUTO: {link} - {len(violations)} violations pending review")

    except Exception as e:
        log(f"AUTO: {link} - SKIPPED: {e}")
        result["status"] = "skipped"

    return result


def main():
    parser = argparse.ArgumentParser(
        description="Phase 3 post-processing for review-prs")
    parser.add_argument("--pr-repo", required=True, help="owner/repo for PRs")
    parser.add_argument("--bot-username",
                        required=True,
                        help="Bot GitHub username")
    parser.add_argument("--auto", action="store_true", help="Auto-post mode")
    parser.add_argument("--input",
                        dest="input_file",
                        help="Input JSON file (default: stdin)")
    args = parser.parse_args()

    # Read input
    if args.input_file:
        with open(args.input_file) as f:
            data = json.load(f)
    else:
        data = json.load(sys.stdin)

    pr_results_input = data.get("pr_results", [])
    if not pr_results_input:
        log("No PR results to process.")
        output = {
            "results": [],
            "summary": {
                "prs_reviewed": 0,
                "prs_with_violations": 0,
                "total_comments_posted": 0,
                "prs_approved": 0,
            }
        }
        print(json.dumps(output, indent=2))
        return

    results = []
    for pr_data in pr_results_input:
        result = process_pr(pr_data, args.pr_repo, args.bot_username,
                            args.auto)
        results.append(result)

    # Build summary
    prs_reviewed = len(results)
    prs_with_violations = sum(1 for r in results if r["status"] == "posted")
    total_comments = sum(r["comments_posted"] for r in results)
    prs_approved = sum(1 for r in results if r["status"] == "approved")

    summary = {
        "prs_reviewed": prs_reviewed,
        "prs_with_violations": prs_with_violations,
        "total_comments_posted": total_comments,
        "prs_approved": prs_approved,
    }

    # Print summary block to stderr
    now = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    summary_lines = [
        "========================================",
        "AUTO REVIEW SUMMARY",
        f"Date: {now}",
        f"PRs reviewed: {prs_reviewed}",
        f"PRs with violations: {prs_with_violations}",
        f"Total comments posted: {total_comments}",
        "Cached PRs processed: 0",
        f"PRs approved: {prs_approved}",
        "",
        "RESULTS:",
    ]
    for r in results:
        num = r["number"]
        # Find title from input
        title = ""
        for pr_data in pr_results_input:
            if pr_data["number"] == num:
                title = pr_data.get("title", "")
                break
        link = pr_link(args.pr_repo, num, title)
        if r["status"] == "approved":
            summary_lines.append(f"  \u2705 {link} - no violations, approved")
        elif r["status"] == "posted":
            url = r.get("review_url", "")
            summary_lines.append(
                f"  \u274c {link} - {r['comments_posted']} comments - {url}")
        elif r["status"] == "skipped":
            summary_lines.append(f"  \u23ed\ufe0f {link} - SKIPPED")
        elif r["status"] == "pending":
            count = len(r.get("violations", []))
            summary_lines.append(
                f"  \u23f3 {link} - {count} violations pending")
    summary_lines.append("========================================")
    log("\n".join(summary_lines))

    # Output result JSON to stdout
    # Strip internal violations from results before output
    clean_results = []
    for r in results:
        out_r = {
            "number": r["number"],
            "status": r["status"],
            "comments_posted": r["comments_posted"],
            "review_url": r["review_url"],
        }
        if r["status"] == "pending" and "violations" in r:
            out_r["violations"] = r["violations"]
        clean_results.append(out_r)

    output = {
        "results": clean_results,
        "summary": summary,
    }
    print(json.dumps(output, indent=2))


if __name__ == "__main__":
    main()
