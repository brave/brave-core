# Learned memory eval harness

This harness runs Dreaming (learned memory) in a real Brave build on chat sets
that we write. It shows what Dreaming stores, and it logs the input and the
output of each step: the decision model, the local LLM and the embedder.

Code: `brave/tools/learned_memory_eval/`. Design: `impl.md` in the notes folder.

## 1. How it works

```
chat_sets/<name>.json ──► run_eval.py
                            │ 1. check Ollama and the build
                            │ 2. make a new profile (same files each time)
                            │ 3. start Brave with the eval switches
                            ▼
   ┌──────────────────────── Brave (new profile) ───────────────────────┐
   │ UserMemoryManager (eval mode)                                       │
   │   import the chats into AIChatDatabase                              │
   │   run Dreaming one time, trace on                                   │
   │      gate ─► sentences ─► decisions ─► rewrite ─► embed ─► relation │
   │      ─► merge ─► store            (each call is written to trace)   │
   │   write report.json, then wait                                      │
   └──────────────────────────────────────────────────────────────────────┘
                            │ 4. read report.json, stop Brave
                            ▼
              out/<time>/<set>/report.json  trace + memories
                             report.md    readable report
                             brave.log    browser log
              out/<time>/summary.md       one table for all sets
```

The harness does not mock any model. A run uses the same code as the daily
timer: `OllamaDecisionClient` (clef-flash), the BYOM engine (qwen3.5:9b) and
the in-browser EmbeddingGemma. Only the chats are different: the harness puts
them in the database, so there is no need to type them in Leo.

## 2. Requirements

| Item | Value |
|---|---|
| Build | `pnpm build` with `enable_local_ai` on. The default is `out/Component_arm64` |
| Ollama | Running on `http://localhost:11434` |
| Decision model | `clef-flash:9b` pulled in Ollama |
| Local LLM | `qwen3.5:9b` pulled in Ollama |
| Embedder files | The component `BraveLocalAIModels` in the dev profile. The harness copies it to each new profile |
| Python | 3.9 or newer. No packages |

## 3. Run it

```sh
cd brave/tools/learned_memory_eval
./run_eval.py --list                     # show the chat sets
./run_eval.py                            # run all sets
./run_eval.py --sets life_events task_only
./run_eval.py --threshold 0.8            # certain_threshold of the decisions
./run_eval.py --dry-run --sets task_only # make the profile, print the command
./run_eval.py --report-only /tmp/learned_memory_eval/<time>   # new report.md
```

| Option | Default | Meaning |
|---|---|---|
| `--sets` | all | Names of chat sets |
| `--brave` | `out/Component_arm64/Brave Browser Development.app/...` | Browser binary |
| `--out` | `/tmp/learned_memory_eval/<time>` | Output folder |
| `--source-profile` | dev profile folder | Where the embedder component is copied from |
| `--ollama` | `http://localhost:11434` | Ollama URL |
| `--decision` | `clef-flash:9b` | Decision model name |
| `--llm` | `qwen3.5:9b` | Local LLM name (the BYOM model) |
| `--threshold`, `--margin` | `0.8`, `0.2` | Certainty rule of the decision model. It is only for the labels (category, type) and the relation. The gate and the keep rules have their own params (`gate_threshold`, `fact_threshold`) |
| `--time-limit` | `600` | Run time limit in the browser, in seconds |
| `--timeout` | `1200` | The harness stops waiting after this time, in seconds |
| `--no-warmup` | off | Do not load the Ollama models before the run |
| `--keep-profiles` | off | Do not delete the profile after the run |

Each set takes about 2 to 15 minutes. The embedder and the Ollama models load
at the start, and the LLM runs for each turn that passes the gate.

## 4. The new profile

The harness makes a new profile for each set, so a run never depends on an
earlier run. These are the steps and the files it writes.

| What | Where | Value |
|---|---|---|
| Profile folder | `--user-data-dir` | `<out>/<set>/profile` |
| Embedder component | `<profile>/BraveLocalAIModels` | Copy of the dev profile files (APFS clone) |
| Custom model (BYOM) | `Default/Preferences`, `brave.ai_chat.custom_models` | Key `custom:e0a1b2c3`, name = `--llm`, endpoint `<ollama>/v1/chat/completions` |
| Chat history | `brave.ai_chat.storage_enabled` | `true` |
| Memory | `brave.ai_chat.user_memory_enabled` | `true` |
| Leo opt-in | `brave.ai_chat.last_accepted_disclaimer` | A date in the past |
| Embeddings | `brave.history_embeddings_enabled` | `true` |

These are the switches of the browser:

| Switch | Why |
|---|---|
| `--no-first-run --no-default-browser-check` | No first run page |
| `--use-mock-keychain --password-store=basic` | No keychain dialog. `OSCryptAsync` needs a key to encrypt the chats |
| `--component-updater=url-source=http://127.0.0.1:9/` | The update check fails, so the embedder component stays at the copied version. Do not use `--disable-component-update`: with it, Brave does not register the component, and the embedder never gets its model |
| `--enable-features=AIChatLearnedMemory:...,HistoryEmbeddings` | The feature flag and its params: `certain_threshold`, `certain_margin`, `run_time_limit`, `decision_model_name`, `decision_model_url`, `local_llm_model_name`. `HistoryEmbeddings` makes Brave register the embedder component |
| `--enable-logging=stderr --vmodule=...` | Logs of `user_memory_manager` and `learned_memory_eval` |
| `--learned-memory-eval-chats=<json>` | The chat set. This switch and the next one turn on eval mode |
| `--learned-memory-eval-output=<json>` | The place of `report.json` |
| `about:blank` | One empty tab |

Eval mode (C++, `learned_memory_eval.cc`): when the database comes, the manager
imports the chats, runs Dreaming one time with `record_trace`, writes the
report, and does not start the daily timer. The factory makes `AIChatService`
at the start of the profile when the feature flag is on.

## 5. Chat sets

A chat set is a JSON file in `chat_sets/`.

```json
{
  "name": "life_events",
  "description": "What the set tests",
  "chats": [
    {"title": "Moving plans", "days_ago": 60,
     "turns": [{"user": "We are moving to Berlin next month.",
                "assistant": "Congratulations!"}]}
  ],
  "expect": {
    "memories":  [{"label": "lives in Berlin", "all": ["berlin"], "any": ["live", "moved"]}],
    "absent":    [{"label": "old home is replaced", "all": ["lives in san francisco"]}],
    "types":     [{"label": "trip is short-term, if stored", "all": ["tokyo"], "type": "short_term"}],
    "at_most_one": [{"label": "stored once", "all": ["short"]}],
    "forbidden": ["diabetes"],
    "max_memories": 12
  }
}
```

- `days_ago` is the age of the first turn. Turns of one chat are 2 minutes
  apart, and the assistant answers 2 minutes after the user. Dreaming reads
  only the text that the user typed.
- `assistant` is optional. It does not go to any model.
- Ids: chat number `c`, turn number `t`. The conversation is `eval-<c>`. The
  entries are `eval-<c>-<t>-user` and `eval-<c>-<t>-assistant`. The report uses
  them to link a memory to its source.

### The expectations

A match is a case-insensitive search in the memory text. A memory matches an
item when it has all words of `all`, and at least one word of `any`.

| Key | Passes when |
|---|---|
| `memories` | At least one memory matches each item. With `type`, the match must also have this type |
| `absent` | No memory matches the item (the text or the old text) |
| `types` | Each memory that matches has the type. No match is also a pass |
| `at_most_one` | At most one memory matches each item |
| `forbidden` | No memory text or old text has the word. This is the privacy check |
| `max_memories` | The count of memories is at most this number |

The expectations are a guide, not a proof. A wrong word in a pass is possible,
so read the memory table too.

## 6. The chat sets in this folder

| Set | Chats | What it tests |
|---|---|---|
| `life_events` | 8 | Replace (home), merge (two dogs), diet, a goal, a job, one task chat |
| `privacy_sensitive` | 9 | Safety decision, code denylist (email, phone, account), instruction injection, 2 safe facts |
| `task_only` | 9 | Gate and fact decisions. The result must be zero memories |
| `preferences_style` | 7 | The preference category, and the same preference in two chats (must be one memory) |
| `temporary_states` | 6 | Short-lived answer, short-term type, a trip must not replace a home |
| `multilingual` | 4 | German and Spanish facts, sentence split, rewrite in other languages |
| `relations` | 28 | 14 pairs of old and new chats in random order: facts that share a word but differ, replace, merge, and the same fact twice. Tests the relation step and the rule that a replace or merge needs a certain decision model |

## 7. The output

```
<out>/
  summary.md          one row for each set
  summary.json
  <set>/
    report.json       result, config, trace, memories (from the browser)
    report.md         the readable report
    brave.log         browser log
    profile/          deleted, unless --keep-profiles
```

`report.md` has these parts:

1. **Result.** Status, turns read and kept, memories added and updated, time.
2. **Expectations.** One row for each check, with pass or fail and the detail.
3. **Memories.** Text, category, type, previous text (undo), source chat.
4. **Latency.** Count, median, max and total for each kind of call.
5. **Turns.** For each user turn, in order, every step below.

### The trace steps

Each step in `report.json` has `step`, `t_ms` (time from the start of the run)
and `entry` (the user turn). `latency_ms` is the time of the model call.

| Step | Shows |
|---|---|
| `loaded` | Memories, tombstones and watermarks at the start |
| `turn` | Chat, date and the text of the user turn |
| `gate` | Probabilities, answer, keep or skip |
| `split` | Sentences, and which ones the denylist removes |
| `sentence_decisions` | For each sentence: fact, safety, category and temporary probabilities, and keep or the reason to drop (for example `drop: sensitive (0.58 >= 0.30)`) |
| `llm_call` | Purpose (`rewrite`, `relation`, `merge`), the request, the raw answer, the error |
| `rewrite_parsed` | Facts from the LLM, their sources, the flag `no_new_details` (only a flag, not a gate), `too_long` |
| `embed` | Purpose, passages, dimensions, latency, status |
| `rewrite_checked` | The facts that go to the relation step: the LLM facts, and the user's sentence for a sentence that no fact covers |
| `fact` | One fact: text, labels, links |
| `fact_dropped` | The reason: tombstone, relation not certain, no LLM answer |
| `neighbors` | The closest old memories with similarity, and the best similarity |
| `relations` | The decision model answer for each old memory |
| `llm_relation` | The LLM answer, when the decision model is not certain |
| `relation_applied` | The relation, and if the rules allow the text change |
| `merge_parsed` | The merged text, the flag `no_new_details`, `too_long`. A merge has mechanical guards only |
| `store` | `add` or `update`, the text, the labels, the old text |
| `watermark` | The date that Dreaming saved for the chat |
| `done` | Final status and total time |

## 8. Add a chat set

1. Copy a file in `chat_sets/`, and change the name, the chats and `expect`.
2. Run `./run_eval.py --sets <name>`.
3. Read `report.md`. Change the expectations only for a real mistake of the
   set, not to make a failed check pass.

## 9. Limits

- One run for each set. The LLM and the decision model are not exactly
  repeatable, so use the same set more than one time before you trust a
  difference.
- The chats are short and they are written by us. They show the behavior of
  each step, but they do not give accuracy numbers.
- The assistant text is not read by Dreaming, so it does not change a result.
- The harness checks only Dreaming. Chat time (Track B) has its own work.
- If the embedder model is not ready, the run waits for it until the time
  limit. A run that stops at the time limit has status `timed_out`, and
  `report.md` shows a diagnosis. Cause that we found: a missing
  `HistoryEmbeddings` feature, or the switch `--disable-component-update`.
  Log line of this problem (with `--vmodule=*scheduling_embedder*=5`):
  `SubmitWorkToEmbedder: embedder not ready`.
- The decision model file `clef-flash:9b` and `qwen3.5:9b` together need about
  17 GB of memory in Ollama.

## 10. Test the decision questions alone

`decision_eval.py` calls the decision model (Ollama `/v1/systemone`) directly.
One wording takes about a minute, and it needs no browser. Use it before you
change a question in `ollama_decision_client.cc`.

```sh
./decision_eval.py                                      # all variants, tuning cases
./decision_eval.py --cases decision_cases_holdout.json  # new cases, not used to tune
./decision_eval.py --variants A I --errors I            # list the wrong cases
```

- Cases: `decision_cases.json` (69 sentences, 28 gate turns) and
  `decision_cases_holdout.json` (36 sentences, 10 gate turns). Each sentence has
  labels for fact, safety, category and temporary. Write new cases in the
  hold-out file before you look at results.
- Variants: the wordings of each question. `A` is the first wording, `I` is the
  wording in the code. Add a variant in the script to try a new wording.
- Rules: `FINAL` is the keep rule of `DreamingRun`. A good result keeps most of
  the sentences that should be kept, with 0 unsafe keeps (a sensitive or
  instruction sentence that is kept).
- The order of the questions and of the options is part of the wording. The
  script keeps the order of the dictionary, and the C++ client builds its
  request in the same order. Change both when you change the order.
- Results are cached in `/tmp/decision_eval_cache.json`.
