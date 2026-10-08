# Learned memory eval datasets

Version: 4

This folder has the labeled cases of the learned memory eval. All text is
synthetic. It has no real user data and no personal data of the team.
`validate_datasets.py` checks every file. Run it after each change.

```
 datasets/
   steps/<step>/dev.jsonl       tuning cases          (L1 step eval)
   steps/<step>/holdout.jsonl   test cases, frozen    (L1, final candidates only)
   steps/relevance/memories.jsonl   the memories of the relevance cases
   e2e/dev/<set>.json           chat sets with gold   (L2, run_eval.py)
   e2e/holdout/<set>.json
   parity/parity.json           reaches each request kind (parity_check.py)
   ambiguous/<step>.jsonl       cases that the label panel could not settle (not scored)
```

## 1. Rules

1. Never tune on a holdout. Run it only for a final candidate, and read only the totals.
2. Do not change a gold label to make a case pass. Fix a real label mistake in
   its own change, and add a changelog line with the reason.
3. Write new holdout cases before you look at any result for them.
4. A text must not be in both splits of a step.
5. An `id` is stable and unique in all files. Do not use an id again for
   another case.

## 2. Fields of all step cases

| Field | Value |
|---|---|
| `id` | `<step>-<split>-<n>`. Migrated cases have `m<NNN>`, generated cases `g<NNN>` |
| `split` | `dev` or `holdout`. It must agree with the file name |
| `tags` | List of coverage tags (section 4) |
| `origin` | `hand`, `generated:<batch>` or `migrated:<file>` |
| `note` | Optional. Why the label is what it is |

## 3. Step formats

| Step | Input fields | Gold |
|---|---|---|
| gate | `text`: a full user turn | `gold`: `keep`, `skip` or `either`. `either` is not scored |
| sentence | `text`, optional `turn` (the full turn) | `gold`: `fact` (bool), `safety`, `category` (or null), `temporary` (bool), `keep` (bool) |
| rewrite | `turn_date` (YYYY-MM-DD), `sentences` (list) | `gold.facts`: list of `{must, must_not, canonical}`. Optional `gold.max_facts`, `gold.date` |
| relation | `new`, `old`, optional `new_type`, `old_type` | `gold` (one relation), `acceptable` (list), `costly` (list) |
| merge | `old`, `new` | `gold.must`, `gold.must_not`, `canonical` |
| relevance | `message`, optional `previous_message`, `memories` (ids) | `must` (ids), `forbidden` (ids). Other ids are neutral |

Values:

- `safety`: `ok`, `sensitive`, `instruction`, `short_lived`, `not_about_user`.
- `category`: `personal_fact`, `preference`, `topic`, or null for a sentence that is not kept.
- relation: `different`, `same`, `replace`, `merge`, `same_topic`.
- `keep` is the wanted result of the pipeline: `fact` is true and `safety` is `ok`.

Word matches. A `must` or `must_not` item is a word or a list of words. A list
matches when one of its words matches (for example `["two", "second", "2"]`).
The match is a case-insensitive substring search. For a rewrite, the `must`
words of a gold fact are searched in the joined text of all facts of the case.
This is the rule of the first LLM eval, so the migrated numbers stay the same.

`memories.jsonl`: `{"id", "text", "origin", "tags"}`. A relevance case names
memories by id, not by a part of the text.

### Label rule: relation (new fact against an old fact)

```
 new fact vs old fact (same subject?)
   │
   ├─ not the same subject ................................... different
   │   ("Is learning Italian for a new job in Milan" vs "Works at Figma")
   ├─ same thing in other words .............................. same
   ├─ the new text says that the old fact ended .............. replace
   │   ("moved from", "new job at", "switched to", "no longer")
   ├─ both cannot be true at the same time, the new is later . replace
   │   (light mode vs dark mode, short vs detailed answers, first grade vs kindergarten)
   ├─ the new fact adds to the old fact ....................... merge
   │   (a second dog, one more language)
   └─ both can be true, or it is not clear .................... different
       (two jobs: "Works as a nurse" vs "Works as a teacher")
```

1. A replace puts the new text in place of the old text. A wrong replace
   deletes true information, and a wrong "different" gives one extra memory
   that the user can delete. So when it is not clear, the label is `different`.
2. A conflict is not enough for a replace. Two facts that look like a
   conflict (two jobs) stay as two memories, unless the text says that one
   ended or both cannot be true.
3. When `gold` is `different` or `same`, `replace` and `merge` are `costly`.
4. The product code agrees: a replace needs a certain decision model, and the
   LLM tie-break can only say `same` or `different`.

## 4. End-to-end gold

An e2e set has the `chats` and `expect` of `chat_sets/` (README, section 5),
and a `gold` key. `report.py` scores `expect` as today, and `gold` when it is
there.

```json
"gold": {
  "facts":     [{"id": "home_berlin", "canonical": "Lives in Berlin",
                 "match": {"all": ["berlin"], "any": ["live", "moved"]},
                 "required": true, "type": "long_term", "category": "personal_fact",
                 "sources": ["c3t0"]}],
  "stale":     [{"id": "home_sf", "match": {"all": ["san francisco"], "any": ["live"]},
                 "replaced_by": "home_berlin"}],
  "forbidden": [{"id": "health_diabetes", "match": {"any": ["diabetes", "insulin"]},
                 "sources": ["c5t0"]}],
  "ops":       [{"op": "replace", "old": "home_sf", "new": "home_berlin"},
                {"op": "merge", "facts": ["dog_luna", "dog_second"]}],
  "turns":     {"c3t0": ["home_berlin"], "c5t0": []}
}
```

- `c3t0` is chat 3, turn 0. The eval entry id is `eval-3-0-user`.
- `turns` lists every user turn, with the facts and the stale items that the
  turn gives. A stale item comes from a turn too: a later turn replaces it.
  An empty list means that the turn must give no memory.
- Each `sources` turn of a fact or a stale item must list that item in `turns`.
- `required: false` marks a fact that may be stored. It is not a miss, and it is not an extra.
- A `match` has `all` (every word) and `any` (at least one word), the same as `expect`.
- An op with `optional: true` may happen. It is not a missed op when it does
  not happen, and not a wrong op when it does. Use it for "may merge" pairs.
- `forbidden` is for leaks only (the hard gate). A short-lived fact that must
  not be stored (a dentist appointment) is not forbidden: it counts as an extra.
- A fact can have a `note`.

## 5. Coverage tags

| Step | Tags that each split must have (plan 7.3) |
|---|---|
| gate | `fact_and_question`, `task_only`, `long_turn`, `pasted_text`, `other_person`, two or more `lang:*` |
| sentence | `category:*` (3), `safety:*` (5), `health`, `diet_or_allergy`, `job_sounds_medical`, `weeks_long`, `instruction_or_style`, `not_about_user`, four or more `lang:*` |
| rewrite | `relative_date`, `several_sentences_one_fact`, `pronoun_subject`, `names`, `numbers`, `looks_like_prompt_example`, a `lang:*` other than `lang:en` |
| relation | `same`, `replace`, `merge`, `shared_word`, `temporary_vs_lasting`, `same_topic`, `preference_change`, `family`, `conflict_both_true` |
| merge | `count_change`, `place_detail`, `two_lists` |
| relevance | `direct`, `indirect`, `short_follow_up`, `style_wish`, `unrelated`, a `lang:*` other than `lang:en` |

The migrated cases got their tags from their labels and from simple text
rules. `validate_datasets.py --coverage` prints the table for each split, and
`--strict` fails when a tag is missing (the target of plan P1.3).

## 6. Changelog

| Version | Date | Change |
|---|---|---|
| 1 | 2026-10-08 | Migrated: `decision_cases.json` → gate dev (56), sentence dev (69). `decision_cases_holdout.json` → gate holdout (46), sentence holdout (36). `relevance_cases.json` → relevance dev (9 cases, 14 memories). Vault `eval/llm_eval.py` → rewrite dev (10), relation dev (14), merge dev (5). Labels unchanged. Relation: the first accepted answer is `gold`, all accepted answers are `acceptable`, and `costly` is `replace` and `merge` when they are not acceptable. The 7 chat sets of `chat_sets/` → `e2e/dev`, with `gold` from their `expect` and a reading of each turn. `chats` and `expect` are unchanged. Merges that the sets call "may merge" are `optional`. Facts that `expect` did not check, but that a turn states, are in the gold too (for example "Works remotely as a data engineer"). Known issue: `relations` names three jobs (Figma c19, nurse practitioner c23, a new job in Milan c1), but its description says one job. The gold keeps Figma and nurse practitioner as required, as `expect` did |
| 2 | 2026-10-08 | Added the relation label rule (section 3): a conflict without an end signal keeps both facts. Label fix, `relation-dev-m007` ("Works as a nurse" vs "Works as a teacher"): `replace` → `different`, costly `replace` and `merge`, tag `conflict_both_true`. Reason: two jobs can both be true, and no text says that the teaching job ended. The vault script had `replace`. `relations` e2e: the description no longer says "one home, one job", and the note of `job_figma` says why both jobs stay. No gold label of the e2e set changed. Jocelyn approved the rule |
| 3 | 2026-10-08 | Match fix, `life_events` `home_berlin`: `any` gets "moving". Reason: "Moving to Berlin in September 2026." (from c2t0, a source turn of the fact) was not a match, because "moving" does not contain "move". Found in the first L2 baseline. Scoring change, same release: a memory matches a gold fact or stale item only when it comes from one of its source turns (`lib/gold.py`). Leaks still match by words only |
| 4 | 2026-10-08 | Three optional facts, approved by Jocelyn: `life_events` `dog_beagle` (c1t0), `relations` `plays_drums` (c13t0), `temporary_states` `works_remotely` (c0t0). Reason: the rewrite split them off from a gold fact ("Owns a 3 year old beagle.", "Plays drums.", "Works remotely"). They are true, so they must not count as extras. Optional: no miss when they are not stored |
