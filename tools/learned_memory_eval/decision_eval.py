#!/usr/bin/env python3
"""Compares wordings of the decision model questions on labeled sentences.

Calls Ollama (/v1/systemone) directly, so a test takes about a minute and
needs no browser. Cases: datasets/steps/{sentence,gate}/<split>.jsonl (see
README.md, section 10). The wordings are in lib/requests.py.
Usage:
  ./decision_eval.py                       # all variants, summary table
  ./decision_eval.py --variants A D        # some variants
  ./decision_eval.py --errors D            # list the wrong cases of variant D
  ./decision_eval.py --split holdout       # the holdout cases
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
import urllib.request
from pathlib import Path

import ollama_setup
from lib import requests as rq

HERE = Path(__file__).resolve().parent
CACHE = Path("/tmp/decision_eval_cache.json")
SAFETY = ["sensitive", "instruction", "short_lived", "not_about_user", "ok"]
CATEGORY = ["personal_fact", "preference", "topic"]


# ---- wordings (lib/requests.py) ---------------------------------------------
def _dicts(questions: dict) -> dict:
    return {k: q.as_dict() for k, q in questions.items()}


GATE = _dicts(rq.GATE)
FACT = _dicts(rq.FACT)
SAFETY_CHOICE = _dicts(rq.SAFETY)
SAFETY_SPLIT = _dicts(rq.SAFETY_SPLIT)
CATEGORY_Q = _dicts(rq.CATEGORY)
TEMP = _dicts(rq.TEMPORARY)


# Variants: which wording of each question, and the keep rule.
VARIANTS = {
    "A": dict(desc="current wording", gate="G0", fact="F0", safety="S0", cat="C0", temp="T0"),
    "B": dict(desc="criteria on gate and fact", gate="G1", fact="F1", safety="S0", cat="C0", temp="T0"),
    "C": dict(desc="self-statement fact, richer safety choice", gate="G1", fact="F2", safety="S1", cat="C1", temp="T1"),
    "D": dict(desc="self-statement fact, split safety questions", gate="G1", fact="F2", safety="split", cat="C1", temp="T1"),
    "E": dict(desc="style wishes count as facts, clear instruction rule", gate="G1", fact="F3", safety="S2", cat="C1", temp="T1"),
    "F": dict(desc="E with the safety wording of C", gate="G1", fact="F3", safety="S1", cat="C1", temp="T1"),
    "G": dict(desc="F3 fact, original safety, new category and temporary", gate="G1", fact="F3", safety="S0", cat="C1", temp="T1"),
    "I": dict(desc="F3 fact, S0 with a clearer instruction option", gate="G1", fact="F3", safety="S3", cat="C1", temp="T1"),
    "K": dict(desc="I with gate G2 (any personal detail)", gate="G2", fact="F3", safety="S3", cat="C1", temp="T1"),
    "L": dict(desc="I with gate G3 (ignore the question)", gate="G3", fact="F3", safety="S3", cat="C1", temp="T1"),
    "M": dict(desc="K with tastes in the gate (G4)", gate="G4", fact="F3", safety="S3", cat="C1", temp="T1"),
    "J": dict(desc="F1 fact, S0 with a clearer instruction option", gate="G1", fact="F1", safety="S3", cat="C1", temp="T1"),
    "H": dict(desc="F1 fact (B), new category and temporary", gate="G1", fact="F1", safety="S0", cat="C1", temp="T1"),
}


def sentence_questions(v):
    q = {"fact": FACT[v["fact"]], "category": CATEGORY_Q[v["cat"]],
         "temporary": TEMP[v["temp"]]}
    if v["safety"] == "split":
        q.update({f"safety_{k}": val for k, val in SAFETY_SPLIT.items()})
    else:
        q["safety"] = SAFETY_CHOICE[v["safety"]]
    return q


# ---- model calls ------------------------------------------------------------
_cache = json.loads(CACHE.read_text()) if CACHE.exists() else {}


def ask(args, state, questions):
    body = {"model": args.model, "state": state, "questions": questions}
    key = hashlib.sha1(json.dumps(body, sort_keys=True).encode()).hexdigest()
    if key not in _cache:
        req = urllib.request.Request(f"{args.ollama}/v1/systemone",
                                     json.dumps(body).encode(),
                                     {"Content-Type": "application/json"})
        _cache[key] = json.load(urllib.request.urlopen(req, timeout=600))["answers"]
        CACHE.write_text(json.dumps(_cache))
    return _cache[key]


def extract(v, answers):
    """Turns the model answers into probabilities of the pipeline."""
    out = {"fact": answers["fact"]["noul"], "temporary": answers["temporary"]["noul"],
           "category": answers["category"]["probabilities"]}
    if v["safety"] == "split":
        p = {k: answers[f"safety_{k}"]["noul"] for k in SAFETY_SPLIT}
        out["safety"] = {**p, "not_about_user": 0.0,
                         "ok": 1.0 - max(p.values())}
    else:
        out["safety"] = answers["safety"]["probabilities"]
    return out


def certain(probs, t, m):
    items = sorted(probs.items(), key=lambda kv: -kv[1])
    top = items[0]
    nxt = items[1][1] if len(items) > 1 else 0.0
    return top[0] if top[1] + 1e-9 >= t and top[1] - nxt + 1e-9 >= m else None


# ---- keep rules -------------------------------------------------------------
def keep_current(x, t, m, _u):
    """The rule in DreamingRun today: fact certain yes AND safety certain ok."""
    return (certain({"yes": x["fact"], "no": 1 - x["fact"]}, t, m) == "yes" and
            certain(x["safety"], t, m) == "ok")


def keep_asym(x, t, _m, u):
    """Fact likely yes, and no unsafe answer reaches u (fail-safe)."""
    unsafe = max(x["safety"].get(k, 0) for k in ("sensitive", "instruction", "short_lived"))
    return x["fact"] >= t and unsafe < u


def keep_asym2(x, t, _m, _u):
    """Fact likely yes, and each unsafe answer is below its own limit."""
    s = x["safety"]
    return (x["fact"] >= t and s.get("sensitive", 0) < 0.25 and
            s.get("instruction", 0) < 0.3 and s.get("short_lived", 0) < 0.5)


def keep_final(x, t, _m, _u):
    """The rule that DreamingRun uses: fact likely yes, and each unsafe answer
    is below its own limit. Sensitive data has the lowest limit."""
    s = x["safety"]
    return (x["fact"] >= t and s.get("sensitive", 0) < 0.3 and
            s.get("instruction", 0) < 0.3 and s.get("short_lived", 0) < 0.5)


RULES = [("FINAL fact>=0.6 (sens<.3 instr<.3 short<.5)", keep_final, 0.6, 0, 0),
         ("current 0.7/0.2", keep_current, 0.7, 0.2, 0),
         ("current 0.6/0.2", keep_current, 0.6, 0.2, 0),
         ("asym fact>=0.6 unsafe<0.3", keep_asym, 0.6, 0, 0.3),
         ("asym fact>=0.5 unsafe<0.3", keep_asym, 0.5, 0, 0.3),
         ("asym fact>=0.5 unsafe<0.2", keep_asym, 0.5, 0, 0.2),
         ("asym2 fact>=0.6 (sens<.25 instr<.3 short<.5)", keep_asym2, 0.6, 0, 0),
         ("asym2 fact>=0.5 (sens<.25 instr<.3 short<.5)", keep_asym2, 0.5, 0, 0)]


def run_variant(args, name, cases):
    v = VARIANTS[name]
    questions = sentence_questions(v)
    rows = []
    for c in cases["sentences"]:
        rows.append((c, extract(v, ask(args, c["text"], questions))))
    gates = []
    for g in cases["gates"]:
        gates.append((g, ask(args, g["text"], {"gate": GATE[v["gate"]]})["gate"]["noul"]))
    return rows, gates


def evaluate(rows, gates):
    keep_labels = [c["fact"] and c["safety"] == "ok" for c, _ in rows]
    n_keep = sum(keep_labels)
    res = {}
    for label, rule, t, m, u in RULES:
        kept = [rule(x, t, m, u) for _, x in rows]
        hit = sum(1 for k, want in zip(kept, keep_labels) if k and want)
        false = [(c, x) for (c, x), k, want in zip(rows, kept, keep_labels) if k and not want]
        unsafe = [c["text"] for c, _ in false if c["safety"] in ("sensitive", "instruction")]
        res[label] = dict(recall=f"{hit}/{n_keep}", false=len(false), unsafe=len(unsafe),
                          unsafe_texts=unsafe,
                          missed=[c["text"] for (c, _), k, want in zip(rows, kept, keep_labels) if want and not k],
                          false_texts=[(c["text"], c["safety"]) for c, _ in false])
    should = [(c, x) for (c, x), w in zip(rows, keep_labels) if w]
    res["fact_acc"] = sum((x["fact"] > 0.5) == c["fact"] for c, x in rows) / len(rows)
    res["cat_acc"] = sum(max(x["category"], key=x["category"].get) == c["category"] for c, x in should) / len(should)
    res["temp_acc"] = sum((x["temporary"] > 0.5) == c["temporary"] for c, x in should) / len(should)
    res["safety_acc"] = sum(max(x["safety"], key=x["safety"].get) == c["safety"] for c, x in rows) / len(rows)
    for t in (0.7, 0.5):
        keep = [(g, p) for g, p in gates if g["gate"] == "keep"]
        skip = [(g, p) for g, p in gates if g["gate"] == "skip"]
        res[f"gate@{t}"] = (f"{sum(p >= t for _, p in keep)}/{len(keep)} keep, "
                            f"{sum(p >= t for _, p in skip)}/{len(skip)} wrong keeps")
    keep_p = [p for g, p in gates if g["gate"] == "keep"]
    skip_p = [p for g, p in gates if g["gate"] == "skip"]
    res["gate_range"] = (f"keep min {min(keep_p):.2f} median {sorted(keep_p)[len(keep_p)//2]:.2f}; "
                         f"skip max {max(skip_p):.2f}")
    for t in (0.3, 0.2, 0.1):
        res[f"gate@{t}"] = (f"{sum(p >= t for p in keep_p)}/{len(keep_p)} keep, "
                            f"{sum(p >= t for p in skip_p)}/{len(skip_p)} wrong keeps")
    res["gate_missed@0.5"] = [g["text"][:70] for g, p in gates if g["gate"] == "keep" and p < 0.5]
    res["gate_false@0.5"] = [g["text"][:70] for g, p in gates if g["gate"] == "skip" and p >= 0.5]
    return res


def load_split(split: str) -> dict:
    """The step datasets in the old format of this script."""
    def rows(step):
        path = HERE / "datasets" / "steps" / step / f"{split}.jsonl"
        return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]
    sentences = [{"text": c["text"], "fact": c["gold"]["fact"], "safety": c["gold"]["safety"],
                  "temporary": c["gold"]["temporary"], "category": c["gold"]["category"]}
                 for c in rows("sentence")]
    gates = [{"text": c["text"], "gate": c["gold"]} for c in rows("gate")]
    return {"sentences": sentences, "gates": gates}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--variants", nargs="*", default=list(VARIANTS))
    parser.add_argument("--errors", metavar="VARIANT")
    parser.add_argument("--ollama", default=ollama_setup.DEFAULT_OLLAMA)
    parser.add_argument("--model", default=ollama_setup.DEFAULT_DECISION)
    parser.add_argument("--keep-alive", default=ollama_setup.DEFAULT_KEEP_ALIVE)
    parser.add_argument("--no-warmup", action="store_true",
                        help="do not load the decision model before the test")
    parser.add_argument("--split", choices=["dev", "holdout"], default="dev",
                        help="datasets/steps/{sentence,gate}/<split>.jsonl")
    parser.add_argument("--cases", default=None,
                        help="an old case file ({\"sentences\": [...], \"gates\": [...]}) "
                             "instead of --split")
    args = parser.parse_args()
    try:
        if not args.no_warmup:
            ollama_setup.load_models(args.ollama, args.model, None, args.keep_alive)
    except ollama_setup.OllamaSetupError as error:
        sys.exit(str(error))
    cases = (json.loads((HERE / args.cases).read_text()) if args.cases
             else load_split(args.split))
    results = {}
    for name in args.variants:
        print(f"running {name}: {VARIANTS[name]['desc']}", file=sys.stderr, flush=True)
        results[name] = evaluate(*run_variant(args, name, cases))

    n_keep = sum(c["fact"] and c["safety"] == "ok" for c in cases["sentences"])
    print(f"\n{len(cases['sentences'])} sentences ({n_keep} should be kept), "
          f"{len(cases['gates'])} gate turns. 'unsafe' = a sensitive or instruction "
          f"sentence that was kept (must be 0).\n")
    print("| Variant | Rule | Kept (recall) | Wrong keeps | Unsafe keeps |")
    print("|---|---|---|---|---|")
    for name, r in results.items():
        for label, *_ in RULES:
            print(f"| {name} | {label} | {r[label]['recall']} | {r[label]['false']} | {r[label]['unsafe']} |")
    print("\n| Variant | Fact acc | Safety acc | Category acc | Temporary acc | Gate @0.7 | Gate @0.5 |")
    print("|---|---|---|---|---|---|---|")
    for name, r in results.items():
        print(f"| {name} | {r['fact_acc']:.2f} | {r['safety_acc']:.2f} | {r['cat_acc']:.2f} | "
              f"{r['temp_acc']:.2f} | {r['gate@0.7']} | {r['gate@0.5']} |")
    print("\n| Variant | Gate range | @0.3 | @0.2 | @0.1 |")
    print("|---|---|---|---|---|")
    for name, r in results.items():
        print(f"| {name} | {r['gate_range']} | {r['gate@0.3']} | {r['gate@0.2']} | {r['gate@0.1']} |")
    if args.errors:
        r = results[args.errors]
        for label, *_ in RULES:
            print(f"\n### {args.errors}, rule {label}")
            print("missed:", *[f"\n  - {t}" for t in r[label]["missed"]])
            print("wrong keeps:", *[f"\n  - {t} [{s}]" for t, s in r[label]["false_texts"]])
        print("\ngate missed @0.5:", *[f"\n  - {t}" for t in r["gate_missed@0.5"]])
        print("gate wrong keeps @0.5:", *[f"\n  - {t}" for t in r["gate_false@0.5"]])


if __name__ == "__main__":
    main()
