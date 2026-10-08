"""Shared scoring helpers: proportions with Wilson intervals, word matches,
latency summaries and paired flips (plan section 8)."""
from __future__ import annotations

import math
import statistics


def wilson(k: int, n: int, z: float = 1.96) -> tuple:
    """The Wilson 95% interval of k/n."""
    if n == 0:
        return (0.0, 1.0)
    p = k / n
    d = 1 + z * z / n
    c = (p + z * z / (2 * n)) / d
    h = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / d
    return (max(0.0, c - h), min(1.0, c + h))


def rate(k: int, n: int) -> dict:
    lo, hi = wilson(k, n)
    return {"k": k, "n": n, "p": (k / n) if n else None, "lo": lo, "hi": hi}


def fmt_rate(r: dict | None, interval: bool = True) -> str:
    if not r or not r["n"]:
        return "n/a"
    text = f"{r['k']}/{r['n']} ({r['p']:.2f})"
    if interval:
        text += f" [{r['lo']:.2f}, {r['hi']:.2f}]"
    return text


def word_matches(item, text: str) -> bool:
    """A word, or a list of words of which one must match. Case-insensitive
    substring search (datasets/DATASETS.md, section 3)."""
    t = text.lower()
    if isinstance(item, list):
        return any(w.lower() in t for w in item)
    return item.lower() in t


def all_words(items: list, text: str) -> bool:
    return all(word_matches(i, text) for i in items)


def any_words(items: list, text: str) -> list:
    """The items that match."""
    return [i for i in items if word_matches(i, text)]


def latency(values: list) -> dict:
    v = sorted(x for x in values if x is not None)
    if not v:
        return {"n": 0, "median_ms": None, "p95_ms": None, "total_s": 0}
    p95 = v[min(len(v) - 1, int(math.ceil(0.95 * len(v))) - 1)]
    return {"n": len(v), "median_ms": statistics.median(v), "p95_ms": p95,
            "total_s": round(sum(v) / 1000, 1)}


def fmt_latency(lat: dict) -> str:
    if not lat["n"]:
        return "n/a"
    return f"median {lat['median_ms']:.0f} ms, p95 {lat['p95_ms']:.0f} ms"


def case_outcomes(records: list) -> dict:
    """Per case id: True when at least half of the repeats are ok, False when
    fewer, None when no repeat is scored. Records have "id" and "ok"."""
    by_id: dict = {}
    for r in records:
        if r.get("ok") is None:
            continue
        by_id.setdefault(r["id"], []).append(bool(r["ok"]))
    return {cid: sum(v) * 2 >= len(v) for cid, v in by_id.items()}


def flips(base: list, new: list) -> dict:
    """Paired flips of the primary outcome: cases that the new variant fixed
    and broke, against the base variant."""
    a, b = case_outcomes(base), case_outcomes(new)
    common = sorted(set(a) & set(b))
    return {"fixed": [c for c in common if not a[c] and b[c]],
            "broken": [c for c in common if a[c] and not b[c]],
            "paired": len(common)}
