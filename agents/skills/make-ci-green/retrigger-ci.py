#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Re-run failed CI jobs for a brave/brave-core PR.

Queries GitHub for failing checks, determines the failure stage via Jenkins API,
and triggers rebuilds with WIPE_WORKSPACE for build/infra failures or normal
re-runs for test/storybook failures.

Usage:
    python3 retrigger-ci.py <pr-number> [--dry-run] [--format json|markdown]
"""

import argparse
import base64
import json
import os
import re
import subprocess
import sys
import urllib.error
import urllib.request
from urllib.parse import urlparse

JENKINS_BASE_URL = os.environ.get("JENKINS_BASE_URL", "").rstrip("/")
JENKINS_USER = os.environ.get("JENKINS_USER", "")
_ALLOWED_SCHEMES = ("https://", )


def _safe_urlopen(req, **kwargs):
    """Wrapper around urllib.request.urlopen that validates URL scheme.

    Prevents file:// and other dangerous schemes from being used.
    """
    url = req.full_url if isinstance(req, urllib.request.Request) else req
    if not any(url.startswith(s) for s in _ALLOWED_SCHEMES):
        raise ValueError(f"URL scheme not allowed: {url}")
    return urllib.request.urlopen(req, **kwargs)  # nosemgrep


# Stage name keywords that indicate pre-test infrastructure failures.
# These warrant WIPE_WORKSPACE to clear potentially corrupted build state.
WIPE_WORKSPACE_KEYWORDS = {
    "init",
    "checkout",
    "install",
    "config",
    "build",
    "compile",
    "setup",
    "sync",
    "gclient",
    "source",
    "deps",
    "fetch",
    "configure",
    "bootstrap",
    "prepare",
    "environment",
}


def get_jenkins_auth():
    """Get Jenkins authentication credentials from environment.

    Returns:
        tuple of (user, token) or exits with error.
    """
    missing = []
    if not JENKINS_BASE_URL:
        missing.append("JENKINS_BASE_URL")
    if not JENKINS_USER:
        missing.append("JENKINS_USER")
    token = os.environ.get("JENKINS_TOKEN")
    if not token:
        missing.append("JENKINS_TOKEN")
    if missing:
        print(
            "Error: Required environment variable(s) not set: "
            f"{', '.join(missing)}\n"
            "Set them in your .envrc:\n"
            "  export JENKINS_BASE_URL=<value>\n"
            "  export JENKINS_USER=<value>\n"
            "  export JENKINS_TOKEN=<value>",
            file=sys.stderr,
        )
        sys.exit(1)
    return JENKINS_USER, token


def make_auth_header(user, token):
    """Create Basic Auth header value."""
    credentials = base64.b64encode(f"{user}:{token}".encode()).decode()
    return f"Basic {credentials}"


def get_crumb(auth_header):
    """Fetch Jenkins CSRF crumb for POST requests.

    Returns:
        dict with crumb header name and value, or None if crumbs are disabled.
    """
    url = f"{JENKINS_BASE_URL}/crumbIssuer/api/json"
    req = urllib.request.Request(
        url,
        headers={"Authorization": auth_header},
    )
    try:
        with _safe_urlopen(req, timeout=15) as resp:
            data = json.loads(resp.read())
            return {data["crumbRequestField"]: data["crumb"]}
    except (urllib.error.HTTPError, urllib.error.URLError, KeyError):
        # Crumb issuer may be disabled; proceed without it
        return None


def get_failing_checks(pr_number):
    """Query GitHub for PR check statuses.

    Uses gh api to fetch commit statuses for the PR's head SHA, which is
    compatible with all versions of the gh CLI.

    Returns:
        list of dicts with keys: name, state, link, is_jenkins
    """
    # Step 1: Get the PR's head commit SHA
    result = subprocess.run(
        [
            "gh",
            "pr",
            "view",
            str(pr_number),
            "--repo",
            "brave/brave-core",
            "--json",
            "statusCheckRollup",
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        print(f"Error: Failed to get PR checks: {result.stderr}",
              file=sys.stderr)
        sys.exit(2)

    data = json.loads(result.stdout)
    rollup = data.get("statusCheckRollup", [])

    # Map GitHub API states to our internal states
    state_map = {
        # Commit status states
        "success": "SUCCESS",
        "failure": "FAILURE",
        "error": "FAILURE",
        "pending": "PENDING",
        # Check run conclusions
        "SUCCESS": "SUCCESS",
        "FAILURE": "FAILURE",
        "ERROR": "FAILURE",
        "PENDING": "PENDING",
        "COMPLETED": None,  # need to check conclusion
    }

    results = []
    seen = set()
    jenkins_host = urlparse(JENKINS_BASE_URL).hostname or ""

    for entry in rollup:
        # statusCheckRollup entries can be either commit statuses or check runs
        name = entry.get("context") or entry.get("name", "")
        link = entry.get("targetUrl") or entry.get("detailsUrl") or ""

        # Determine state: check runs use status+conclusion, statuses use state
        raw_state = entry.get("state", "")
        conclusion = entry.get("conclusion", "")

        if raw_state == "COMPLETED" or (not raw_state and conclusion):
            state = state_map.get(conclusion.upper(), conclusion.upper())
        else:
            state = state_map.get(
                raw_state, state_map.get(raw_state.upper(), raw_state.upper()))

        if state is None:
            state = "PENDING"

        # Deduplicate: keep the latest status for each check name
        # (GitHub returns all statuses, we want the most recent)
        if name in seen:
            # Update existing entry if this one is newer (later in the list)
            for r in results:
                if r["name"] == name:
                    r["state"] = state
                    r["link"] = link
                    break
            continue

        seen.add(name)
        is_jenkins = bool(jenkins_host and jenkins_host in link)
        results.append({
            "name": name,
            "state": state,
            "link": link,
            "is_jenkins": is_jenkins,
        })

    return results


def parse_jenkins_url(link):
    """Parse a Jenkins build URL into components.

    Example input:
        https://<jenkins>/job/brave-core-build-pr-linux-x64/\
job/PR-33936/2/
    Returns:
        ("brave-core-build-pr-linux-x64", "PR-33936", "2")
    """
    parsed = urlparse(link)
    path = parsed.path.rstrip("/")
    parts = path.split("/")

    # Expected: /job/<job-name>/job/<branch>/<build-number>
    # parts: ['', 'job', 'brave-core-build-pr-linux-x64',
    #         'job', 'PR-33936', '2']
    job_name = None
    branch = None
    build_number = None

    i = 0
    while i < len(parts):
        if parts[i] == "job" and i + 1 < len(parts):
            if job_name is None:
                job_name = parts[i + 1]
            else:
                branch = parts[i + 1]
            i += 2
        else:
            # The last segment after job/branch is the build number
            if branch is not None and parts[i]:
                build_number = parts[i]
            i += 1

    return job_name, branch, build_number


def get_failed_stage(job, branch, build, auth_header):
    """Query Jenkins API for the failed pipeline stage.

    Returns:
        The name of the first failed stage, or None if not determinable.
    """
    url = (f"{JENKINS_BASE_URL}/job/{job}/job/{branch}/{build}/wfapi/describe")
    req = urllib.request.Request(
        url,
        headers={"Authorization": auth_header},
    )
    try:
        with _safe_urlopen(req, timeout=30) as resp:
            data = json.loads(resp.read())
    except urllib.error.HTTPError as e:
        print(
            f"  Warning: Jenkins API returned HTTP {e.code}"
            f" for {job}/{branch}/{build}",
            file=sys.stderr,
        )
        return None
    except urllib.error.URLError as e:
        print(
            f"  Warning: Could not reach Jenkins API: {e.reason}",
            file=sys.stderr,
        )
        return None

    # wfapi/describe returns {"stages": [{"name": "...", "status": "..."}]}
    stages = data.get("stages", [])
    for stage in stages:
        if stage.get("status") in ("FAILED", "ABORTED"):
            return stage.get("name")

    return None


def decide_action(stage_name):
    """Decide whether to use WIPE_WORKSPACE based on the failed stage.

    Returns:
        tuple of (wipe: bool, reason: str)
    """
    if stage_name is None:
        return (False, "Could not determine failed stage;"
                " defaulting to normal re-run")

    stage_lower = stage_name.lower()

    # Check if the stage name contains any WIPE_WORKSPACE keywords
    for keyword in WIPE_WORKSPACE_KEYWORDS:
        if keyword in stage_lower:
            return (True, f"Pre-test stage failure: \"{stage_name}\""
                    " -> WIPE_WORKSPACE")

    return (False, f"Test/post-build stage failure: \"{stage_name}\""
            " -> normal re-run")


# ---------------------------------------------------------------------------
# Test failure analysis helpers
# ---------------------------------------------------------------------------

# Cache for PR changed files (avoids redundant gh calls)
_pr_files_cache = {}


def is_test_stage(stage_name):
    """Return True if the failed stage is a test/post-build stage."""
    if stage_name is None:
        return False
    stage_lower = stage_name.lower()
    for keyword in WIPE_WORKSPACE_KEYWORDS:
        if keyword in stage_lower:
            return False
    return True


def extract_platform_from_job(job_name):
    """Extract platform string from a Jenkins job name.

    Example: 'brave-core-build-pr-linux-x64' -> 'linux-x64'
    """
    if not job_name:
        return "unknown"
    # Strip common prefixes to get the platform suffix
    for prefix in ("brave-core-build-pr-", "brave-core-build-"):
        if job_name.startswith(prefix):
            return job_name[len(prefix):]
    return job_name


def fetch_console_tail(job, branch, build, auth_header, tail_bytes=500_000):
    """Fetch the tail of Jenkins console output for a build.

    Uses an HTTP Range header to avoid downloading the entire log.

    Returns:
        Console text string, or empty string on failure.
    """
    url = f"{JENKINS_BASE_URL}/job/{job}/job/{branch}/{build}/consoleText"
    headers = {"Authorization": auth_header}
    # Request only the last tail_bytes
    headers["Range"] = f"bytes=-{tail_bytes}"

    req = urllib.request.Request(url, headers=headers)
    try:
        with _safe_urlopen(req, timeout=60) as resp:
            return resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        if e.code == 416:
            # Range not satisfiable — log is smaller than
            # tail_bytes, fetch all
            try:
                req2 = urllib.request.Request(
                    url, headers={"Authorization": auth_header})
                with _safe_urlopen(req2, timeout=60) as resp:
                    return resp.read().decode("utf-8", errors="replace")
            except (urllib.error.HTTPError, urllib.error.URLError):
                return ""
        print(
            f"  Warning: Jenkins console returned HTTP {e.code}",
            file=sys.stderr,
        )
        return ""
    except urllib.error.URLError as e:
        print(
            f"  Warning: Could not fetch console output: {e.reason}",
            file=sys.stderr,
        )
        return ""


def extract_test_failures(console_text):
    """Parse GTest output from console text to find failing tests.

    Looks for the GTest summary section (lines starting with [  FAILED  ])
    and extracts the test output block for each failing test.

    Returns:
        List of dicts with keys: test_name, stack_trace
    """
    lines = console_text.split("\n")

    # 1. Find all failing test names from the summary section.
    #    GTest prints a summary like:
    #    [  FAILED  ] TestSuite.TestMethod (123 ms)
    #    at the very end after "X test(s) failed".
    summary_re = re.compile(r"^\[\s+FAILED\s+\]\s+(\S+)")
    failing_names = []
    seen = set()
    for line in lines:
        m = summary_re.match(line.strip())
        if m:
            name = m.group(1).rstrip(",")
            if name not in seen:
                seen.add(name)
                failing_names.append(name)

    if not failing_names:
        return []

    # 2. For each failing test, extract its output block between
    #    [ RUN      ] TestName and [  FAILED  ] TestName
    results = []
    for test_name in failing_names:
        run_marker = f"[ RUN      ] {test_name}"
        fail_marker = f"[  FAILED  ] {test_name}"

        trace_lines = []
        capturing = False
        for line in lines:
            stripped = line.strip()
            if not capturing and run_marker in stripped:
                capturing = True
                continue
            if capturing:
                if fail_marker in stripped:
                    break
                trace_lines.append(line)

        # Limit stack trace length
        stack_trace = "\n".join(trace_lines[-50:]) if trace_lines else ""
        results.append({
            "test_name": test_name,
            "stack_trace": stack_trace,
        })

    return results


def _resolve_src_dir():
    """Resolve the path to the chromium src directory relative to this script.

    The script is at: src/brave/.claude/skills/make-ci-green/retrigger-ci.py
    The src dir is at: src/ (4 levels up from the script dir)
    """
    script_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.normpath(os.path.join(script_dir, "..", "..", "..", ".."))


def find_test_location(test_class_name):
    """Determine if a test is a Brave test or Chromium test.

    Runs git grep in src/brave/ first, then in src/ (excluding brave/).

    Returns:
        'brave', 'chromium', or 'unknown'
    """
    src_dir = _resolve_src_dir()
    brave_dir = os.path.join(src_dir, "brave")

    # Check src/brave/ first
    try:
        result = subprocess.run(
            ["git", "grep", "-l", test_class_name],
            cwd=brave_dir,
            capture_output=True,
            text=True,
            timeout=30,
            check=False,
        )
        if result.returncode == 0 and result.stdout.strip():
            return "brave"
    except (subprocess.TimeoutExpired, FileNotFoundError):
        pass

    # Check src/ excluding brave/
    try:
        result = subprocess.run(
            ["git", "grep", "-l", test_class_name, "--", ".", ":!brave"],
            cwd=src_dir,
            capture_output=True,
            text=True,
            timeout=30,
            check=False,
        )
        if result.returncode == 0 and result.stdout.strip():
            return "chromium"
    except (subprocess.TimeoutExpired, FileNotFoundError):
        pass

    return "unknown"


def get_test_source_files(test_class_name):
    """Find source files containing a test class.

    Returns:
        List of file paths relative to src/ (e.g., ['brave/browser/test.cc',
        'chrome/browser/test.cc']).
    """
    src_dir = _resolve_src_dir()
    files = []

    try:
        result = subprocess.run(
            ["git", "grep", "-l", test_class_name],
            cwd=src_dir,
            capture_output=True,
            text=True,
            timeout=30,
            check=False,
        )
        if result.returncode == 0 and result.stdout.strip():
            files = [
                f.strip() for f in result.stdout.strip().split("\n")
                if f.strip()
            ]
    except (subprocess.TimeoutExpired, FileNotFoundError):
        pass

    return files


def check_upstream_flake(test_name):
    """Run check-upstream-flake.py for a Chromium test.

    Returns:
        Dict with verdict info, or None on error.
    """
    script_dir = os.path.dirname(os.path.abspath(__file__))
    flake_script = os.path.normpath(
        os.path.join(script_dir, "..", "..", "..", "scripts",
                     "check-upstream-flake.py"))

    if not os.path.exists(flake_script):
        print(
            f"  Warning: check-upstream-flake.py not found at {flake_script}",
            file=sys.stderr,
        )
        return None

    try:
        result = subprocess.run(
            [sys.executable, flake_script, test_name, "--json"],
            capture_output=True,
            text=True,
            timeout=60,
            check=False,
        )
        if result.returncode in (0, 2):
            # 0 = found, 2 = not found (both produce valid JSON)
            data = json.loads(result.stdout)
            return {
                "verdict": data.get("overall_verdict", "unknown"),
                "recommendation": data.get("overall_recommendation", ""),
                "matched_tests": len(data.get("matched_tests", [])),
            }
    except (subprocess.TimeoutExpired, json.JSONDecodeError,
            FileNotFoundError) as e:
        print(f"  Warning: Upstream flake check failed: {e}", file=sys.stderr)

    return None


def get_pr_changed_files(pr_number):
    """Get the list of files changed in a PR. Results are cached.

    Returns:
        List of file paths relative to the brave-core repo root.
    """
    if pr_number in _pr_files_cache:
        return _pr_files_cache[pr_number]

    try:
        result = subprocess.run(
            [
                "gh",
                "pr",
                "view",
                str(pr_number),
                "--repo",
                "brave/brave-core",
                "--json",
                "files",
            ],
            capture_output=True,
            text=True,
            timeout=30,
            check=False,
        )
        if result.returncode != 0:
            print(
                f"  Warning: Could not get PR files: {result.stderr.strip()}",
                file=sys.stderr,
            )
            _pr_files_cache[pr_number] = []
            return []

        data = json.loads(result.stdout)
        files = [f.get("path", "") for f in data.get("files", [])]
        _pr_files_cache[pr_number] = files
        return files
    except (subprocess.TimeoutExpired, json.JSONDecodeError) as e:
        print(f"  Warning: Error getting PR files: {e}", file=sys.stderr)
        _pr_files_cache[pr_number] = []
        return []


def assess_pr_correlation(pr_files, test_source_files, test_location):
    """Determine if a test failure is likely related to PR changes.

    Args:
        pr_files: Files changed in the PR (relative to brave-core root).
        test_source_files: Files containing the test class (relative to src/).
        test_location: 'brave', 'chromium', or 'unknown'.

    Returns:
        Tuple of (assessment, reason). Assessment is one of:
        'likely_from_pr', 'likely_unrelated', 'unknown'.
    """
    if not test_source_files:
        return "unknown", "Could not locate test source files"
    if not pr_files:
        return "unknown", "Could not determine PR changed files"

    # Build sets of directories from PR files for quick lookup.
    # PR files are relative to brave-core root, which maps to src/brave/.
    pr_dirs = set()
    pr_file_set = set(pr_files)
    for f in pr_files:
        parts = f.rsplit("/", 1)
        if len(parts) > 1:
            pr_dirs.add(parts[0])
            # Add parent dir too for broader module matching
            parent = parts[0].rsplit("/", 1)
            if len(parent) > 1:
                pr_dirs.add(parent[0])

    for test_file in test_source_files:
        if test_file.startswith("brave/"):
            # Brave test: PR files map to paths under brave/.
            # Strip "brave/" prefix to compare with PR file paths.
            test_in_repo = test_file[len("brave/"):]

            # Direct match: PR modifies the test file
            if test_in_repo in pr_file_set:
                return ("likely_from_pr",
                        f"PR modifies test file: {test_in_repo}")

            # Same directory
            test_dir = test_in_repo.rsplit("/",
                                           1)[0] if "/" in test_in_repo else ""
            if test_dir and test_dir in pr_dirs:
                return "likely_from_pr", ("PR modifies files in same"
                                          f" directory as test: {test_dir}/")
        else:
            # Chromium test: check if PR has chromium_src
            # overrides in related paths.
            # e.g., test in chrome/browser/ui/ -> PR might have
            # chromium_src/chrome/browser/ui/ overrides.
            test_dir = (test_file.rsplit("/", 1)[0]
                        if "/" in test_file else "")
            for pr_file in pr_files:
                if (pr_file.startswith("chromium_src/") and test_dir):
                    override_path = pr_file[len("chromium_src/"):]
                    override_dir = override_path.rsplit(
                        "/", 1)[0] if "/" in override_path else ""
                    if override_dir and (
                            override_dir == test_dir
                            or override_dir.startswith(test_dir + "/")
                            or test_dir.startswith(override_dir + "/")):
                        return "likely_from_pr", (
                            "PR has chromium_src override"
                            f" in related path: {pr_file}")

    if test_location == "chromium":
        return "likely_unrelated", ("Chromium test with no related"
                                    " chromium_src overrides in PR")

    return ("likely_unrelated", "PR changes do not overlap with"
            " test source location")


def search_existing_issues(test_name):
    """Search for existing open issues matching a test name.

    Searches in brave/brave-browser.

    Returns:
        Dict with number, title, url of the best match, or None.
    """
    try:
        result = subprocess.run(
            [
                "gh",
                "issue",
                "list",
                "--repo",
                "brave/brave-browser",
                "--search",
                test_name,
                "--state",
                "open",
                "--json",
                "number,title,url",
                "--limit",
                "5",
            ],
            capture_output=True,
            text=True,
            timeout=30,
            check=False,
        )
        if result.returncode != 0:
            print(
                f"  Warning: Could not search issues: {result.stderr.strip()}",
                file=sys.stderr,
            )
            return None

        issues = json.loads(result.stdout)
        test_class = test_name.split(".")[0] if "." in test_name else test_name
        for issue in issues:
            title = issue.get("title", "")
            if test_name in title or test_class in title:
                return {
                    "number": issue["number"],
                    "title": issue["title"],
                    "url": issue["url"],
                }
        return None
    except (subprocess.TimeoutExpired, json.JSONDecodeError) as e:
        print(f"  Warning: Error searching issues: {e}", file=sys.stderr)
        return None


def build_issue_suggestion(test_name, stack_trace, platform, upstream_flake):
    """Build a suggested GitHub issue for a test failure.

    Returns:
        Dict with title, body, and labels for the suggested issue.
    """
    body_lines = [
        f"## Test Failure: `{test_name}`",
        "",
        f"**Platform:** {platform}",
        "",
    ]

    if upstream_flake:
        verdict = upstream_flake.get("verdict", "unknown")
        body_lines.append(f"**Upstream flake verdict:** {verdict}")
        rec = upstream_flake.get("recommendation", "")
        if rec:
            body_lines.append(f"**Recommendation:** {rec}")
        body_lines.append("")

    body_lines.append("### Stack Trace")
    body_lines.append("```")
    # Truncate stack trace for the issue body
    body_lines.append(
        stack_trace[:3000] if stack_trace else "(no stack trace captured)")
    body_lines.append("```")

    return {
        "title": f"Test failure: {test_name}",
        "body": "\n".join(body_lines),
        "labels": ["QA/intermittent"],
        "platform": platform,
    }


def analyze_test_failures(check, job, branch, build, auth_header, pr_number):
    """Analyze test failures for a single failing Jenkins check.

    Called when the failed stage is a test stage. Extracts failing tests,
    classifies them, checks upstream flakiness, assesses PR correlation,
    and searches for existing issues.

    Returns:
        List of test failure analysis dicts.
    """
    platform = extract_platform_from_job(job)

    # Fetch console output
    print(f"  Fetching console output for {check['name']}...", file=sys.stderr)
    console_text = fetch_console_tail(job, branch, build, auth_header)
    if not console_text:
        print("  Warning: No console output available", file=sys.stderr)
        return []

    # Extract test failures
    raw_failures = extract_test_failures(console_text)
    if not raw_failures:
        print("  No GTest failures found in console output", file=sys.stderr)
        return []

    print(f"  Found {len(raw_failures)} failing test(s)", file=sys.stderr)

    # Get PR changed files (cached across checks)
    pr_files = get_pr_changed_files(pr_number)

    # Analyze each failure
    test_failures = []
    for failure in raw_failures:
        test_name = failure["test_name"]
        stack_trace = failure["stack_trace"]

        print(f"    Analyzing: {test_name}", file=sys.stderr)

        # Classify location
        test_class = test_name.split(".")[0] if "." in test_name else test_name
        location = find_test_location(test_class)

        # Find source files for PR correlation
        source_files = get_test_source_files(test_class)

        # Check upstream flake (chromium tests only)
        upstream = None
        if location == "chromium":
            print("    Checking upstream flakiness...", file=sys.stderr)
            upstream = check_upstream_flake(test_name)

        # Assess PR correlation
        correlation, correlation_reason = assess_pr_correlation(
            pr_files, source_files, location)

        # Search for existing issues
        existing_issue = search_existing_issues(test_name)

        # Determine if we should suggest filing an issue:
        # Only if the failure seems unrelated to the PR AND no issue exists
        suggest_filing = (correlation == "likely_unrelated"
                          and existing_issue is None)

        issue_suggestion = None
        if suggest_filing:
            issue_suggestion = build_issue_suggestion(test_name, stack_trace,
                                                      platform, upstream)

        test_failures.append({
            "test_name": test_name,
            "stack_trace": stack_trace,
            "test_location": location,
            "test_source_files": source_files,
            "upstream_flake": upstream,
            "pr_correlation": correlation,
            "pr_correlation_reason": correlation_reason,
            "existing_issue": existing_issue,
            "suggest_filing_issue": suggest_filing,
            "issue_suggestion": issue_suggestion,
        })

    return test_failures


def trigger_build(job, branch, wipe, auth_header, crumb):
    """Trigger a Jenkins build.

    Returns:
        True on success, False on failure.
    """
    if wipe:
        url = (f"{JENKINS_BASE_URL}/job/{job}/job/{branch}"
               f"/buildWithParameters?WIPE_WORKSPACE=true")
    else:
        url = (
            f"{JENKINS_BASE_URL}/job/{job}/job/{branch}/buildWithParameters")

    headers = {
        "Authorization": auth_header,
        "Content-Type": "application/x-www-form-urlencoded",
    }
    if crumb:
        headers.update(crumb)

    req = urllib.request.Request(url, data=b"", headers=headers, method="POST")

    try:
        with _safe_urlopen(req, timeout=30):
            # Jenkins returns 201 (Created) or 302 (redirect) on success
            return True
    except urllib.error.HTTPError as e:
        # 201 may come as an "error" in some urllib versions
        if e.code in (201, 302):
            return True
        print(
            f"  Error: Jenkins returned HTTP {e.code} when triggering "
            f"{job}/{branch}: {e.reason}",
            file=sys.stderr,
        )
        return False
    except urllib.error.URLError as e:
        print(
            f"  Error: Could not reach Jenkins to trigger build: {e.reason}",
            file=sys.stderr,
        )
        return False


def format_markdown(results, pr_number, dry_run):
    """Format results as human-readable markdown."""
    lines = []

    failing = [
        r for r in results if r["state"] == "FAILURE" and r["is_jenkins"]
    ]
    non_jenkins_failing = [
        r for r in results if r["state"] == "FAILURE" and not r["is_jenkins"]
    ]
    pending = [
        r for r in results if r["state"] == "PENDING" and r["is_jenkins"]
    ]

    if not failing:
        lines.append(f"No failing Jenkins CI checks found for PR {pr_number}.")
        if non_jenkins_failing:
            lines.append("")
            lines.append("Non-Jenkins failures (not handled by this tool):")
            for r in non_jenkins_failing:
                lines.append(f"  - {r['name']}")
        return "\n".join(lines)

    lines.append(f"PR {pr_number}: {len(failing)} failing Jenkins check(s)\n")

    for r in failing:
        status_icon = "OK" if r.get("triggered") else (
            "DRY-RUN" if dry_run else "FAILED")
        action = "WIPE_WORKSPACE" if r.get("wipe") else "normal"
        lines.append(f"  [{status_icon}] {r['name']}")
        lines.append(f"       Stage: {r.get('failed_stage', 'unknown')}")
        lines.append(f"       Action: {action}")
        lines.append(f"       Reason: {r.get('reason', '')}")
        lines.append(f"       URL: {r.get('link', '')}")

        # Test failure details
        test_failures = r.get("test_failures", [])
        if test_failures:
            lines.append(f"       Test Failures ({len(test_failures)}):")
            for tf in test_failures:
                lines.append("")
                lines.append(f"         - {tf['test_name']}")
                lines.append(f"           Location: {tf['test_location']}")

                if tf.get("upstream_flake"):
                    uf = tf["upstream_flake"]
                    lines.append(f"           Upstream flake: {uf['verdict']}")

                lines.append(
                    f"           PR correlation: {tf['pr_correlation']}"
                    f" ({tf['pr_correlation_reason']})")

                if tf.get("existing_issue"):
                    ei = tf["existing_issue"]
                    lines.append("           Existing issue:"
                                 f" #{ei['number']} - {ei['title']}")
                    lines.append(f"             {ei['url']}")

                if tf.get("suggest_filing_issue") and tf.get(
                        "issue_suggestion"):
                    sug = tf["issue_suggestion"]
                    lines.append("           >> SUGGEST FILING"
                                 f" ISSUE: \"{sug['title']}\"")

                # Truncated stack trace
                trace = tf.get("stack_trace", "")
                if trace:
                    trace_lines = trace.split("\n")[-20:]
                    lines.append("           Stack trace (last 20 lines):")
                    for tl in trace_lines:
                        lines.append(f"             {tl}")

        lines.append("")

    if non_jenkins_failing:
        lines.append("Non-Jenkins failures (skipped):")
        for r in non_jenkins_failing:
            lines.append(f"  - {r['name']}")
        lines.append("")

    if pending:
        lines.append("Still pending:")
        for r in pending:
            lines.append(f"  - {r['name']}")

    return "\n".join(lines)


def format_json(results, pr_number, dry_run):
    """Format results as JSON."""
    failing = [
        r for r in results if r["state"] == "FAILURE" and r["is_jenkins"]
    ]
    non_jenkins_failing = [
        r for r in results if r["state"] == "FAILURE" and not r["is_jenkins"]
    ]
    pending = [
        r for r in results if r["state"] == "PENDING" and r["is_jenkins"]
    ]

    output = {
        "pr_number": pr_number,
        "dry_run": dry_run,
        "failing_jenkins_checks": [{
            "name": r["name"],
            "link": r.get("link", ""),
            "failed_stage": r.get("failed_stage"),
            "wipe_workspace": r.get("wipe", False),
            "reason": r.get("reason", ""),
            "triggered": r.get("triggered", False),
            "platform": r.get("platform", "unknown"),
            "test_failures": [{
                "test_name": tf["test_name"],
                "stack_trace": tf["stack_trace"],
                "test_location": tf["test_location"],
                "test_source_files": tf.get("test_source_files", []),
                "upstream_flake": tf.get("upstream_flake"),
                "pr_correlation": tf["pr_correlation"],
                "pr_correlation_reason": tf["pr_correlation_reason"],
                "existing_issue": tf.get("existing_issue"),
                "suggest_filing_issue": tf.get("suggest_filing_issue", False),
                "issue_suggestion": tf.get("issue_suggestion"),
            } for tf in r.get("test_failures", [])],
        } for r in failing],
        "non_jenkins_failures": [r["name"] for r in non_jenkins_failing],
        "pending": [r["name"] for r in pending],
    }
    return json.dumps(output, indent=2)


def main():
    parser = argparse.ArgumentParser(
        description="Re-run failed CI jobs for a brave/brave-core PR.")
    parser.add_argument("pr_number",
                        type=int,
                        help="PR number in brave/brave-core")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Analyze failures without triggering rebuilds",
    )
    parser.add_argument(
        "--format",
        choices=["json", "markdown"],
        default="markdown",
        help="Output format (default: markdown)",
    )
    args = parser.parse_args()

    # Validate Jenkins credentials
    user, token = get_jenkins_auth()
    auth_header = make_auth_header(user, token)

    # Get all check statuses
    print(f"Fetching checks for PR {args.pr_number}...", file=sys.stderr)
    checks = get_failing_checks(args.pr_number)

    # Find failing Jenkins checks
    failing_jenkins = [
        c for c in checks if c["state"] == "FAILURE" and c["is_jenkins"]
    ]

    if not failing_jenkins:
        # Format and output even for no-failures case
        output = (format_json(checks, args.pr_number, args.dry_run)
                  if args.format == "json" else format_markdown(
                      checks, args.pr_number, args.dry_run))
        print(output)
        sys.exit(3)

    # Analyze each failing check
    crumb = None
    if not args.dry_run:
        print("Fetching Jenkins CSRF crumb...", file=sys.stderr)
        crumb = get_crumb(auth_header)

    for check in failing_jenkins:
        job, branch, build = parse_jenkins_url(check["link"])
        if not job or not branch:
            check["failed_stage"] = None
            check["wipe"] = False
            check["reason"] = f"Could not parse Jenkins URL: {check['link']}"
            check["triggered"] = False
            continue

        print(f"Checking stage info for {check['name']}...", file=sys.stderr)
        failed_stage = get_failed_stage(job, branch, build, auth_header)
        wipe, reason = decide_action(failed_stage)

        check["failed_stage"] = failed_stage
        check["wipe"] = wipe
        check["reason"] = reason
        check["platform"] = extract_platform_from_job(job)

        # Analyze test failures when the failed stage is a test stage
        if is_test_stage(failed_stage):
            print("Test stage detected, analyzing failures...",
                  file=sys.stderr)
            check["test_failures"] = analyze_test_failures(
                check, job, branch, build, auth_header, args.pr_number)
        else:
            check["test_failures"] = []

        if args.dry_run:
            check["triggered"] = False
        else:
            print(
                f"Triggering {'WIPE_WORKSPACE ' if wipe else ''}rebuild for "
                f"{check['name']}...",
                file=sys.stderr,
            )
            check["triggered"] = trigger_build(job, branch, wipe, auth_header,
                                               crumb)

    # Output results
    output = (format_json(checks, args.pr_number, args.dry_run)
              if args.format == "json" else format_markdown(
                  checks, args.pr_number, args.dry_run))
    print(output)


if __name__ == "__main__":
    main()
