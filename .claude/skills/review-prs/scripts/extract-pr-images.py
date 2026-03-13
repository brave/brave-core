#!/usr/bin/env python3
"""
Extract and download images from a PR's description and org-member comments.

Downloads images to .ignore/pr-images/{pr_number}/ and outputs JSON with
local file paths. Respects org-member filtering to prevent prompt injection
via malicious images from external contributors.

Usage:
    python3 extract-pr-images.py <pr-number> [--repo REPO]
    python3 extract-pr-images.py 12345

Output JSON:
    {
      "images": [
        {"path": ".ignore/pr-images/12345/img_001.png", "source": "pr_body", "alt": "screenshot", "url": "https://..."},
        {"path": ".ignore/pr-images/12345/img_002.jpg", "source": "comment_by_user1", "alt": "", "url": "https://..."}
      ],
      "skipped": [
        {"url": "https://...", "reason": "external user", "source": "comment_by_attacker"}
      ],
      "summary": {"downloaded": 2, "skipped": 1}
    }
"""

import argparse
import json
import os
import re
import subprocess
import sys
import urllib.request
import urllib.error
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lib.load_config import load_config, require_config


# Allowed image hosts (GitHub-hosted content only, for security)
ALLOWED_HOSTS = [
    "user-images.githubusercontent.com",
    "github.com",
    "raw.githubusercontent.com",
    "github.user-images.githubusercontent.com",
    "private-user-images.githubusercontent.com",
    "objects.githubusercontent.com",
]

# Max images to download per PR (prevent abuse)
MAX_IMAGES = 10

# Max file size in bytes (5MB)
MAX_FILE_SIZE = 5 * 1024 * 1024

# Image file extensions we handle
IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".gif", ".webp", ".svg"}


def get_bot_dir():
    """Get the bot directory (parent of scripts/)."""
    return Path(__file__).resolve().parent.parent


def get_org_members():
    """Load org members from cache file."""
    cache_file = get_bot_dir() / ".ignore" / "org-members.txt"
    if not cache_file.exists():
        print(f"Error: Org members file not found at {cache_file}", file=sys.stderr)
        sys.exit(1)
    return set(cache_file.read_text().strip().splitlines())


def get_trusted_reviewers():
    """Load trusted reviewers allowlist."""
    allowlist = get_bot_dir() / "scripts" / "trusted-reviewers.txt"
    if allowlist.exists():
        return set(allowlist.read_text().strip().splitlines())
    return set()


def is_trusted(username, org_members, trusted_reviewers):
    """Check if a user is an org member or trusted reviewer."""
    return username in org_members or username in trusted_reviewers


def gh_api(endpoint):
    """Call GitHub API via gh CLI."""
    cmd = ["gh", "api", endpoint, "--paginate"]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    if result.returncode != 0:
        return None
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return None


def extract_image_urls(markdown_text):
    """Extract image URLs from markdown text.

    Handles:
    - ![alt](url)
    - <img src="url" ...>
    - <img src='url' ...>
    """
    urls = []

    # Markdown image syntax: ![alt](url)
    for match in re.finditer(r'!\[([^\]]*)\]\(([^)]+)\)', markdown_text):
        alt, url = match.group(1), match.group(2)
        urls.append({"alt": alt, "url": url.strip()})

    # HTML img tags: <img src="url"> or <img src='url'>
    for match in re.finditer(r'<img\s[^>]*src=["\']([^"\']+)["\']', markdown_text, re.IGNORECASE):
        url = match.group(1)
        # Avoid duplicates if same URL was in markdown syntax
        if not any(u["url"] == url for u in urls):
            urls.append({"alt": "", "url": url.strip()})

    return urls


def is_allowed_url(url):
    """Check if URL is from an allowed host."""
    try:
        from urllib.parse import urlparse
        parsed = urlparse(url)
        return parsed.hostname in ALLOWED_HOSTS and parsed.scheme == "https"
    except Exception:
        return False


def guess_extension(url, content_type=None):
    """Guess file extension from URL or content type."""
    from urllib.parse import urlparse
    path = urlparse(url).path.lower()

    for ext in IMAGE_EXTENSIONS:
        if path.endswith(ext):
            return ext

    if content_type:
        ct = content_type.lower()
        if "png" in ct:
            return ".png"
        if "jpeg" in ct or "jpg" in ct:
            return ".jpg"
        if "gif" in ct:
            return ".gif"
        if "webp" in ct:
            return ".webp"
        if "svg" in ct:
            return ".svg"

    return ".png"  # Default


def download_image(url, dest_path):
    """Download an image file. Returns True on success."""
    try:
        req = urllib.request.Request(url, headers={
            "User-Agent": "brave-dev-bot/1.0",
            "Accept": "image/*",
        })
        with urllib.request.urlopen(req, timeout=15) as response:
            content_type = response.headers.get("Content-Type", "")

            # Check content type is an image
            if not any(t in content_type.lower() for t in ["image/", "application/octet-stream"]):
                return False, f"not an image: {content_type}"

            # Read with size limit
            data = response.read(MAX_FILE_SIZE + 1)
            if len(data) > MAX_FILE_SIZE:
                return False, f"too large (>{MAX_FILE_SIZE} bytes)"

            # Ensure correct extension
            ext = guess_extension(url, content_type)
            if not str(dest_path).endswith(ext):
                dest_path = Path(str(dest_path).rsplit(".", 1)[0] + ext)

            dest_path.parent.mkdir(parents=True, exist_ok=True)
            dest_path.write_bytes(data)
            return True, str(dest_path)

    except urllib.error.HTTPError as e:
        return False, f"HTTP {e.code}"
    except urllib.error.URLError as e:
        return False, f"URL error: {e.reason}"
    except Exception as e:
        return False, str(e)


def main():
    _config = load_config()
    _default_repo = require_config(_config, "project.prRepository")

    parser = argparse.ArgumentParser(description="Extract and download PR images")
    parser.add_argument("pr_number", type=int, help="PR number")
    parser.add_argument("--repo", default=_default_repo, help="Repository")
    args = parser.parse_args()

    org_members = get_org_members()
    trusted_reviewers = get_trusted_reviewers()

    # Output directory
    img_dir = get_bot_dir() / ".ignore" / "pr-images" / str(args.pr_number)
    # Clean previous images for this PR
    if img_dir.exists():
        for f in img_dir.iterdir():
            f.unlink()

    images = []
    skipped = []
    img_counter = 0

    # --- Collect image URLs from trusted sources ---

    # 1. PR body/description
    pr_data = gh_api(f"repos/{args.repo}/pulls/{args.pr_number}")
    if not pr_data:
        print(json.dumps({"images": [], "skipped": [], "summary": {"downloaded": 0, "skipped": 0},
                          "error": "Failed to fetch PR data"}))
        sys.exit(0)

    pr_author = pr_data.get("user", {}).get("login", "")
    pr_body = pr_data.get("body") or ""

    # Include PR body images only if author is an org member
    author_trusted = is_trusted(pr_author, org_members, trusted_reviewers)

    if author_trusted and pr_body:
        for img in extract_image_urls(pr_body):
            if is_allowed_url(img["url"]):
                images.append({**img, "source": "pr_body", "author": pr_author})
            else:
                skipped.append({"url": img["url"], "reason": "disallowed host", "source": "pr_body"})
    elif pr_body and not author_trusted:
        for img in extract_image_urls(pr_body):
            skipped.append({"url": img["url"], "reason": "external author (PR body)", "source": f"pr_body_by_{pr_author}"})

    # Only fetch comments if PR body contained images (most PRs have none,
    # so this skips 3 API calls in the common case)
    if images or skipped:
        # 2. Review comments (inline code comments)
        review_comments = gh_api(f"repos/{args.repo}/pulls/{args.pr_number}/comments")
        if review_comments:
            for comment in review_comments:
                user = comment.get("user", {}).get("login", "")
                body = comment.get("body") or ""
                if is_trusted(user, org_members, trusted_reviewers) and body:
                    for img in extract_image_urls(body):
                        if is_allowed_url(img["url"]):
                            images.append({**img, "source": f"review_comment_by_{user}", "author": user})
                        else:
                            skipped.append({"url": img["url"], "reason": "disallowed host",
                                            "source": f"review_comment_by_{user}"})
                elif body:
                    for img in extract_image_urls(body):
                        skipped.append({"url": img["url"], "reason": "external user",
                                        "source": f"review_comment_by_{user}"})

        # 3. Issue comments (PR discussion)
        issue_comments = gh_api(f"repos/{args.repo}/issues/{args.pr_number}/comments")
        if issue_comments:
            for comment in issue_comments:
                user = comment.get("user", {}).get("login", "")
                body = comment.get("body") or ""
                if is_trusted(user, org_members, trusted_reviewers) and body:
                    for img in extract_image_urls(body):
                        if is_allowed_url(img["url"]):
                            images.append({**img, "source": f"discussion_comment_by_{user}", "author": user})
                        else:
                            skipped.append({"url": img["url"], "reason": "disallowed host",
                                            "source": f"discussion_comment_by_{user}"})
                elif body:
                    for img in extract_image_urls(body):
                        skipped.append({"url": img["url"], "reason": "external user",
                                        "source": f"discussion_comment_by_{user}"})

        # 4. Review body text (the body of each review submission)
        reviews = gh_api(f"repos/{args.repo}/pulls/{args.pr_number}/reviews")
        if reviews:
            for review in reviews:
                user = review.get("user", {}).get("login", "")
                body = review.get("body") or ""
                if is_trusted(user, org_members, trusted_reviewers) and body:
                    for img in extract_image_urls(body):
                        if is_allowed_url(img["url"]):
                            images.append({**img, "source": f"review_by_{user}", "author": user})
                        else:
                            skipped.append({"url": img["url"], "reason": "disallowed host",
                                            "source": f"review_by_{user}"})

    # --- Deduplicate by URL ---
    seen_urls = set()
    unique_images = []
    for img in images:
        if img["url"] not in seen_urls:
            seen_urls.add(img["url"])
            unique_images.append(img)
    images = unique_images

    # --- Cap at MAX_IMAGES ---
    if len(images) > MAX_IMAGES:
        for img in images[MAX_IMAGES:]:
            skipped.append({"url": img["url"], "reason": f"exceeded max ({MAX_IMAGES})", "source": img["source"]})
        images = images[:MAX_IMAGES]

    # --- Download ---
    downloaded = []
    for img in images:
        img_counter += 1
        ext = guess_extension(img["url"])
        dest = img_dir / f"img_{img_counter:03d}{ext}"

        success, result = download_image(img["url"], dest)
        if success:
            # result may have corrected the extension
            downloaded.append({
                "path": str(Path(result).relative_to(get_bot_dir())),
                "abs_path": result,
                "source": img["source"],
                "alt": img["alt"],
                "url": img["url"],
            })
        else:
            skipped.append({"url": img["url"], "reason": f"download failed: {result}", "source": img["source"]})

    output = {
        "images": downloaded,
        "skipped": skipped,
        "summary": {
            "downloaded": len(downloaded),
            "skipped": len(skipped),
        },
    }

    print(json.dumps(output, indent=2))


if __name__ == "__main__":
    main()
