#!/usr/bin/env python3
"""
Resolve bot review threads on a PR.

Atomically: fetches bot comments, finds developer replies, adds thumbs-up
reactions, and resolves review threads. Replaces the multi-step manual
process that the LLM was unreliable at (consistently added reactions but
never resolved threads).

Usage:
    python3 resolve-bot-threads.py <pr-number> <bot-username> [--dry-run]
"""

import argparse
import json
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lib.load_config import load_config, require_config


def gh_rest(endpoint, method="GET", data=None):
    """Call a GitHub REST API endpoint via gh CLI."""
    cmd = ["gh", "api", endpoint]
    if method != "GET":
        cmd += ["--method", method]
    if data:
        for key, value in data.items():
            cmd += ["-f", f"{key}={value}"]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    if result.returncode != 0:
        return None
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return None


def gh_rest_paginated(endpoint, jq_filter=".[]"):
    """Call a GitHub REST API endpoint with pagination via gh CLI."""
    cmd = [
        "gh",
        "api",
        endpoint,
        "--paginate",
        "--jq",
        jq_filter,
    ]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
    if result.returncode != 0:
        print(f"Error fetching {endpoint}: {result.stderr}", file=sys.stderr)
        return []
    # --paginate with --jq outputs one JSON object per line
    items = []
    for line in result.stdout.strip().split("\n"):
        if line:
            try:
                items.append(json.loads(line))
            except json.JSONDecodeError:
                continue
    return items


def gh_graphql(query, variables):
    """Call the GitHub GraphQL API via gh CLI."""
    cmd = ["gh", "api", "graphql", "-f", f"query={query}"]
    for key, value in variables.items():
        if isinstance(value, int):
            cmd += ["-F", f"{key}={value}"]
        else:
            cmd += ["-f", f"{key}={value}"]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    if result.returncode != 0:
        print(f"GraphQL error: {result.stderr}", file=sys.stderr)
        return None
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return None


def fetch_review_comments(pr_number, repo):
    """Fetch all review comments on a PR."""
    endpoint = f"repos/{repo}/pulls/{pr_number}/comments"
    return gh_rest_paginated(endpoint)


def fetch_review_threads(pr_number, repo_owner, repo_name):
    """Fetch all review threads with their first comment info via GraphQL."""
    query = """
    query($owner: String!, $name: String!, $number: Int!) {
      repository(owner: $owner, name: $name) {
        pullRequest(number: $number) {
          reviewThreads(first: 100) {
            nodes {
              id
              isResolved
              comments(first: 1) {
                nodes {
                  databaseId
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
        return []
    try:
        return data["data"]["repository"]["pullRequest"]["reviewThreads"]["nodes"]
    except (KeyError, TypeError):
        return []


def add_reaction(comment_id, repo, dry_run):
    """Add a +1 reaction to a review comment."""
    if dry_run:
        return True
    result = gh_rest(
        f"repos/{repo}/pulls/comments/{comment_id}/reactions",
        method="POST",
        data={"content": "+1"},
    )
    return result is not None


def resolve_thread(thread_id, dry_run):
    """Resolve a review thread via GraphQL mutation."""
    if dry_run:
        return True
    mutation = """
    mutation($threadId: ID!) {
      resolveReviewThread(input: {threadId: $threadId}) {
        thread { isResolved }
      }
    }
    """
    data = gh_graphql(mutation, {"threadId": thread_id})
    if not data:
        return False
    try:
        return data["data"]["resolveReviewThread"]["thread"]["isResolved"]
    except (KeyError, TypeError):
        return False


def main():
    config = load_config()
    default_repo = require_config(config, "project.prRepository")

    parser = argparse.ArgumentParser(
        description="Resolve bot review threads on a PR."
    )
    parser.add_argument("pr_number", type=int, help="PR number")
    parser.add_argument("bot_username", help="Bot's GitHub username")
    parser.add_argument("--repo", default=default_repo, help="owner/repo for PRs")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be done without making changes",
    )
    args = parser.parse_args()

    repo = args.repo
    repo_owner, repo_name = repo.split("/", 1)

    # 1. Fetch all review comments
    comments = fetch_review_comments(args.pr_number, repo)
    if not comments:
        print(
            json.dumps(
                {
                    "resolved": [],
                    "already_resolved": [],
                    "no_reply": [],
                    "unresolved_bot_threads": 0,
                    "total_bot_threads": 0,
                }
            )
        )
        return

    # 2. Fetch review threads (GraphQL)
    threads = fetch_review_threads(args.pr_number, repo_owner, repo_name)
    if not threads:
        print(
            json.dumps(
                {
                    "resolved": [],
                    "already_resolved": [],
                    "no_reply": [],
                    "unresolved_bot_threads": 0,
                    "total_bot_threads": 0,
                }
            )
        )
        return

    # 3. Build mappings
    # Map: bot comment databaseId -> thread info
    bot_threads = {}
    for thread in threads:
        first_comments = thread.get("comments", {}).get("nodes", [])
        if not first_comments:
            continue
        first = first_comments[0]
        author = (first.get("author") or {}).get("login", "")
        if author == args.bot_username:
            db_id = first.get("databaseId")
            if db_id:
                bot_threads[db_id] = {
                    "threadId": thread["id"],
                    "isResolved": thread.get("isResolved", False),
                }

    # Find developer replies to bot comments
    # Map: bot comment databaseId -> latest reply info
    replies_to_bot = {}
    for comment in comments:
        reply_to = comment.get("in_reply_to_id")
        if reply_to and reply_to in bot_threads:
            user = (comment.get("user") or {}).get("login", "")
            if user != args.bot_username:
                # Keep the latest reply per bot comment
                existing = replies_to_bot.get(reply_to)
                if not existing or comment["id"] > existing["id"]:
                    replies_to_bot[reply_to] = {
                        "id": comment["id"],
                        "user": user,
                        "body": comment.get("body", "")[:100],
                    }

    # 4. Process each bot thread
    resolved = []
    already_resolved = []
    no_reply = []
    errors = []

    for bot_comment_id, thread_info in bot_threads.items():
        thread_id = thread_info["threadId"]
        is_resolved = thread_info["isResolved"]

        reply = replies_to_bot.get(bot_comment_id)

        if is_resolved:
            already_resolved.append(
                {
                    "botCommentId": bot_comment_id,
                    "threadId": thread_id,
                }
            )
            continue

        if not reply:
            no_reply.append(
                {
                    "botCommentId": bot_comment_id,
                    "threadId": thread_id,
                }
            )
            continue

        # Resolve first, then react — both must succeed or neither is visible.
        # Resolve is the harder operation; only add the visible thumbs-up
        # if the thread was actually resolved.
        resolve_ok = resolve_thread(thread_id, args.dry_run)
        reaction_ok = add_reaction(reply["id"], repo, args.dry_run) if resolve_ok else False

        if resolve_ok and reaction_ok:
            resolved.append(
                {
                    "botCommentId": bot_comment_id,
                    "threadId": thread_id,
                    "replyId": reply["id"],
                    "replyUser": reply["user"],
                }
            )
        else:
            errors.append(
                {
                    "botCommentId": bot_comment_id,
                    "threadId": thread_id,
                    "replyId": reply["id"],
                    "reactionOk": reaction_ok,
                    "resolveOk": resolve_ok,
                }
            )

    # Count remaining unresolved bot threads after our actions
    unresolved = len(no_reply) + len(errors)

    output = {
        "resolved": resolved,
        "already_resolved": already_resolved,
        "no_reply": no_reply,
        "unresolved_bot_threads": unresolved,
        "total_bot_threads": len(bot_threads),
    }
    if errors:
        output["errors"] = errors
    if args.dry_run:
        output["dry_run"] = True

    print(json.dumps(output, indent=2))


if __name__ == "__main__":
    main()
