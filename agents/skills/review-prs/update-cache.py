# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Update the review-prs cache with a PR's HEAD SHA after review.

Also updates _last_run timestamp so the next fetch-prs.py run uses it
as the cutoff instead of a fixed N-day window. This prevents gaps if a
cron run is missed.

Usage:
  update-cache.py <pr_number> <head_ref_oid>            # Update SHA only
  update-cache.py <pr_number> <head_ref_oid> --approve
      # Update SHA + mark approved
"""

import json
import os
import sys
from datetime import datetime, timezone

_script_dir = os.path.dirname(os.path.abspath(__file__))
_repo_dir = os.path.normpath(os.path.join(_script_dir, "..", "..", ".."))
PR_REPO = "brave/brave-core"

args = [a for a in sys.argv[1:] if not a.startswith("--")]
flags = [a for a in sys.argv[1:] if a.startswith("--")]

if len(args) != 2:
    print(f"Usage: {sys.argv[0]} <pr_number> <head_ref_oid> [--approve]",
          file=sys.stderr)
    sys.exit(1)

pr_number = args[0]
head_ref_oid = args[1]
approve = "--approve" in flags

cache_path = os.path.join(_repo_dir, ".ignore", "review-prs-cache.json")

try:
    with open(cache_path) as f:
        cache = json.load(f)
except (FileNotFoundError, json.JSONDecodeError):
    cache = {}
    os.makedirs(os.path.dirname(cache_path), exist_ok=True)

cache[pr_number] = head_ref_oid
cache["_last_run"] = datetime.now(timezone.utc).isoformat()

if approve:
    approved = set(cache.get("_approved", []))
    approved.add(pr_number)
    cache["_approved"] = sorted(approved)

with open(cache_path, "w") as f:
    json.dump(cache, f, indent=2)
    f.write("\n")

status = "approved + cached" if approve else "cached"
pr_url = (f"https://github.com/{PR_REPO}/pull/{pr_number}")
print(f"Cache updated ({status}): "
      f"[PR #{pr_number}]({pr_url}) -> {head_ref_oid}")
