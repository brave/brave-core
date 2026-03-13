# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Collect subagent result files and feed them to post-review.py.

Replaces what the LLM used to do manually: parsing subagent output,
building JSON for post-review.py.

Usage:
    python3 collect-results.py --work-dir /tmp/review-prs-XXXXX [--auto]
"""

import argparse
import json
import os
import re
import subprocess
import sys

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", ".."))


def log(msg):
    """Print to stderr."""
    print(msg, file=sys.stderr)


def load_manifest(work_dir):
    """Load and return manifest.json from work_dir."""
    manifest_path = os.path.join(work_dir, "manifest.json")
    with open(manifest_path) as f:
        return json.load(f)


def collect_violations(pr):
    """Collect all violations and validation logs from a PR's subagent results.

    Returns (violations_list, validation_log_list).
    """
    all_violations = []
    all_validation_log = []

    for prompt_entry in pr.get("subagent_prompts", []):
        results_file = prompt_entry.get("results_file")
        if not results_file:
            continue

        if not os.path.isfile(results_file):
            chunk_id = prompt_entry.get("chunk_id", "unknown")
            log(f"WARNING: results file missing for PR #{pr['number']} "
                f"chunk {chunk_id}: {results_file}")
            continue

        try:
            with open(results_file) as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError) as e:
            chunk_id = prompt_entry.get("chunk_id", "unknown")
            log(f"WARNING: invalid results file for PR #{pr['number']} "
                f"chunk {chunk_id}: {e}")
            continue

        all_violations.extend(data.get("violations", []))
        all_validation_log.extend(data.get("validation_log", []))

    return all_violations, all_validation_log


def build_post_review_input(manifest):
    """Build the input JSON structure for post-review.py."""
    pr_results = []

    for pr in manifest.get("prs", []):
        violations, validation_log = collect_violations(pr)

        pr_results.append({
            "number": pr["number"],
            "title": pr.get("title", ""),
            "headRefOid": pr.get("headRefOid", ""),
            "hasApproval": pr.get("hasApproval", False),
            "violations": violations,
            "validation_log": validation_log,
        })

    return {"pr_results": pr_results}


def print_cached_and_progress(manifest):
    """Print cached PR results and progress lines to stderr."""
    for cached in manifest.get("cached_prs", []):
        number = cached.get("number", "?")
        title = cached.get("title", "")
        reason = cached.get("reason", "cached")
        log(f"CACHED: PR #{number} ({title}) — {reason}")

    for line in manifest.get("progress_lines", []):
        log(line)


def main():
    parser = argparse.ArgumentParser(
        description="Collect subagent results and run post-review.py")
    parser.add_argument("--work-dir",
                        required=True,
                        help="Temp directory with manifest.json and results")
    parser.add_argument("--auto",
                        action="store_true",
                        help="Pass --auto to post-review.py")
    args = parser.parse_args()

    # Load manifest
    try:
        manifest = load_manifest(args.work_dir)
    except (OSError, json.JSONDecodeError) as e:
        log(f"ERROR: failed to load manifest: {e}")
        sys.exit(1)

    bot_username = manifest.get("bot_username", "")
    pr_repo = manifest.get("pr_repo", "")
    auto_mode = args.auto or manifest.get("auto_mode", False)

    if not bot_username or not pr_repo:
        log("ERROR: manifest missing bot_username or pr_repo")
        sys.exit(1)

    # Validate manifest values to prevent command injection
    if not re.match(r"^[a-zA-Z0-9_.-]+/[a-zA-Z0-9_.-]+$", pr_repo):
        log(f"ERROR: invalid pr_repo format: {pr_repo}")
        sys.exit(1)
    if not re.match(r"^[a-zA-Z0-9_-]+$", bot_username):
        log(f"ERROR: invalid bot_username format: {bot_username}")
        sys.exit(1)

    # Print cached PRs and progress lines first (before post-review output)
    print_cached_and_progress(manifest)

    # Build post-review input
    post_review_data = build_post_review_input(manifest)

    # Collection stats
    total_chunks = sum(
        len(pr.get("subagent_prompts", [])) for pr in manifest.get("prs", []))
    results_found = 0
    results_missing = 0
    for pr in manifest.get("prs", []):
        for sp in pr.get("subagent_prompts", []):
            rf = sp.get("results_file", "")
            if rf and os.path.isfile(rf):
                results_found += 1
            else:
                results_missing += 1

    total_violations = sum(
        len(pr_r.get("violations", []))
        for pr_r in post_review_data.get("pr_results", []))
    total_validated = sum(
        len(pr_r.get("validation_log", []))
        for pr_r in post_review_data.get("pr_results", []))

    log(f"\n{'=' * 60}")
    log("COLLECTION SUMMARY")
    log(f"{'=' * 60}")
    log(f"Total subagent chunks: {total_chunks}")
    log(f"Results files found: {results_found}")
    log(f"Results files missing: {results_missing}")
    log(f"Total violations collected: {total_violations}")
    log(f"Total validation log entries: {total_validated}")
    for pr_r in post_review_data.get("pr_results", []):
        v_count = len(pr_r.get("violations", []))
        log(f"  PR #{pr_r['number']}: {v_count} violations")
    log(f"{'=' * 60}\n")

    # Write input file
    input_path = os.path.join(args.work_dir, "post-review-input.json")
    with open(input_path, "w") as f:
        json.dump(post_review_data, f, indent=2)

    # Log any errors from manifest
    for error in manifest.get("errors", []):
        log(f"ERROR (from prepare): {error}")

    # Run post-review.py
    cmd = [
        "python3",
        os.path.join(SCRIPT_DIR, "post-review.py"),
        "--pr-repo",
        pr_repo,
        "--bot-username",
        bot_username,
        "--input",
        input_path,
    ]
    if auto_mode:
        cmd.append("--auto")

    result = subprocess.run(cmd,
                            capture_output=True,
                            text=True,
                            cwd=REPO_DIR,
                            check=False)

    # Pass through stderr (summary log)
    if result.stderr:
        print(result.stderr, file=sys.stderr, end="")

    # Pass through stdout (result JSON)
    if result.stdout:
        print(result.stdout, end="")

    sys.exit(result.returncode)


if __name__ == "__main__":
    main()
