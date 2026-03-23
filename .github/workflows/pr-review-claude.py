#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Run Claude-driven PR review in CI (Docker/GitHub Actions).

Fetches PR context and diff via gh, sends to Anthropic API with the review
skill instructions, and posts the result as a GitHub review: one summary body
plus multiple inline comments on the code (when the model provides them).

Usage:
  python3 pr-review-claude.py <pr_number> [repo]
  repo defaults to brave/brave-core.

Environment:
  GH_TOKEN          - GitHub token (with pull request read + comment write)
  ANTHROPIC_API_KEY - Anthropic API key for Claude
"""

import json
import os
import subprocess
import sys
from pathlib import Path

try:
    import anthropic
except ImportError:
    print("anthropic package required; run: pip install anthropic", file=sys.stderr)
    sys.exit(1)

REPO_DEFAULT = "brave/brave-core"


def _skill_path() -> Path:
    """Review skill: baked into the CI image (trusted), or next to the repo in dev."""
    bundled = Path("/opt/pr-review-claude/skills/review/SKILL.md")
    if bundled.exists():
        return bundled
    # Local dev: script at .github/workflows/pr-review-claude.py → repo root is parent^3
    repo_root = Path(__file__).resolve().parent.parent.parent
    return repo_root / ".claude" / "skills" / "review" / "SKILL.md"


REVIEW_DISCLAIMER = (
    "I generated this review about the changes, sharing here. "
    "It should be used for informational purposes only and not as proof of review.\n\n"
)


def run_gh(*args, repo: str, capture=True):
    cmd = ["gh", *args, "-R", repo]
    env = {**os.environ}
    if "GH_TOKEN" in env:
        env["GH_TOKEN"] = os.environ["GH_TOKEN"]
    result = subprocess.run(
        cmd,
        capture_output=capture,
        text=True,
        env=env,
    )
    if result.returncode != 0 and capture:
        raise RuntimeError(f"gh failed: {result.stderr or result.stdout}")
    return result.stdout.strip() if capture else None


def gather_pr_context(pr_number: str, repo: str) -> dict:
    """Fetch PR title, body, and diff via gh."""
    out = run_gh("pr", "view", pr_number, "--json", "title,body,author", repo=repo)
    meta = json.loads(out)
    diff = run_gh("pr", "diff", pr_number, repo=repo)
    return {
        "title": meta.get("title", ""),
        "body": meta.get("body") or "",
        "author": (meta.get("author") or {}).get("login", ""),
        "diff": diff,
    }


def load_skill_content() -> str:
    """Load the review skill markdown (for system prompt)."""
    path = _skill_path()
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8", errors="replace")


def run_review(pr_number: str, repo: str) -> str:
    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if not api_key:
        raise ValueError("ANTHROPIC_API_KEY is not set")

    ctx = gather_pr_context(pr_number, repo)
    skill_content = load_skill_content()

    system = (
        "You are performing a PR code review for brave-core. Follow the Code Review Skill instructions below. "
        "You are running in CI: you cannot run bash commands or read additional files. Use ONLY the PR title, body, and diff provided. "
        "Output a single JSON object with two keys: 'body' and 'comments'. "
        "'body' (string): full review in markdown (Summary, Context, Analysis, Best Practices, Verdict: PASS/FAIL, Confidence). "
        "Prefix the body with: 'I generated this review about the changes, sharing here. It should be used for informational purposes only and not as proof of review.\\n\\n'. "
        "'comments' (array): for each issue that applies to a specific line in the diff, one object with 'path' (file path as in the diff), 'line' (line number in the new file), 'body' (1-3 sentence comment). "
        "Put issues that are not tied to a specific line in the body only. Output only the JSON object, no surrounding text or markdown code fence.\n\n"
        "---\n"
        + (skill_content if skill_content else "Review the diff for correctness, root cause analysis, and best practices.")
    )

    user_content = (
        f"Review this pull request.\n\n"
        f"**Repo**: {repo}\n**PR**: #{pr_number}\n**Title**: {ctx['title']}\n**Author**: @{ctx['author']}\n\n"
        f"**PR body:**\n```\n{ctx['body'][:15000]}\n```\n\n"
        f"**Diff:**\n```diff\n{ctx['diff'][:180000]}\n```\n\n"
        "Produce the review as a JSON object with 'body' (full markdown report) and 'comments' (array of {path, line, body} for inline comments)."
    )

    client = anthropic.Anthropic(api_key=api_key)
    message = client.messages.create(
        model="claude-sonnet-4-20250514",
        max_tokens=8192,
        system=system,
        messages=[{"role": "user", "content": user_content}],
    )

    text = ""
    for block in message.content:
        if hasattr(block, "text"):
            text += block.text
    return text.strip()


def _parse_review_response(raw: str) -> tuple[str, list[dict]]:
    """Parse Claude response into body and comments. Falls back to raw as body if not JSON."""
    raw = raw.strip()
    # Allow JSON wrapped in ```json ... ```
    if "```json" in raw:
        start = raw.index("```json") + 7
        end = raw.index("```", start)
        raw = raw[start:end]
    elif "```" in raw:
        start = raw.index("```") + 3
        end = raw.index("```", start)
        raw = raw[start:end]
    try:
        data = json.loads(raw)
        body = data.get("body") or ""
        comments = data.get("comments") or []
        if not isinstance(comments, list):
            comments = []
        # Normalize: each comment needs path, line (int), body; add side for API
        out = []
        for c in comments:
            if isinstance(c, dict) and c.get("path") and c.get("line") is not None and c.get("body"):
                out.append({
                    "path": str(c["path"]).strip(),
                    "line": int(c["line"]),
                    "side": "RIGHT",
                    "body": str(c["body"]).strip()[:65535],
                })
        body = REVIEW_DISCLAIMER + body if body else ""
        return body, out
    except (json.JSONDecodeError, ValueError, TypeError):
        return REVIEW_DISCLAIMER + raw, []


def post_review(pr_number: str, repo: str, body: str, comments: list[dict]) -> None:
    """Post as a GitHub review (body + inline comments) or a single PR comment if no inlines."""
    if comments:
        payload = {
            "event": "COMMENT",
            "body": body or "See inline comments.",
            "comments": comments,
        }
        cmd = [
            "gh", "api", f"repos/{repo}/pulls/{pr_number}/reviews",
            "--method", "POST", "-R", repo, "--input", "-",
        ]
        proc = subprocess.run(cmd, input=json.dumps(payload), text=True, env=os.environ)
        if proc.returncode != 0:
            raise RuntimeError(f"gh api reviews failed: {proc.returncode}")
    else:
        run_gh("pr", "comment", pr_number, "-b", body or "No review content.", repo=repo, capture=False)


def main():
    if len(sys.argv) < 2:
        print("Usage: pr-review-claude.py <pr_number> [repo]", file=sys.stderr)
        sys.exit(2)
    pr_number = sys.argv[1].strip()
    repo = sys.argv[2].strip() if len(sys.argv) > 2 else REPO_DEFAULT

    if not os.environ.get("GH_TOKEN"):
        print("GH_TOKEN is not set", file=sys.stderr)
        sys.exit(1)

    try:
        raw = run_review(pr_number, repo)
        body, comments = _parse_review_response(raw)
        post_review(pr_number, repo, body, comments)
        if comments:
            print(f"Posted review with {len(comments)} inline comment(s) on PR #{pr_number}")
        else:
            print("Posted review comment on PR #" + pr_number)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
