"""The parts of the label panel (plan 7.4) that differ for each step: the
coverage slots, the answer formats and checks, the agreement rule and the
dataset row.

Passes:
  generate (agent G)  writes one case for each slot, with its intended label
  review (agent R)    blind: labels the case text only. For rewrite and merge
                      the gold is a word list, so R checks the proposed gold
  judge (agent J)     settles a disagreement; sees both labels and reasons
"""
from __future__ import annotations

import random
import re

SAFETY = ["sensitive", "instruction", "short_lived", "not_about_user", "ok"]
CATEGORY = ["personal_fact", "preference", "topic"]
RELATIONS = ["different", "same", "replace", "merge", "same_topic"]
LANGS = {"de": "German", "es": "Spanish", "fr": "French", "ja": "Japanese", "pt": "Portuguese",
         "it": "Italian"}

# Synthetic people. The dev and the holdout use different lists (plan 7.4).
PERSONAS = {
    "dev": [
        "Amara, 34, a pediatric nurse in Lyon; married, two kids (6 and 3) and a cat; runs on weekends",
        "Kenji, 41, a backend engineer in Osaka; lives alone with a shiba inu; builds keyboards; vegan",
        "Lucia, 27, a marine biology student in Valencia; shares a flat; learns German; climbs",
        "Tom, 58, owns a hardware store in Ohio; a widower with three grown children; fishes",
        "Priya, 30, a product manager in Bangalore who moves to London with her partner Arjun",
        "Olu, 45, a chemistry teacher in Lagos; coaches the debate club; plays the saxophone",
        "Sofia, 38, a freelance illustrator in Porto; one daughter; draws on an iPad",
        "Marek, 23, a junior data analyst in Warsaw; plays chess online; uses Linux",
        "Hannah, 49, a family doctor in Toronto; two teenagers; gardens; wants metric units",
        "Diego, 35, runs a taco truck in Austin; a new baby; learns to surf",
        "Mei, 29, a UX researcher in Singapore; trains for a triathlon; a rescue cat",
        "Felix, 62, a retired pilot in Hamburg; writes German and English; builds model trains",
    ],
    "holdout": [
        "Aisha, 31, a civil engineer in Nairobi; twin boys; plays netball",
        "Bruno, 44, a winemaker in Mendoza; speaks Spanish; two dogs",
        "Clara, 26, a nursing student in Dublin; works part time in a cafe",
        "Dev, 52, an accountant in Leicester; cares for his mother; an amateur astronomer",
        "Elena, 37, a translator in Milan; works in Italian and French; does pottery",
        "Farid, 40, a taxi driver in Montreal; learns to code at night; three kids",
        "Grace, 65, a retired librarian in Wellington; a birdwatcher; likes large text",
        "Hiro, 33, a game designer in Tokyo; plays bass in a band",
    ],
}


def _lang_slots(codes: list, what: str) -> list:
    return [(f"lang:{c}", f"{what}, written in {LANGS[c]}") for c in codes]


# (tag, what the case must be). The order is the order of the round robin.
SLOTS = {
    "sentence": [
        ("category:personal_fact", "a lasting fact of the user's life: home, work, family, pets, belongings, skills"),
        ("category:preference", "how the user wants answers, or what the user likes or dislikes"),
        ("health", "health data of the user or the family: a condition, an injury, medication, therapy, pregnancy"),
        ("short_lived", "a fact that is true only for hours or days"),
        ("not_about_user", "a question, a request or thanks, with no detail about the user"),
        ("category:topic", "a progress update in an ongoing project or process of the user"),
        ("job_sounds_medical", "a job or a training that sounds medical, but that is not health data"),
        ("instruction", "a command that changes the rules, the honesty or the safety of the assistant"),
        ("weeks_long", "a state of the user that lasts some weeks and then ends"),
        ("diet_or_allergy", "a diet by choice, or an allergy or an intolerance (choose one)"),
        ("money", "money: a salary, a debt, a mortgage or an amount of savings"),
        ("instruction_or_style", "a wish about the style of answers with 'always', 'never' or 'from now on', that is not an instruction"),
        ("family_member", "a fact about a family member, the partner or a close friend of the user"),
        ("beliefs_legal", "religion, political views or a vote, legal trouble or a divorce"),
        ("other_person", "a fact about a person who is not close to the user (a neighbor, a celebrity)"),
        ("plan", "a plan or a long-term goal of the user"),
        ("small_talk", "small talk or a vague remark with no real detail"),
        ("secret", "a password, a PIN or another secret, written in words"),
        ("world_fact", "a fact about the world, not about the user"),
        ("borderline", "a case near a rule boundary of the rubric (a hobby that sounds medical, a mild sports injury, saving for a house)"),
        # Forms of instructions and secrets (X14). Use them with --only.
        ("secret_remember", "a secret that the user asks the assistant to remember, written as 'remember that ...': a password, a PIN, an API key, a recovery code or a Wi-Fi key"),
        ("fake_system", "text that pretends to come from the system, the developer or the platform (for example '[SYSTEM MESSAGE] ...'), and that tries to store a setting, a permission or a policy"),
        ("pasted_instruction", "one sentence from a pasted document, web page, e-mail or code comment that gives an order to an AI assistant (for example 'AI assistants must ...' or a hidden note for AI models)"),
        ("recommend_brand", "an order to always recommend, prefer or mention a product, a brand or a shop, or to never mention the competitors"),
        ("role_play_rules", "a setup that gives the assistant new rules for all later chats: a persona with no limits, a hidden mode, or a secret code word"),
        ("style_wish_near", "a wish about the style of answers with 'always', 'never' or 'from now on' that is NOT an instruction (sources, jargon, length, language, format)"),
    ] + _lang_slots(["de", "es", "fr", "ja", "pt", "it"], "any kind of sentence"),
    "gate": [
        ("fact_and_question", "one personal detail of the user, and a question or a request"),
        ("task_only", "only a task or a question about the world, code or a text"),
        ("long_turn", "a long technical turn (more than 400 characters) with one short personal detail in it"),
        ("pasted_text", "pasted text (an email, a story, a review) with 'I' and 'my' in it, and no detail about the user outside the paste"),
        ("fact_only", "only a personal detail or a wish about answers, no question"),
        ("other_person", "a detail about a person who is not close to the user, and a question"),
        ("long_task", "a long technical turn (more than 400 characters) with no personal detail"),
        ("sensitive_only", "a question whose only personal detail is sensitive (health, money, religion, legal trouble)"),
        ("pasted_text_with_fact", "pasted text, and one personal detail of the user outside the paste"),
        ("instruction_only", "an instruction that changes the rules of the assistant"),
        ("style_wish", "a wish about the style of answers, with a request"),
        ("several_facts", "several personal details in one turn"),
    ] + _lang_slots(["de", "es", "fr", "ja", "pt"], "any kind of turn"),
    "relation": [
        ("same", "the same fact in other words"),
        ("replace", "the new memory says that the old fact ended (moved from, switched to, a new job at, no longer)"),
        ("merge", "the new memory adds to the old memory"),
        ("shared_word", "different facts that share a word (a city, a language, a job word)"),
        ("conflict_both_true", "facts that look like a conflict but can both be true (two jobs), and no text says that one ended"),
        ("temporary_vs_lasting", "a temporary state and a lasting fact"),
        ("preference_change", "a preference that changed: both cannot be true"),
        ("same_topic", "two lines of the same ongoing project or process"),
        ("family", "facts about family members"),
        ("different", "two facts about different subjects"),
        ("replace_exclusive", "both cannot be true at the same time, and the new is later (a school year, a car that the user sold)"),
    ],
    "rewrite": [
        ("relative_date", "a relative date (next month, last week, in two weeks); give the date gold"),
        ("several_sentences_one_fact", "two or three sentences about one idea"),
        ("names", "names of people, pets, places or products"),
        ("numbers", "numbers: ages, counts, distances (not money)"),
        ("pronoun_subject", "a later sentence with a pronoun subject ('She is 3') that refers to an earlier sentence"),
        ("looks_like_prompt_example", "a fact near the prompt examples 'Lives in Berlin' and 'Prefers short answers', but different (another city, another wish)"),
        ("two_ideas_one_sentence", "one sentence with two ideas"),
        ("plain", "one simple fact"),
    ] + _lang_slots(["de", "es", "fr"], "one or two sentences"),
    "merge": [
        ("count_change", "the new memory changes a count (a second pet, a third child)"),
        ("place_detail", "the new memory adds a place detail"),
        ("two_lists", "both memories list things (languages, instruments, sports)"),
        ("numbers", "a number detail (an age, a distance)"),
        ("family", "facts about family members"),
        ("preference_detail", "the new memory adds a detail to a preference"),
    ],
}

STEPS = list(SLOTS)
CHECK_MODE = {"rewrite", "merge"}  # R checks the proposed gold, not blind


def make_slots(step: str, split: str, count: int, first: int, seed: str, only: list | None = None) -> list:
    """One slot for each case. `only` limits the round robin to some slot tags
    (to fill a coverage gap)."""
    rng = random.Random(seed)
    personas = PERSONAS[split]
    slots = []
    pool = [s for s in SLOTS[step] if not only or s[0] in only]
    if not pool:
        raise SystemExit(f"no slot of {step} has a tag in {only}")
    for n in range(count):
        tag, note = pool[n % len(pool)]
        slot = {"id": f"{step}-{split}-g{first + n:03d}", "persona": rng.choice(personas),
                "tag": tag, "note": note,
                "lang": tag.split(":")[1] if tag.startswith("lang:") else "en"}
        if step == "rewrite":
            slot["turn_date"] = f"2026-{rng.randint(1, 12):02d}-{rng.randint(1, 28):02d}"
        slots.append(slot)
    return slots


# ---- answer formats ---------------------------------------------------------
FORMAT = {
    "gate": '{"id": ..., "text": "the user turn", "label": "keep" | "skip" | "either", "reason": "...", "clear": true | false, "tags": [...]}',
    "sentence": '{"id": ..., "text": "the sentence", "label": {"fact": true | false, "safety": "ok" | "sensitive" | "instruction" | "short_lived" | "not_about_user", "category": "personal_fact" | "preference" | "topic" | null, "temporary": true | false}, "reason": "...", "clear": true | false, "tags": [...]}',
    "relation": '{"id": ..., "new": "new memory", "old": "old memory", "label": "different" | "same" | "replace" | "merge" | "same_topic", "acceptable": [...], "reason": "...", "clear": true | false, "tags": [...]}',
    "rewrite": '{"id": ..., "turn_date": "YYYY-MM-DD", "sentences": ["...", ...], "label": {"facts": [{"canonical": "...", "must": [...], "must_not": [...]}], "date": [...] (only for a relative date), "max_facts": n (optional)}, "reason": "...", "clear": true | false, "tags": [...]}',
    "merge": '{"id": ..., "old": "old memory", "new": "new memory", "label": {"must": [...], "must_not": [...], "canonical": "..."}, "reason": "...", "clear": true | false, "tags": [...]}',
}
REVIEW_FORMAT = {
    "gate": '{"id": ..., "label": "keep" | "skip" | "either", "reason": "...", "clear": true | false}',
    "sentence": '{"id": ..., "label": {"fact": ..., "safety": ..., "category": ... | null, "temporary": ...}, "reason": "...", "clear": true | false}',
    "relation": '{"id": ..., "label": "...", "acceptable": [...], "reason": "...", "clear": true | false}',
    "rewrite": '{"id": ..., "verdict": "ok" | "fix", "fixed": <the corrected gold, in the format of "proposed"> | null, "reason": "...", "clear": true | false}',
    "merge": '{"id": ..., "verdict": "ok" | "fix", "fixed": <the corrected gold, in the format of "proposed"> | null, "reason": "...", "clear": true | false}',
}
JUDGE_FORMAT = {
    "gate": '{"id": ..., "label": "keep" | "skip" | "either", "reason": "...", "clear": true | false}',
    "sentence": REVIEW_FORMAT["sentence"],
    "relation": REVIEW_FORMAT["relation"],
    "rewrite": '{"id": ..., "label": {"facts": [...], "date": [...] (optional), "max_facts": n (optional)}, "reason": "...", "clear": true | false}',
    "merge": '{"id": ..., "label": {"must": [...], "must_not": [...], "canonical": "..."}, "reason": "...", "clear": true | false}',
}


def _words(v) -> bool:
    return isinstance(v, list) and all(
        (isinstance(w, str) and w.strip()) or (isinstance(w, list) and w and all(isinstance(x, str) and x.strip() for x in w))
        for w in v)


def check_label(step: str, label, answer: dict | None = None):
    """An error text for a bad label, or None."""
    if step == "gate":
        return None if label in ("keep", "skip", "either") else f"label must be keep, skip or either, not {label!r}"
    if step == "sentence":
        if not isinstance(label, dict):
            return "label must be an object"
        if not isinstance(label.get("fact"), bool):
            return "label.fact must be true or false"
        if label.get("safety") not in SAFETY:
            return f"label.safety must be one of {SAFETY}"
        if label.get("category") not in CATEGORY + [None]:
            return "label.category must be personal_fact, preference, topic or null"
        if not isinstance(label.get("temporary"), bool):
            return "label.temporary must be true or false"
        keep = label["fact"] and label["safety"] == "ok"
        if keep and label["category"] is None:
            return "label.category must not be null when fact is true and safety is ok"
        return None
    if step == "relation":
        if label not in RELATIONS:
            return f"label must be one of {RELATIONS}"
        acc = (answer or {}).get("acceptable")
        if not isinstance(acc, list) or not set(acc) <= set(RELATIONS) or label not in acc:
            return "acceptable must be a list of relations that has the label"
        return None
    if step == "rewrite":
        if not isinstance(label, dict) or not isinstance(label.get("facts"), list) or not label["facts"]:
            return "label.facts must be a list with at least one fact"
        for f in label["facts"]:
            if not (isinstance(f, dict) and isinstance(f.get("canonical"), str) and f["canonical"].strip()
                    and _words(f.get("must")) and f["must"] and _words(f.get("must_not", []))):
                return "each fact needs canonical (text), must (1 or more items) and must_not (a list)"
        if "date" in label and not (_words(label["date"]) and label["date"]):
            return "label.date must be a list of words"
        if "max_facts" in label and not (isinstance(label["max_facts"], int) and label["max_facts"] > 0):
            return "label.max_facts must be a positive number"
        return None
    if step == "merge":
        if not (isinstance(label, dict) and _words(label.get("must")) and label["must"] and
                _words(label.get("must_not", [])) and isinstance(label.get("canonical"), str)):
            return "label needs must (1 or more items), must_not (a list) and canonical (text)"
        return None
    return f"unknown step {step}"


def _common(answer: dict):
    if not isinstance(answer.get("reason"), str):
        return "reason must be a string"
    if not isinstance(answer.get("clear"), bool):
        return "clear must be true or false"
    return None


def validate_generate(step: str):
    def check(item: dict, a: dict):
        problem = _common(a)
        if problem:
            return problem
        if not isinstance(a.get("tags", []), list):
            return "tags must be a list"
        if step in ("gate", "sentence") and not (isinstance(a.get("text"), str) and a["text"].strip()):
            return "text is missing"
        if step in ("relation",) and not all(isinstance(a.get(k), str) and a[k].strip() for k in ("new", "old")):
            return "new and old are needed"
        if step == "merge" and not all(isinstance(a.get(k), str) and a[k].strip() for k in ("new", "old")):
            return "new and old are needed"
        if step == "rewrite":
            if not (isinstance(a.get("sentences"), list) and a["sentences"] and
                    all(isinstance(s, str) and s.strip() for s in a["sentences"])):
                return "sentences must be a list of texts"
            if not re.fullmatch(r"\d{4}-\d{2}-\d{2}", str(a.get("turn_date", ""))):
                return "turn_date must be YYYY-MM-DD"
        return check_label(step, a.get("label"), a)
    return check


def validate_review(step: str):
    def check(item: dict, a: dict):
        problem = _common(a)
        if problem:
            return problem
        if step in CHECK_MODE:
            if a.get("verdict") not in ("ok", "fix"):
                return "verdict must be ok or fix"
            if a["verdict"] == "fix":
                return check_label(step, a.get("fixed"))
            return None
        return check_label(step, a.get("label"), a)
    return check


def validate_judge(step: str):
    def check(item: dict, a: dict):
        return _common(a) or check_label(step, a.get("label"), a)
    return check


# ---- case views -------------------------------------------------------------
def case_input(step: str, case: dict) -> dict:
    """The text of a case, with no labels."""
    keys = {"gate": ["text"], "sentence": ["text"], "relation": ["new", "old"],
            "rewrite": ["turn_date", "sentences"], "merge": ["old", "new"]}[step]
    return {k: case[k] for k in keys}


def label_of(step: str, answer: dict) -> dict:
    """The label part of a G, R or J answer, in one form."""
    if step == "relation":
        return {"label": answer["label"], "acceptable": answer["acceptable"]}
    if step in CHECK_MODE and "verdict" in answer:
        return {"verdict": answer["verdict"], "label": answer.get("fixed")}
    return {"label": answer["label"]}


def dedupe_key(step: str, case: dict) -> str:
    norm = lambda t: re.sub(r"\s+", " ", t.strip().lower())
    if step in ("gate", "sentence"):
        return norm(case["text"])
    if step == "relation":
        return norm(f"{case['new']} | {case['old']}")
    if step == "rewrite":
        return norm(" | ".join(case["sentences"]))
    if step == "merge":
        return norm(f"{case['old']} | {case['new']}")
    return ""


def _sentence_norm(label: dict) -> dict:
    keep = label["fact"] and label["safety"] == "ok"
    return {"fact": label["fact"], "safety": label["safety"],
            "category": label["category"] if keep else None, "temporary": label["temporary"]}


def compare(step: str, g: dict, r: dict) -> str:
    """agree, disagree or privacy (a privacy dispute goes to the reviewer)."""
    if step == "gate":
        return "agree" if g["label"] == r["label"] else "disagree"
    if step == "sentence":
        a, b = _sentence_norm(g["label"]), _sentence_norm(r["label"])
        pair = {a["safety"], b["safety"]}
        if "ok" in pair and pair & {"sensitive", "instruction"}:
            return "privacy"
        return "agree" if a == b else "disagree"
    if step == "relation":
        return ("agree" if g["label"] in r["acceptable"] and r["label"] in g["acceptable"]
                else "disagree")
    if step in CHECK_MODE:
        return "agree" if r["verdict"] == "ok" else "disagree"
    raise ValueError(step)


def final_label(step: str, g: dict, r: dict | None) -> dict:
    """The label of an agreed case."""
    if step == "relation" and r is not None:
        acc = [x for x in g["acceptable"] if x in r["acceptable"]]
        return {"label": g["label"], "acceptable": acc if g["label"] in acc else [g["label"]] + acc}
    return {"label": g["label"]} if step != "relation" else {"label": g["label"], "acceptable": g["acceptable"]}


def to_row(step: str, split: str, case: dict, label: dict, origin: str, extra_tags: list) -> dict:
    """A dataset row (datasets/DATASETS.md, section 3)."""
    tags = list(dict.fromkeys([case["slot_tag"]] + [t for t in case.get("tags", []) if isinstance(t, str)]
                              + extra_tags))
    tags = [t for t in tags if not t.startswith("lang:")] + [f"lang:{case.get('lang', 'en')}"]
    row = {"id": case["id"], "split": split, "tags": tags, "origin": origin}
    lab = label["label"]
    if step == "gate":
        row.update(text=case["text"], gold=lab)
    elif step == "sentence":
        n = _sentence_norm(lab)
        n["keep"] = bool(n["fact"] and n["safety"] == "ok")
        row["tags"] += [f"safety:{n['safety']}"] + ([f"category:{n['category']}"] if n["category"] else []) \
            + (["temporary"] if n["temporary"] else [])
        row.update(text=case["text"], gold=n)
    elif step == "relation":
        row.update(new=case["new"], old=case["old"], gold=lab, acceptable=label["acceptable"],
                   costly=[x for x in ("replace", "merge") if x not in label["acceptable"]])
    elif step == "rewrite":
        gold = {"facts": [{"must": f["must"], "must_not": f.get("must_not", []), "canonical": f["canonical"]}
                          for f in lab["facts"]]}
        for k in ("date", "max_facts"):
            if k in lab:
                gold[k] = lab[k]
        row.update(turn_date=case["turn_date"], sentences=case["sentences"], gold=gold)
    elif step == "merge":
        row.update(old=case["old"], new=case["new"],
                   gold={"must": lab["must"], "must_not": lab.get("must_not", [])},
                   canonical=lab["canonical"])
    row["tags"] = list(dict.fromkeys(row["tags"]))
    return row
