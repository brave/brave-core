# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Pick the detect candidates to validate; write one validator prompt per PR.

Usage:
    python3 select-candidates.py --work-dir /var/tmp/review-prs/review-prs-XXXXX

Prints {"work_dir", "validators": [{"pr", "prompt_file", "results_file",
"candidates"}], "incomplete": [<pr>]} on stdout.
"""

import argparse
import importlib.util
import json
import os
import re
import sys
import tempfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))


def _load(name, filename):
    spec = importlib.util.spec_from_file_location(
        name, os.path.join(SCRIPT_DIR, filename)
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


_post = _load("post_review", "post-review.py")
_prep = _load("prepare_review", "prepare-review.py")

# Twice the posting cap, so it still has a choice after the validator drops
# some.
CANDIDATE_LIMIT = 2 * _post.MAX_COMMENTS_PER_PR

_CANDIDATE_FIELDS = (
    "file",
    "line",
    "severity",
    "rule",
    "rule_link",
    "issue",
    "draft_comment",
)


def log(msg):
    print(msg, file=sys.stderr)


def load_candidates(pr):
    """Every detect result for a PR. Returns (violations, missing, total)."""
    violations = []
    missing = 0
    prompts = pr.get("subagent_prompts", [])
    for entry in prompts:
        path = entry.get("results_file", "")
        try:
            with open(path, encoding='utf-8') as f:
                data = json.load(f)
        except (OSError, json.JSONDecodeError) as e:
            missing += 1
            log(
                f"WARNING: no detect results for PR #{pr['number']} "
                f"{entry.get('chunk_id', '?')}: {e}"
            )
            continue
        for v in data.get("violations", []) or []:
            if isinstance(v, dict) and v.get("file"):
                violations.append(v)
    return violations, missing, len(prompts)


_RULES_BY_DOC = {}


def rule_text(rule_link, bp_dir):
    """The text of the rule a link's anchor names, or None if it names none."""
    m = re.search(r"/([^/]+\.md)#([A-Za-z0-9_-]+)$", rule_link or "")
    if not m:
        return None
    doc, anchor = m.groups()
    key = (bp_dir, doc)
    if key not in _RULES_BY_DOC:
        try:
            with open(os.path.join(bp_dir, doc), encoding='utf-8') as f:
                _RULES_BY_DOC[key] = _prep._cb_mod.split_into_rules(f.read())[1]
        except OSError:
            _RULES_BY_DOC[key] = []
    marker = re.compile(rf'<a id="{re.escape(anchor)}"')
    for rule in _RULES_BY_DOC[key]:
        if marker.search(rule["text"]):
            return rule["text"]
    return None


def select(pr, violations, existing_comments, bp_dir):
    """Candidates post-review.py would still post, capped at CANDIDATE_LIMIT."""
    violations = sorted(
        violations,
        key=lambda v: _post.SEVERITY_ORDER.get(v.get("severity", "low"), 2),
    )
    violations = _post.filter_violations_by_rule_link(violations)
    known = []
    for v in violations:
        if v.get("rule_link") and rule_text(v["rule_link"], bp_dir) is None:
            log(
                f"DROPPED: {v.get('file')}:{v.get('line')} cites "
                f"{v['rule_link']}, which names no rule"
            )
            continue
        known.append(v)
    violations = _post.deduplicate_batch_violations(known)
    violations = _post.deduplicate_violations(violations, existing_comments)
    kept, _ = _post.prioritize_violations(
        violations, pr.get("hasApproval", False), limit=CANDIDATE_LIMIT
    )
    return kept


def validator_diff(diff_file, files):
    """The reviewed diff of just the files the candidates are on."""
    try:
        with open(diff_file, encoding='utf-8') as f:
            sections = _prep.split_diff(f.read())
    except OSError:
        return "", {}
    shown = [
        s
        for path, s in sections.items()
        if path in files and not _prep.omitted_reason(path, s)
    ]
    text = _prep.join_sections(shown)
    return text, _prep.parse_diff_line_ranges(text)


def write_validator(pr, candidates, bot_username, bp_dir):
    pr_work_dir = os.path.dirname(pr["file_hashes_file"])
    numbered = []
    for i, v in enumerate(candidates, 1):
        entry = {"id": f"c{i}"}
        entry.update({k: v[k] for k in _CANDIDATE_FIELDS if k in v})
        numbered.append(entry)
    cited = []
    for link in dict.fromkeys(
        v["rule_link"] for v in candidates if v.get("rule_link")
    ):
        cited.append((link, rule_text(link, bp_dir)))
    diff_text, ranges = validator_diff(
        pr.get("diff_file", ""), {v["file"] for v in candidates}
    )
    prior = ""
    try:
        with open(pr.get("prior_comments_file", ""), encoding='utf-8') as f:
            prior = f.read()
    except OSError:
        pass
    ctx = {
        "number": pr["number"],
        "title": pr.get("title", ""),
        "base_ref": pr.get("baseRefName", ""),
        "has_approval": pr.get("hasApproval", False),
        "prior_comments": prior,
        "bot_username": bot_username,
    }
    prompt_file = os.path.join(pr_work_dir, "validate_prompt.txt")
    results_file = os.path.join(pr_work_dir, "validated.json")
    prompt = _prep.build_validate_prompt(
        ctx,
        numbered,
        cited,
        diff_text,
        ranges,
        pr.get("images", []),
        pr.get("source_path")
        or pr.get("worktree_path")
        or _prep.TARGET_REPO_PATH,
        results_file,
    )
    with open(prompt_file, "w", encoding='utf-8') as f:
        f.write(prompt)
    return {
        "prompt_file": prompt_file,
        "results_file": results_file,
        "candidates": len(numbered),
        "prompt_chars": len(prompt),
    }


def write_manifest(path, manifest):
    fd, tmp = tempfile.mkstemp(dir=os.path.dirname(path), prefix=".manifest-")
    with os.fdopen(fd, "w") as f:
        json.dump(manifest, f, indent=2)
    os.replace(tmp, path)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__.split("\n", maxsplit=1)[0]
    )
    parser.add_argument("--work-dir", required=True)
    args = parser.parse_args()

    manifest_path = os.path.join(args.work_dir, "manifest.json")
    with open(manifest_path, encoding='utf-8') as f:
        manifest = json.load(f)
    repo = manifest["pr_repo"]
    bot_username = manifest["bot_username"]

    validators = []
    incomplete = []
    for pr in manifest.get("prs", []):
        number = pr["number"]
        violations, missing, total = load_candidates(pr)
        if total and missing == total:
            log(f"PR #{number}: no detect results at all; not reviewed")
            pr["review_incomplete"] = True
            pr["validation"] = None
            incomplete.append(number)
            continue
        existing = (
            _post.fetch_existing_comments(repo, number) if violations else []
        )
        candidates = select(pr, violations, existing, _prep.BP_DIR)
        log(
            f"PR #{number}: {len(violations)} detected, "
            f"{len(candidates)} to validate"
        )
        if not candidates:
            pr["validation"] = None
            continue
        pr["validation"] = write_validator(
            pr, candidates, bot_username, _prep.BP_DIR
        )
        validators.append({"pr": number, **pr["validation"]})

    write_manifest(manifest_path, manifest)
    print(
        json.dumps(
            {
                "work_dir": args.work_dir,
                "validators": validators,
                "incomplete": incomplete,
            }
        )
    )


if __name__ == "__main__":
    main()
