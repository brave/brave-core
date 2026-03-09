#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Query Brave's Backtrace instance for top crashers.

Outputs developer-actionable crash data: signatures, stack traces,
platform/version
breakdowns, and triage URLs. Designed for both human consumption (markdown) and
bot/CI integration (json, ndjson, csv).

PII-safe: only outputs allowlisted aggregate fields. For full crash details,
use the triage URLs which link to the authenticated Backtrace UI.

Usage:
    python3 scripts/top-crashers.py --project myproject [--days 7] [--limit 25]
    python3 scripts/top-crashers.py --project myproject --format json --limit 10
    python3 scripts/top-crashers.py --project myproject --compare 2 --limit 10
    python3 scripts/top-crashers.py --project myproject --new-only --days 3

Environment:
    BACKTRACE_API_KEY   Required. API token with query:post capability.
    BACKTRACE_PROJECT   Optional default for --project.
"""

import argparse
import csv
import io
import json
import os
import re
import shutil
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timedelta, timezone

BACKTRACE_ENDPOINT = "https://brave.sp.backtrace.io"
BACKTRACE_UNIVERSE = "brave"

MAX_RETRIES = 2
RETRY_BACKOFF = [1, 3]  # seconds
REQUEST_TIMEOUT = 20  # seconds
MAX_FRAME_LENGTH = 200
_ALLOWED_SCHEMES = ("https://", )


def _safe_urlopen(req, **kwargs):
    """Wrapper around urllib.request.urlopen that validates URL scheme.

    Prevents file:// and other dangerous schemes from being used.
    """
    url = req.full_url if isinstance(req, urllib.request.Request) else req
    if not any(url.startswith(s) for s in _ALLOWED_SCHEMES):
        raise ValueError(f"URL scheme not allowed: {url}")
    return urllib.request.urlopen(req, **kwargs)  # nosemgrep


# Brave-specific namespace/prefix patterns for code origin classification
BRAVE_CODE_PATTERNS = [
    re.compile(r'\bbrave[:_]', re.IGNORECASE),
    re.compile(r'\bntp_background_images::', re.IGNORECASE),
    re.compile(r'\bbrave_ads::', re.IGNORECASE),
    re.compile(r'\bbrave_new_tab_page', re.IGNORECASE),
    re.compile(r'\bbrave_wallet::', re.IGNORECASE),
    re.compile(r'\bbrave_rewards::', re.IGNORECASE),
    re.compile(r'\bbrave_shields::', re.IGNORECASE),
    re.compile(r'\bmisc_metrics::', re.IGNORECASE),
    re.compile(r'\bBraveProfile', re.IGNORECASE),
    re.compile(r'\bBraveBrowser', re.IGNORECASE),
    re.compile(r'\bspeedreader::', re.IGNORECASE),
    re.compile(r'\bipfs::', re.IGNORECASE),
    re.compile(r'\btor::', re.IGNORECASE),
    re.compile(r'\bai_chat::', re.IGNORECASE),
    re.compile(r'\bskus::', re.IGNORECASE),
    re.compile(r'\bplaylist::', re.IGNORECASE),
    re.compile(r'\bdecentralized_dns::', re.IGNORECASE),
    re.compile(r'\bbrave_vpn::', re.IGNORECASE),
    re.compile(r'\bbrave_sync::', re.IGNORECASE),
    re.compile(r'\bbrave_search', re.IGNORECASE),
    re.compile(r'\bbrave_news::', re.IGNORECASE),
    re.compile(r'\bbrave_federated::', re.IGNORECASE),
]

# Fallback channel versions if wiki fetch fails
DEFAULT_CHANNEL_VERSIONS = {
    "nightly": "1.89",
    "beta": "1.88",
    "release": "1.87",
}

WIKI_URL = ("https://raw.githubusercontent.com/wiki"
            "/brave/brave-browser/Brave-Release-Schedule.md")

# Patterns to strip from stack frames for PII safety
PII_PATH_PATTERNS = [
    re.compile(r"/Users/[^\s/]+"),
    re.compile(r"/home/[^\s/]+"),
    re.compile(r"C:\\Users\\[^\s\\]+"),
    re.compile(r"/var/[^\s/]*/[^\s/]+"),
]

# ---------------------------------------------------------------------------
# Code origin classification
# ---------------------------------------------------------------------------


def find_brave_src(explicit_path=None):
    """Find the brave source directory (src/brave).

    Args:
        explicit_path: Explicit path from --brave-src flag.

    Returns:
        Absolute path to src/brave directory, or None if not found.
    """
    if explicit_path:
        normalized = os.path.normpath(explicit_path)
        if os.path.isdir(normalized):
            return normalized
        return None

    # Auto-discover relative to script location
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidates = [
        os.path.join(script_dir, "..", "..", "src", "brave"),
        os.path.join(script_dir, "..", "src", "brave"),
    ]
    for path in candidates:
        normalized = os.path.normpath(path)
        if os.path.isdir(normalized):
            return normalized
    return None


_symbol_grep_cache = {}


def symbol_in_brave_src(symbol, brave_src_path):
    """Check if a crashing symbol is defined/implemented in brave source.

    Extracts the short function name from a qualified C++ symbol and greps
    for it in the brave source tree. Uses ripgrep if available, falls back
    to grep.

    Args:
        symbol: Qualified C++ symbol (e.g. "BrowserView::NonClientHitTest").
        brave_src_path: Path to the src/brave directory.

    Returns:
        True if the symbol is found in brave source.
    """
    if not brave_src_path or not symbol or symbol == "unknown":
        return False

    # Skip unqualified C/system function names (no :: means it's likely a
    # system function like g_log, clone, start_thread, main, etc.)
    if "::" not in symbol:
        return False

    # Strip template params and function args before extracting name
    clean = re.sub(r'<[^>]*>', '', symbol)
    clean = clean.split("(")[0].strip()
    parts = [p for p in clean.split("::") if p]

    # Use the last two segments (Class::Method) for a more specific grep
    # "content::ChildThreadImpl::IOThreadState::CrashHungProcess"
    #   -> "CrashHungProcess" (method only, since
    #      IOThreadState is an inner class)
    # "BrowserView::NonClientHitTest" -> "NonClientHitTest"
    if len(parts) >= 2:
        search_name = parts[-1]
    else:
        search_name = parts[-1] if parts else ""

    # Skip very short or generic names that would match too broadly
    if not search_name or len(search_name) < 6:
        return False

    # Skip known system/library symbols
    if search_name.startswith("lib") or search_name.startswith("__"):
        return False

    if search_name in _symbol_grep_cache:
        return _symbol_grep_cache[search_name]

    rg_path = shutil.which("rg")
    try:
        if rg_path:
            result = subprocess.run(
                [
                    rg_path, "-l", "--type", "cpp", "--type-add", "cpp:*.mm",
                    "-m", "1", search_name, brave_src_path
                ],
                capture_output=True,
                text=True,
                timeout=10,
                check=False,
            )
        else:
            result = subprocess.run(
                [
                    "grep", "-rl", "-m", "1", "--include=*.cc",
                    "--include=*.h", "--include=*.mm", search_name,
                    brave_src_path
                ],
                capture_output=True,
                text=True,
                timeout=10,
                check=False,
            )
        found = result.returncode == 0 and bool(result.stdout.strip())
    except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
        found = False

    _symbol_grep_cache[search_name] = found
    return found


def is_brave_frame(frame):
    """Check if a stack frame belongs to Brave-specific code (by namespace)."""
    for pattern in BRAVE_CODE_PATTERNS:
        if pattern.search(frame):
            return True
    return False


def classify_code_origin(frames, brave_src_path=None):
    """Classify whether a crash is in Brave or Chromium code.

    Uses two strategies:
    1. Namespace heuristics (fast check for brave_*, brave::, etc.)
    2. Source grep (greps for the symbol in src/brave/ when available)

    Args:
        frames: List of sanitized stack frame strings.
        brave_src_path: Path to src/brave directory for grep-based checking.

    Returns:
        "brave" if top frame or majority of top frames are brave code,
        "chromium" if no brave frames found,
        "mixed" if both brave and non-brave frames in top 3.
    """
    if not frames:
        return "chromium"

    top_frames = frames[:3]

    # Strategy 1: namespace heuristics (fast, checks for brave_ prefixes)
    brave_count = sum(1 for f in top_frames if is_brave_frame(f))

    # Strategy 2: grep the crashing function (frame[0]) in brave source.
    # Only checks the top frame to avoid false positives from generic method
    # names deeper in the stack.
    if brave_count == 0 and brave_src_path:
        if symbol_in_brave_src(frames[0], brave_src_path):
            brave_count = 1

    if brave_count == 0:
        return "chromium"
    if brave_count == len(top_frames):
        return "brave"
    return "mixed"


# ---------------------------------------------------------------------------
# Channel version mapping
# ---------------------------------------------------------------------------


def fetch_channel_versions(verbose=False):
    """Fetch current channel versions from Brave Release Schedule wiki.

    The wiki table format is transposed (channels as columns):
        | **Channel**     | Release |  Beta  | Nightly |
        | --------------- | ------- | ------ | ------- |
        | **Milestone**   | 1.87.x  | 1.88.x | 1.89.x |

    Returns:
        Dict mapping channel name to Brave major.minor version string,
        e.g. {"nightly": "1.89", "beta": "1.88", "release": "1.87"}.
    """
    try:
        req = urllib.request.Request(WIKI_URL, method="GET")
        with _safe_urlopen(req, timeout=5) as resp:
            content = resp.read().decode("utf-8", errors="replace")

        # Find the Channel header row and Milestone row
        lines = content.split("\n")
        channel_cols = None
        versions = {}

        for line in lines:
            cells = [
                c.strip().strip("*").strip() for c in line.split("|")
                if c.strip()
            ]
            if not cells:
                continue

            # Find the header row with channel names
            cells_lower = [c.lower() for c in cells]
            if "channel" in cells_lower[0].lower() and any(
                    ch in cells_lower
                    for ch in ["release", "beta", "nightly"]):
                channel_cols = cells_lower[1:]  # skip the label column
                continue

            # Find the Milestone row with version numbers
            if channel_cols and "milestone" in cells[0].lower():
                ver_cells = cells[1:]
                for i, channel in enumerate(channel_cols):
                    if i < len(ver_cells):
                        ver_match = re.search(r'(\d+\.\d+)', ver_cells[i])
                        if ver_match:
                            versions[channel.strip()] = ver_match.group(1)
                break

        if len(versions) >= 3:
            if verbose:
                print(f"  Channel versions from wiki: {versions}",
                      file=sys.stderr)
            return versions
    except Exception as e:
        if verbose:
            print(f"  Could not fetch wiki channel versions: {e}",
                  file=sys.stderr)

    if verbose:
        print(f"  Using fallback channel versions: {DEFAULT_CHANNEL_VERSIONS}",
              file=sys.stderr)
    return dict(DEFAULT_CHANNEL_VERSIONS)


def parse_brave_version(version_str):
    """Extract Brave major.minor version from full version string.

    Version format: CHROMIUM_MAJOR.BRAVE_MAJOR.BRAVE_MINOR.BRAVE_PATCH
    e.g. "145.1.87.42" -> "1.87"

    Args:
        version_str: Full version string.

    Returns:
        Brave "MAJOR.MINOR" string, or None if unparseable.
    """
    parts = str(version_str).split(".")
    if len(parts) >= 3:
        try:
            return f"{parts[1]}.{parts[2]}"
        except (IndexError, ValueError):
            pass
    return None


def format_brave_version(version_str):
    """Format version string for human display.

    "145.1.87.42" -> "Brave 1.87.42 (Cr 145)"

    Args:
        version_str: Full version string.

    Returns:
        Human-readable version string.
    """
    parts = str(version_str).split(".")
    if len(parts) >= 4:
        return f"Brave {parts[1]}.{parts[2]}.{parts[3]} (Cr {parts[0]})"
    if len(parts) >= 3:
        return f"Brave {parts[1]}.{parts[2]} (Cr {parts[0]})"
    return str(version_str)


def build_channel_breakdown(version_hist, channel_versions):
    """Map a version histogram to channel counts.

    Args:
        version_hist: Dict of {version_string: count}.
        channel_versions: Dict of {channel: "MAJOR.MINOR"} from wiki.

    Returns:
        Tuple of (channel_breakdown_dict, affects_nightly_bool).
        channel_breakdown has keys: "nightly", "beta", "release", "older".
    """
    # Invert channel_versions: "1.87" -> "release"
    ver_to_channel = {}
    for channel, ver in channel_versions.items():
        ver_to_channel[ver] = channel

    breakdown = {"nightly": 0, "beta": 0, "release": 0, "older": 0}

    for version_str, count in version_hist.items():
        brave_ver = parse_brave_version(version_str)
        if brave_ver and brave_ver in ver_to_channel:
            breakdown[ver_to_channel[brave_ver]] += count
        else:
            breakdown["older"] += count

    affects_nightly = breakdown["nightly"] > 0
    return breakdown, affects_nightly


# ---------------------------------------------------------------------------
# Crash type classification
# ---------------------------------------------------------------------------


def classify_crash_type(classifier):
    """Classify whether this is a real crash or DumpWithoutCrashing.

    Args:
        classifier: Classifier string from Backtrace (e.g. "dump",
                     "invalid-access", "breakpoint", etc.)

    Returns:
        "dump" for DumpWithoutCrashing, "crash" for actual crashes.
    """
    if classifier and classifier.lower() == "dump":
        return "dump"
    return "crash"


# ---------------------------------------------------------------------------
# Backtrace API client
# ---------------------------------------------------------------------------


def backtrace_query(project, query_body, api_key, verbose=False):
    """POST a query to the Backtrace coronerd API.

    Args:
        project: Backtrace project name.
        query_body: Dict to send as JSON request body.
        api_key: API token string.
        verbose: Print request timing info.

    Returns:
        Parsed JSON response dict.

    Raises:
        SystemExit on fatal errors.
    """
    params = urllib.parse.urlencode({
        "universe": BACKTRACE_UNIVERSE,
        "project": project,
        "token": api_key,
    })
    url = f"{BACKTRACE_ENDPOINT}/api/query?{params}"
    data = json.dumps(query_body).encode("utf-8")

    req = urllib.request.Request(
        url,
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )

    last_err = None
    for attempt in range(1 + MAX_RETRIES):
        if attempt > 0:
            wait = RETRY_BACKOFF[min(attempt - 1, len(RETRY_BACKOFF) - 1)]
            print(f"  Retrying in {wait}s (attempt {attempt + 1})...",
                  file=sys.stderr)
            time.sleep(wait)

        t0 = time.monotonic()
        try:
            with _safe_urlopen(req, timeout=REQUEST_TIMEOUT) as resp:
                raw = resp.read()
            elapsed = time.monotonic() - t0
            if verbose:
                print(f"  API response: {len(raw)} bytes in {elapsed:.1f}s",
                      file=sys.stderr)
            break
        except urllib.error.HTTPError as e:
            elapsed = time.monotonic() - t0
            body_preview = ""
            try:
                body_preview = e.read().decode("utf-8", errors="replace")[:500]
            except Exception:
                pass

            if e.code == 401 or e.code == 403:
                print(
                    f"Error: HTTP {e.code} from Backtrace API. "
                    "Check that BACKTRACE_API_KEY has query:post capability.",
                    file=sys.stderr,
                )
                if body_preview:
                    print(f"Response: {body_preview}", file=sys.stderr)
                sys.exit(1)

            if e.code == 429 or e.code >= 500:
                last_err = e
                print(f"  HTTP {e.code} after {elapsed:.1f}s", file=sys.stderr)
                if attempt < MAX_RETRIES:
                    continue

            print(
                f"Error: HTTP {e.code} from Backtrace API: {e.reason}",
                file=sys.stderr,
            )
            if body_preview:
                print(f"Response: {body_preview}", file=sys.stderr)
            sys.exit(2)
        except urllib.error.URLError as e:
            last_err = e
            elapsed = time.monotonic() - t0
            print(f"  Network error after {elapsed:.1f}s: {e.reason}",
                  file=sys.stderr)
            if attempt < MAX_RETRIES:
                continue
            print(f"Error: Could not connect to Backtrace API: {e.reason}",
                  file=sys.stderr)
            sys.exit(2)
        except TimeoutError:
            last_err = TimeoutError("Request timed out")
            print(f"  Request timed out after {REQUEST_TIMEOUT}s",
                  file=sys.stderr)
            if attempt < MAX_RETRIES:
                continue
            print(
                f"Error: Backtrace API request timed out after "
                f"{MAX_RETRIES + 1} attempts.",
                file=sys.stderr)
            sys.exit(2)
    else:
        print(f"Error: All {MAX_RETRIES + 1} attempts failed: {last_err}",
              file=sys.stderr)
        sys.exit(2)

    try:
        return json.loads(raw)
    except json.JSONDecodeError:
        print("Error: Could not parse API response as JSON.", file=sys.stderr)
        print(f"Raw (first 500 bytes): {raw[:500]}", file=sys.stderr)
        sys.exit(2)


def build_query(days_start,
                days_end_ts,
                limit,
                platform=None,
                version=None,
                channel=None):
    """Build the coronerd query body for top crashers.

    Args:
        days_start: Unix timestamp for the start of the window.
        days_end_ts: Unix timestamp for the end of the window.
        limit: Max number of crash groups to return.
        platform: Optional platform filter (e.g. 'Windows').
        version: Optional version prefix filter.
        channel: Optional channel filter.

    Returns:
        Dict suitable for JSON POST to /api/query.
    """
    filters = {
        "timestamp": [["at-least", int(days_start)],
                      ["at-most", int(days_end_ts)]],
    }

    if platform:
        filters["uname.sysname"] = [["equal", platform]]
    if version:
        filters["version"] = [["regular-expression", f"^{re.escape(version)}"]]
    if channel:
        filters["channel"] = [["equal", channel]]

    query = {
        "group": ["fingerprint"],
        "fold": {
            "fingerprint": [["count"]],
            "callstack": [["head"]],
            "classifiers": [["head"]],
            "timestamp": [["range"]],
            "version": [["histogram"]],
            "uname.sysname": [["histogram"]],
        },
        "filter": [filters],
        "order": [{
            "name": ";count",
            "ordering": "descending"
        }],
        "limit": limit,
    }

    return query


# ---------------------------------------------------------------------------
# Response parsing (allowlist-only)
# ---------------------------------------------------------------------------


def sanitize_frame(frame):
    """Sanitize a single stack frame string for PII safety."""
    if not isinstance(frame, str):
        return str(frame)[:MAX_FRAME_LENGTH]

    for pattern in PII_PATH_PATTERNS:
        frame = pattern.sub("<path>", frame)

    if len(frame) > MAX_FRAME_LENGTH:
        frame = frame[:MAX_FRAME_LENGTH] + "..."

    return frame


def extract_top_bucket(histogram):
    """From a histogram dict {value: count, ...}, return (top_value, pct).

    Returns:
        Tuple of (top_value_str, percentage_float) or (None, 0.0).
    """
    if not histogram or not isinstance(histogram, dict):
        return None, 0.0

    total = sum(histogram.values())
    if total == 0:
        return None, 0.0

    top_val = max(histogram, key=lambda k: histogram[k])
    pct = histogram[top_val] / total
    return str(top_val), pct


def format_recency(last_seen_ts):
    """Format a unix timestamp as a human-readable recency string."""
    if not last_seen_ts:
        return "unknown"

    now = time.time()
    delta = now - last_seen_ts

    if delta < 60:
        return "just now"
    if delta < 3600:
        mins = int(delta / 60)
        return f"{mins}m ago"
    if delta < 86400:
        hours = int(delta / 3600)
        return f"{hours}h ago"
    days = int(delta / 86400)
    return f"{days}d ago"


def format_timestamp(ts):
    """Format a unix timestamp as ISO date string."""
    if not ts:
        return "unknown"
    return datetime.fromtimestamp(
        ts, tz=timezone.utc).strftime("%Y-%m-%d %H:%M UTC")


def parse_callstack_json(raw_str):
    """Parse a JSON-encoded callstack string into a list of frame strings.

    Backtrace returns callstacks as JSON like:
        {"frame":["func1","func2",...]}

    Args:
        raw_str: JSON string or plain string with frames.

    Returns:
        List of frame name strings.
    """
    if not raw_str or not isinstance(raw_str, str):
        return [str(raw_str)] if raw_str else []

    try:
        parsed = json.loads(raw_str)
        if isinstance(parsed, dict) and "frame" in parsed:
            return [str(f) for f in parsed["frame"]]
        if isinstance(parsed, list):
            return [str(f) for f in parsed]
    except (json.JSONDecodeError, TypeError):
        pass

    # Fall back to newline-separated
    return raw_str.split("\n")


def histogram_from_pairs(pairs):
    """Convert a list of [value, count] pairs to a {value: count} dict.

    The RLE response format returns histograms as arrays of [value, count]
    pairs rather than dicts.

    Args:
        pairs: List of [value, count] pairs, or a dict (returned as-is).

    Returns:
        Dict of {value: count}.
    """
    if isinstance(pairs, dict):
        return pairs
    if not isinstance(pairs, list):
        return {}
    result = {}
    for item in pairs:
        if isinstance(item, list) and len(item) >= 2:
            result[str(item[0])] = item[1]
    return result


def parse_response(response,
                   days,
                   max_frames,
                   min_count,
                   lookback_start_ts,
                   project,
                   channel_versions=None,
                   brave_src_path=None):
    """Parse the Backtrace query response into a list of crash group dicts.

    Handles the RLE-encoded response format (v1.2.0) where results are in
    a "values" array. Each entry is [fingerprint, [fold_results...]].

    Only extracts allowlisted fields. Never dumps raw response.

    Args:
        response: Parsed JSON response from Backtrace API.
        days: Number of days in the lookback window (for rate calc).
        max_frames: Max number of stack frames to include.
        min_count: Minimum crash count to include.
        lookback_start_ts: Unix timestamp of window start (for new-detection).
        project: Project name (for triage URL).
        channel_versions: Dict of {channel: "MAJOR.MINOR"} for channel mapping.

    Returns:
        List of crash group dicts with standardized fields.
    """
    resp_data = response.get("response", {})
    values = resp_data.get("values", [])

    crashers = []

    # RLE response format (v1.2.0):
    # {
    #   "response": {
    #     "encoding": "rle",
    #     "values": [
    #       [fingerprint, [
    #         [count],
    #         [callstack_json],
    #         [classifier],
    #         [ts_min, ts_max],
    #         [[ver1, n1], [ver2, n2], ...],
    #         [[plat1, n1], [plat2, n2], ...]
    #       ]],
    #       ...
    #     ]
    #   }
    # }
    #
    # Fold indices (must match build_query fold order)
    FOLD_COUNT = 0
    FOLD_CALLSTACK = 1
    FOLD_CLASSIFIERS = 2
    FOLD_TIMESTAMP = 3
    FOLD_VERSION = 4
    FOLD_PLATFORM = 5

    if not values:
        return []

    for entry in values:
        if not isinstance(entry, list) or len(entry) < 2:
            continue

        fingerprint = str(entry[0])
        folds = entry[1]

        if not isinstance(folds, list) or len(folds) < 6:
            continue

        # Extract count
        count_data = folds[FOLD_COUNT]
        count = count_data[0] if isinstance(
            count_data, list) and count_data else (
                count_data if isinstance(count_data, (int, float)) else 0)
        count = int(count)

        if count < min_count:
            continue

        # Extract callstack (head fold returns JSON-encoded callstack)
        callstack_raw = folds[FOLD_CALLSTACK]
        if isinstance(callstack_raw, list) and callstack_raw:
            cs = callstack_raw[0]
        else:
            cs = callstack_raw

        frames = parse_callstack_json(cs)
        frames = [sanitize_frame(f) for f in frames if f and str(f).strip()]
        frames = frames[:max_frames]

        # Extract classifiers
        classifiers_raw = folds[FOLD_CLASSIFIERS]
        if isinstance(classifiers_raw, list) and classifiers_raw:
            classifier = str(classifiers_raw[0])
        elif isinstance(classifiers_raw, str):
            classifier = classifiers_raw
        else:
            classifier = "unknown"

        # Extract timestamp range [min, max]
        ts_data = folds[FOLD_TIMESTAMP]
        first_seen_ts = None
        last_seen_ts = None
        if isinstance(ts_data, list) and len(ts_data) >= 2:
            first_seen_ts = ts_data[0]
            last_seen_ts = ts_data[1]
        elif isinstance(ts_data, list) and len(ts_data) == 1:
            first_seen_ts = last_seen_ts = ts_data[0]

        # Extract version histogram (RLE format: [[ver, count], ...])
        version_hist = histogram_from_pairs(folds[FOLD_VERSION])

        # Extract platform histogram (RLE format: [[platform, count], ...])
        platform_hist = histogram_from_pairs(folds[FOLD_PLATFORM])

        # Computed fields
        crashes_per_day = round(count / max(days, 1), 1)
        top_frame = frames[0] if frames else "unknown"
        top_platform, platform_pct = extract_top_bucket(platform_hist)
        top_version, version_pct = extract_top_bucket(version_hist)

        is_new = (first_seen_ts is not None
                  and first_seen_ts >= lookback_start_ts)

        # Crash type: "dump" (DumpWithoutCrashing) vs "crash" (real)
        crash_type = classify_crash_type(classifier)

        # Code origin: "brave", "chromium", or "mixed"
        code_origin = classify_code_origin(frames,
                                           brave_src_path=brave_src_path)

        # Channel breakdown
        channel_breakdown = {"nightly": 0, "beta": 0, "release": 0, "older": 0}
        affects_nightly = False
        if channel_versions:
            channel_breakdown, affects_nightly = build_channel_breakdown(
                version_hist, channel_versions)

        # Top channel
        top_channel = max(channel_breakdown, key=channel_breakdown.get) if any(
            channel_breakdown.values()) else "unknown"

        # Build signature for issue titles
        platform_label = top_platform or "unknown"
        version_label = (format_brave_version(top_version)
                         if top_version else "unknown")
        sig = f"{top_frame} ({classifier}) on {platform_label} {version_label}"

        # Suggested title
        crash_prefix = ("Crash"
                        if crash_type == "crash" else "DumpWithoutCrashing")
        suggested_title = f"{crash_prefix}: {top_frame} on {platform_label}"

        # Labels
        labels = [
            "crash" if crash_type == "crash" else "dump-without-crashing"
        ]
        if top_platform:
            labels.append(top_platform.lower().replace(" ", "-"))
        if is_new:
            labels.append("regression")
        if code_origin == "brave":
            labels.append("brave-code")

        # Triage URL
        triage_url = (
            f"{BACKTRACE_ENDPOINT}/p/{urllib.parse.quote(project)}"
            f"/triage?fingerprints={urllib.parse.quote(fingerprint)}")

        crashers.append({
            "fingerprint": fingerprint,
            "count": count,
            "crashes_per_day": crashes_per_day,
            "classifier": classifier,
            "crash_type": crash_type,
            "code_origin": code_origin,
            "top_frame": top_frame,
            "signature": sig,
            "callstack": frames,
            "platforms": platform_hist,
            "top_platform": top_platform,
            "platform_pct": round(platform_pct * 100, 1),
            "versions": version_hist,
            "top_version": top_version,
            "version_pct": round(version_pct * 100, 1),
            "channel_breakdown": channel_breakdown,
            "top_channel": top_channel,
            "affects_nightly": affects_nightly,
            "first_seen": format_timestamp(first_seen_ts),
            "first_seen_ts": first_seen_ts,
            "last_seen": format_timestamp(last_seen_ts),
            "last_seen_ts": last_seen_ts,
            "recency": format_recency(last_seen_ts),
            "is_new": is_new,
            "triage_url": triage_url,
            "suggested_title": suggested_title,
            "labels": labels,
        })

    return crashers


def sort_crashers(crashers, order):
    """Sort crashers list by the given order key."""
    if order == "last-seen":
        crashers.sort(key=lambda c: c.get("last_seen_ts") or 0, reverse=True)
    elif order == "rate":
        crashers.sort(key=lambda c: c["crashes_per_day"], reverse=True)
    else:  # "count" (default)
        crashers.sort(key=lambda c: c["count"], reverse=True)

    # Assign ranks after sorting
    for i, c in enumerate(crashers, 1):
        c["rank"] = i

    return crashers


# ---------------------------------------------------------------------------
# Regression detection
# ---------------------------------------------------------------------------


def compare_windows(recent, baseline):
    """Compare two lists of crash groups to detect regressions.

    Args:
        recent: List of crash group dicts from the recent window.
        baseline: List of crash group dicts from the baseline window.

    Returns:
        List of crash group dicts annotated with regression info, sorted
        by severity of change.
    """
    baseline_by_fp = {c["fingerprint"]: c for c in baseline}

    annotated = []
    for c in recent:
        fp = c["fingerprint"]
        if fp in baseline_by_fp:
            base_count = baseline_by_fp[fp]["count"]
            if base_count > 0:
                change_factor = c["count"] / base_count
            else:
                change_factor = float("inf")

            if change_factor > 2.0:
                c["regression_badge"] = "RISING"
                c["change_factor"] = round(change_factor, 1)
                c["baseline_count"] = base_count
            elif change_factor < 0.5:
                c["regression_badge"] = "FALLING"
                c["change_factor"] = round(change_factor, 1)
                c["baseline_count"] = base_count
            else:
                c["regression_badge"] = "STABLE"
                c["change_factor"] = round(change_factor, 1)
                c["baseline_count"] = base_count
        else:
            c["regression_badge"] = "NEW"
            c["change_factor"] = float("inf")
            c["baseline_count"] = 0

        annotated.append(c)

    # Sort: NEW first, then RISING by factor, then rest
    badge_priority = {"NEW": 0, "RISING": 1, "STABLE": 2, "FALLING": 3}
    annotated.sort(key=lambda c: (
        badge_priority.get(c.get("regression_badge", "STABLE"), 99),
        -c["count"],
    ))

    for i, c in enumerate(annotated, 1):
        c["rank"] = i

    return annotated


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------


def format_markdown(crashers, days, compare_mode=False):
    """Format crash groups as copy/paste-ready markdown."""
    lines = []
    lines.append("# Top Crashers Report")
    lines.append("")
    lines.append("> PII-safe aggregate summary. For full crash details, "
                 "use the triage URLs below.")
    lines.append("")
    lines.append(f"**Lookback:** {days} days | "
                 f"**Generated:** "
                 f"{datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}"
                 " | "
                 f"**Crashers:** {len(crashers)}")
    lines.append("")

    if not crashers:
        lines.append("No crashes found matching the criteria.")
        return "\n".join(lines)

    for c in crashers:
        # Build badges
        badges = []
        crash_type = c.get("crash_type", "crash")
        if crash_type == "dump":
            badges.append("DUMP_WITHOUT_CRASHING")
        else:
            badges.append("CRASH")

        code_origin = c.get("code_origin", "chromium")
        badges.append(code_origin.upper())

        if compare_mode and c.get("regression_badge"):
            badges.append(c["regression_badge"])
        elif c.get("is_new"):
            badges.append("NEW")

        badge_str = " ".join(f"[{b}]" for b in badges)

        lines.append(f"### #{c['rank']} — {c['suggested_title']} {badge_str}")
        lines.append("")

        lines.append("| Field | Value |")
        lines.append("|-------|-------|")
        lines.append(f"| Fingerprint | `{c['fingerprint']}` |")
        lines.append(
            f"| Count | {c['count']:,} ({c['crashes_per_day']}/day) |")
        type_label = ("DumpWithoutCrashing"
                      if crash_type == "dump" else "Actual crash")
        lines.append(f"| Type | {crash_type.upper()}"
                     f" ({type_label}) |")
        lines.append(f"| Code origin | {code_origin} |")
        lines.append(f"| Classifier | {c['classifier']} |")

        if c.get("top_platform"):
            lines.append(f"| Top platform | {c['top_platform']} "
                         f"({c['platform_pct']}%) |")
        if c.get("top_version"):
            lines.append(
                f"| Top version | {format_brave_version(c['top_version'])} "
                f"({c['version_pct']}%) |")

        # Channel breakdown
        ch = c.get("channel_breakdown", {})
        ch_total = sum(ch.values())
        if ch_total > 0:
            ch_parts = []
            for chan in ["nightly", "beta", "release", "older"]:
                n = ch.get(chan, 0)
                if n > 0:
                    pct = round(n / ch_total * 100)
                    ch_parts.append(f"{chan.capitalize()} {pct}%")
            lines.append(f"| Channels | {', '.join(ch_parts)} |")
            lines.append(f"| Affects Nightly | "
                         f"{'Yes' if c.get('affects_nightly') else 'No'} |")

        lines.append(f"| First seen | {c['first_seen']} |")
        lines.append(f"| Last seen | {c['last_seen']} ({c['recency']}) |")
        if compare_mode and "baseline_count" in c:
            lines.append(f"| Baseline count | {c['baseline_count']:,} |")
            if c.get("change_factor") != float("inf"):
                lines.append(f"| Change factor | {c['change_factor']}x |")
        lines.append(f"| Triage | {c['triage_url']} |")
        lines.append("")

        if c.get("callstack"):
            lines.append(f"**Callstack (top {len(c['callstack'])} frames):**")
            lines.append("```")
            for frame in c["callstack"]:
                lines.append(frame)
            lines.append("```")
            lines.append("")

        # Platform breakdown if multiple
        if c.get("platforms") and len(c["platforms"]) > 1:
            total = sum(c["platforms"].values())
            sorted_plats = sorted(c["platforms"].items(),
                                  key=lambda x: x[1],
                                  reverse=True)
            plat_parts = [
                f"{p} ({round(n / total * 100)}%)" for p, n in sorted_plats[:5]
            ]
            lines.append(f"**Platforms:** {', '.join(plat_parts)}")
            lines.append("")

        # Version breakdown if multiple
        if c.get("versions") and len(c["versions"]) > 1:
            total = sum(c["versions"].values())
            sorted_vers = sorted(c["versions"].items(),
                                 key=lambda x: x[1],
                                 reverse=True)
            ver_parts = [
                f"{format_brave_version(v)} ({round(n / total * 100)}%)"
                for v, n in sorted_vers[:5]
            ]
            lines.append(f"**Versions:** {', '.join(ver_parts)}")
            lines.append("")

        lines.append(f"**Suggested issue title:** {c['suggested_title']}")
        lines.append(f"**Labels:** {', '.join(c['labels'])}")
        lines.append("")
        lines.append("---")
        lines.append("")

    return "\n".join(lines)


def format_json_output(crashers, days, compare_mode=False):
    """Format crash groups as a single JSON object."""
    output = {
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "lookback_days": days,
        "total_crashers": len(crashers),
        "compare_mode": compare_mode,
        "crashers": [],
    }

    for c in crashers:
        entry = {
            "rank": c["rank"],
            "fingerprint": c["fingerprint"],
            "count": c["count"],
            "crashes_per_day": c["crashes_per_day"],
            "classifier": c["classifier"],
            "crash_type": c.get("crash_type", "crash"),
            "code_origin": c.get("code_origin", "chromium"),
            "top_frame": c["top_frame"],
            "signature": c["signature"],
            "callstack": c["callstack"],
            "top_platform": c["top_platform"],
            "platform_pct": c["platform_pct"],
            "platforms": c.get("platforms", {}),
            "top_version": c["top_version"],
            "version_pct": c["version_pct"],
            "versions": c.get("versions", {}),
            "channel_breakdown": c.get("channel_breakdown", {}),
            "top_channel": c.get("top_channel", "unknown"),
            "affects_nightly": c.get("affects_nightly", False),
            "first_seen": c["first_seen"],
            "last_seen": c["last_seen"],
            "recency": c["recency"],
            "is_new": c["is_new"],
            "triage_url": c["triage_url"],
            "suggested_title": c["suggested_title"],
            "labels": c["labels"],
        }
        if compare_mode:
            entry["regression_badge"] = c.get("regression_badge", "STABLE")
            entry["change_factor"] = c.get("change_factor")
            entry["baseline_count"] = c.get("baseline_count")

        output["crashers"].append(entry)

    return json.dumps(output, indent=2)


def format_ndjson(crashers, compare_mode=False):
    """Format crash groups as newline-delimited JSON (one object per line)."""
    lines = []
    for c in crashers:
        entry = {
            "rank": c["rank"],
            "fingerprint": c["fingerprint"],
            "count": c["count"],
            "crashes_per_day": c["crashes_per_day"],
            "classifier": c["classifier"],
            "crash_type": c.get("crash_type", "crash"),
            "code_origin": c.get("code_origin", "chromium"),
            "top_frame": c["top_frame"],
            "signature": c["signature"],
            "callstack": c["callstack"],
            "top_platform": c["top_platform"],
            "platform_pct": c["platform_pct"],
            "top_version": c["top_version"],
            "version_pct": c["version_pct"],
            "channel_breakdown": c.get("channel_breakdown", {}),
            "top_channel": c.get("top_channel", "unknown"),
            "affects_nightly": c.get("affects_nightly", False),
            "first_seen": c["first_seen"],
            "last_seen": c["last_seen"],
            "recency": c["recency"],
            "is_new": c["is_new"],
            "triage_url": c["triage_url"],
            "suggested_title": c["suggested_title"],
            "labels": c["labels"],
        }
        if compare_mode:
            entry["regression_badge"] = c.get("regression_badge", "STABLE")
            entry["change_factor"] = c.get("change_factor")
            entry["baseline_count"] = c.get("baseline_count")

        lines.append(json.dumps(entry))

    return "\n".join(lines)


def format_csv_output(crashers, compare_mode=False):
    """Format crash groups as CSV."""
    buf = io.StringIO()
    fields = [
        "rank",
        "fingerprint",
        "count",
        "crashes_per_day",
        "classifier",
        "crash_type",
        "code_origin",
        "top_frame",
        "top_platform",
        "platform_pct",
        "top_version",
        "version_pct",
        "top_channel",
        "affects_nightly",
        "first_seen",
        "last_seen",
        "recency",
        "is_new",
        "triage_url",
        "suggested_title",
    ]
    if compare_mode:
        fields.extend(["regression_badge", "change_factor", "baseline_count"])

    writer = csv.DictWriter(buf, fieldnames=fields, extrasaction="ignore")
    writer.writeheader()

    for c in crashers:
        row = {k: c.get(k, "") for k in fields}
        writer.writerow(row)

    return buf.getvalue()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(
        description="Query Brave's Backtrace instance for top crashers.",
        epilog=
        ("Examples:\n"
         "  python3 scripts/top-crashers.py --project brave-browser --days 7\n"
         "  python3 scripts/top-crashers.py"
         " --project brave-browser"
         " --format json --limit 10\n"
         "  python3 scripts/top-crashers.py"
         " --project brave-browser --compare 2\n"
         "  python3 scripts/top-crashers.py"
         " --project brave-browser --new-only\n"),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--project",
        default=os.environ.get("BACKTRACE_PROJECT"),
        help="Backtrace project name (or set BACKTRACE_PROJECT env var)",
    )
    parser.add_argument(
        "--days",
        type=int,
        default=7,
        help="Lookback window in days (default: 7)",
    )
    parser.add_argument(
        "--since",
        help="ISO date for start of window (alternative to --days, "
        "e.g. 2025-01-15)",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=25,
        help="Max number of crash groups to return (default: 25, max: 500)",
    )
    parser.add_argument(
        "--min-count",
        type=int,
        default=10,
        help="Minimum crash count to include (default: 10)",
    )
    parser.add_argument(
        "--format",
        choices=["markdown", "json", "ndjson", "csv"],
        default="markdown",
        help="Output format (default: markdown)",
    )
    parser.add_argument(
        "--platform",
        help="Filter by platform (e.g. Windows, Darwin, Linux, Android)",
    )
    parser.add_argument(
        "--version",
        help="Filter by version prefix (e.g. 1.62.)",
    )
    parser.add_argument(
        "--channel",
        help="Filter by channel (e.g. stable, beta, nightly)",
    )
    parser.add_argument(
        "--order",
        choices=["count", "last-seen", "rate"],
        default="count",
        help="Sort order (default: count)",
    )
    parser.add_argument(
        "--frames",
        type=int,
        default=8,
        help="Number of stack frames to show (default: 8)",
    )
    parser.add_argument(
        "--new-only",
        action="store_true",
        help="Only show fingerprints first seen within the lookback window",
    )
    parser.add_argument(
        "--crashes-only",
        action="store_true",
        help="Exclude DumpWithoutCrashing (classifier='dump') — show only "
        "real user-impacting crashes",
    )
    parser.add_argument(
        "--channels",
        default="all",
        help="Only show crashes appearing on these channels "
        "(comma-separated: nightly,beta,release, or 'all'; default: all)",
    )
    parser.add_argument(
        "--brave-only",
        action="store_true",
        help="Only show crashes in Brave-specific code (not upstream Chromium)",
    )
    parser.add_argument(
        "--brave-src",
        help="Path to src/brave directory for code origin detection "
        "(auto-discovered if not set)",
    )
    parser.add_argument(
        "--compare",
        type=int,
        metavar="DAYS",
        help="Regression detection: compare last N days vs prior N days",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the query JSON and URL (minus token) without executing",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print request timing and debug info to stderr",
    )

    args = parser.parse_args()

    # Validate
    api_key = os.environ.get("BACKTRACE_API_KEY")
    if not api_key and not args.dry_run:
        print("Error: BACKTRACE_API_KEY environment variable is required.",
              file=sys.stderr)
        print(
            "Create a token with query:post capability in Backtrace "
            "project settings.",
            file=sys.stderr)
        sys.exit(1)

    if not args.project:
        print(
            "Error: --project is required (or set BACKTRACE_PROJECT "
            "env var).",
            file=sys.stderr)
        sys.exit(1)

    if args.limit < 1 or args.limit > 500:
        print("Error: --limit must be between 1 and 500.", file=sys.stderr)
        sys.exit(1)

    if args.min_count < 0:
        print("Error: --min-count must be non-negative.", file=sys.stderr)
        sys.exit(1)

    if args.frames < 1:
        print("Error: --frames must be at least 1.", file=sys.stderr)
        sys.exit(1)

    # Calculate time window
    now = datetime.now(timezone.utc)
    now_ts = now.timestamp()

    if args.since:
        try:
            since_dt = datetime.strptime(
                args.since, "%Y-%m-%d").replace(tzinfo=timezone.utc)
        except ValueError:
            print("Error: --since must be in YYYY-MM-DD format.",
                  file=sys.stderr)
            sys.exit(1)
        start_ts = since_dt.timestamp()
        days = max(1, (now - since_dt).days)
    elif args.compare:
        # Compare mode uses its own windowing
        days = args.compare
        start_ts = (now - timedelta(days=days)).timestamp()
    else:
        days = args.days
        start_ts = (now - timedelta(days=days)).timestamp()

    # Build query
    # For --compare, we request a larger limit to get good coverage
    query_limit = args.limit * 3 if args.compare else args.limit
    query = build_query(
        start_ts,
        now_ts,
        query_limit,
        platform=args.platform,
        version=args.version,
        channel=args.channel,
    )

    if args.dry_run:
        params_display = urllib.parse.urlencode({
            "universe": BACKTRACE_UNIVERSE,
            "project": args.project,
            "token": "<BACKTRACE_API_KEY>",
        })
        url_display = f"{BACKTRACE_ENDPOINT}/api/query?{params_display}"
        print(f"URL: POST {url_display}")
        print("\nQuery body:")
        print(json.dumps(query, indent=2))

        if args.compare:
            baseline_start = (now - timedelta(days=days * 2)).timestamp()
            baseline_end = start_ts
            baseline_query = build_query(
                baseline_start,
                baseline_end,
                query_limit,
                platform=args.platform,
                version=args.version,
                channel=args.channel,
            )
            print("\nBaseline query body:")
            print(json.dumps(baseline_query, indent=2))

        sys.exit(0)

    # Find brave source directory for code origin detection
    brave_src_path = find_brave_src(explicit_path=args.brave_src)
    if args.verbose:
        if brave_src_path:
            print(f"  Brave source: {brave_src_path}", file=sys.stderr)
        else:
            print("  Brave source not found, using namespace heuristics only",
                  file=sys.stderr)
    if args.brave_only and not brave_src_path:
        print(
            "Warning: --brave-only used but src/brave not found. "
            "Code origin detection will use namespace heuristics only. "
            "Use --brave-src to specify the path.",
            file=sys.stderr)

    # Fetch channel versions for version-to-channel mapping
    channel_versions = fetch_channel_versions(verbose=args.verbose)

    # Execute queries
    compare_mode = bool(args.compare)

    print(
        f"Querying Backtrace for top crashers in '{args.project}' "
        f"(last {days} days)...",
        file=sys.stderr)
    response = backtrace_query(args.project,
                               query,
                               api_key,
                               verbose=args.verbose)
    crashers = parse_response(
        response,
        days,
        args.frames,
        args.min_count,
        start_ts,
        args.project,
        channel_versions=channel_versions,
        brave_src_path=brave_src_path,
    )

    if args.verbose:
        print(f"  Parsed {len(crashers)} crash groups from response",
              file=sys.stderr)

    if args.compare:
        # Query baseline window
        baseline_start = (now - timedelta(days=days * 2)).timestamp()
        baseline_end = start_ts
        baseline_days = days

        print(f"Querying baseline window ({days}-{days * 2} days ago)...",
              file=sys.stderr)
        baseline_query = build_query(
            baseline_start,
            baseline_end,
            query_limit,
            platform=args.platform,
            version=args.version,
            channel=args.channel,
        )
        baseline_response = backtrace_query(
            args.project,
            baseline_query,
            api_key,
            verbose=args.verbose,
        )
        baseline_crashers = parse_response(
            baseline_response,
            baseline_days,
            args.frames,
            0,
            baseline_start,
            args.project,
            channel_versions=channel_versions,
            brave_src_path=brave_src_path,
        )

        if args.verbose:
            print(f"  Parsed {len(baseline_crashers)} baseline crash groups",
                  file=sys.stderr)

        crashers = compare_windows(crashers, baseline_crashers)
        # Trim to requested limit after comparison
        crashers = crashers[:args.limit]
    else:
        # Apply --new-only filter
        if args.new_only:
            crashers = [c for c in crashers if c.get("is_new")]

        # Sort and rank
        crashers = sort_crashers(crashers, args.order)

    # Apply post-query filters
    if args.crashes_only:
        crashers = [c for c in crashers if c.get("crash_type") != "dump"]
    if args.channels and args.channels.lower() != "all":
        selected_channels = [
            ch.strip().lower() for ch in args.channels.split(",")
        ]
        crashers = [
            c for c in crashers if any(
                c.get("channel_breakdown", {}).get(ch, 0) > 0
                for ch in selected_channels)
        ]
    if args.brave_only:
        crashers = [
            c for c in crashers if c.get("code_origin") in ("brave", "mixed")
        ]

    # Re-rank after filtering
    for i, c in enumerate(crashers, 1):
        c["rank"] = i

    if not crashers:
        print("No crashes found matching the criteria.", file=sys.stderr)
        sys.exit(3)

    print(f"Found {len(crashers)} crash groups.", file=sys.stderr)

    # Output
    if args.format == "markdown":
        print(format_markdown(crashers, days, compare_mode))
    elif args.format == "json":
        print(format_json_output(crashers, days, compare_mode))
    elif args.format == "ndjson":
        print(format_ndjson(crashers, compare_mode))
    elif args.format == "csv":
        print(format_csv_output(crashers, compare_mode))

    sys.exit(0)


if __name__ == "__main__":
    main()
