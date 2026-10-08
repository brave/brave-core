# Learned memory eval harness

This harness runs Dreaming (learned memory) in a real Brave build on chat sets
that we write. It shows what Dreaming stores, and it logs the input and the
output of each step: the decision model, the local LLM and the embedder.

Code: `brave/tools/learned_memory_eval/`. Design: `impl.md` in the notes folder.

The tools at a glance:

```
 datasets/ (DATASETS.md) ─── validate_datasets.py                        section 15
   e2e/<split>/*.json ─────► run_eval.py ─► Brave ─► report.json/.md    sections 1-9
                               │ --proxy        gold scoring, misses     section 7
                               ▼
                             eval_proxy.py ─► proxy.jsonl ─► parity_check.py   section 16
                             report.json traces ─► sweep.py (thresholds)   section 20
   steps/<step>/*.jsonl ───► step_eval.py  (one step alone, L1)          section 19
   steps/{sentence,gate} ──► decision_eval.py    (decision model alone)  section 10
   steps/relevance ────────► relevance_eval.py   (relevance alone)       section 12
   steps/{sentence,rewrite} ► noise_check.py     (repeatability)         section 17
 two runs ──► compare.py (deltas, paired flips)                         section 21
 lib/requests.py   the requests of the C++ code, all wordings            section 18
 lib/steps.py      the step evals  ·  lib/gold.py  e2e gold scoring
 lib/clients.py    Ollama clients, disk cache  ·  lib/manifest.py  manifest.json
 results root      ~/brave/learned_memory_eval_runs/<time>-<tag>/
```

## 1. How it works

```
datasets/e2e/dev/<name>.json ──► run_eval.py
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
              <run>/<set>/r<k>/report.json  trace + memories
                              report.md    readable report
                              brave.log    browser log
              <run>/summary.md         one table for all sets
              <run>/manifest.json      versions, params, datasets
              <run>/proxy.jsonl        every model request (--proxy only)
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
| Decision model | `clef-flash:9b` installed in Ollama (the tools load it, see section 14) |
| Local LLM | `qwen3.5:9b` installed in Ollama (the tools load it, see section 14) |
| Embedder files | The component `BraveLocalAIModels` in the dev profile. The harness copies it to each new profile |
| Python | 3.9 or newer. No packages |

## 3. Run it

```sh
cd brave/tools/learned_memory_eval
./run_eval.py --list                     # show the chat sets
./run_eval.py                            # run all dev sets
./run_eval.py --sets life_events task_only
./run_eval.py --threshold 0.8            # certain_threshold of the decisions
./run_eval.py --gate-threshold 0.3 --max-sensitive 0.25   # any feature param
./run_eval.py --proxy --tag baseline     # record every model request (section 16)
./run_eval.py --chat-set datasets/parity/parity.json --proxy --tag parity
./run_eval.py --proxy --repeats 3 --tag baseline-e2e   # 3 runs of each set, new profiles
./run_eval.py --dry-run --sets task_only # make the profile, print the command
./run_eval.py --report-only <run>        # new report.md
./run_eval.py --report-only <run> --gold-from datasets/e2e/dev   # score an old run with new gold
```

| Option | Default | Meaning |
|---|---|---|
| `--sets` | all | Names of chat sets in `datasets/e2e/<split>/` |
| `--split` | `dev` | `dev` or `holdout`. Run the holdout only for a final candidate |
| `--chat-set` | none | Chat set files to run instead of `--sets`, for example the parity set |
| `--brave` | `out/Component_arm64/Brave Browser Development.app/...` | Browser binary |
| `--results` | `$LM_EVAL_RUNS`, else `~/brave/learned_memory_eval_runs` | Results root |
| `--tag` | `e2e` | Short name of the run. The folder is `<results root>/<YYYYMMDD-HHMMSS>-<tag>` |
| `--out` | none | An exact output folder instead of the results root |
| `--proxy`, `--proxy-port` | off, `8898` | Send the model requests through `eval_proxy.py`, and record them in `<run>/proxy.jsonl` |
| `--repeats` | `1` | Run each set this many times, each in a new profile. Use 3 for a result (the LLM is not repeatable) |
| `--gold-from` | none | With `--report-only`: score with the chat sets of this folder, not with the copy in the run |
| `--source-profile` | dev profile folder | Where the embedder component is copied from |
| `--ollama` | `http://localhost:11434` | Ollama URL |
| `--decision` | `clef-flash:9b` | Decision model name |
| `--llm` | `qwen3.5:9b` | Local LLM name (the BYOM model) |
| `--threshold`, `--margin` | `0.8`, `0.2` | Certainty rule of the decision model. It is only for the labels (category, type) and the relation. The gate and the keep rules have their own params |
| `--gate-threshold`, `--fact-threshold` | `0.2`, `0.6` | The gate and fact limits of the keep rule |
| `--max-sensitive`, `--max-instruction`, `--max-short-lived` | `0.3`, `0.3`, `0.5` | A sentence is dropped when this safety answer reaches its limit |
| `--max-neighbors`, `--min-neighbor-similarity` | `5`, `0.5` | The old memories that the relation step compares with a new fact |
| `--max-relation-requests`, `--max-rewrite-requests` | `20`, `50` | LLM limits for one run |
| `--time-limit` | `600` | Run time limit in the browser, in seconds |
| `--timeout` | `1200` | The harness stops waiting after this time, in seconds |
| `--keep-alive` | `60m` | How long Ollama keeps the loaded models (section 14) |
| `--no-warmup` | off | Do not load the Ollama models before the run. The run still checks that they are installed |
| `--keep-profiles` | off | Do not delete the profile after the run |

Each set takes about 2 to 15 minutes. The embedder and the Ollama models load
at the start, and the LLM runs for each turn that passes the gate.

## 4. The new profile

The harness makes a new profile for each set, so a run never depends on an
earlier run. These are the steps and the files it writes.

| What | Where | Value |
|---|---|---|
| Profile folder | `--user-data-dir` | `<out>/<set>/r<k>/profile` |
| Embedder component | `<profile>/BraveLocalAIModels` | Copy of the dev profile files (APFS clone) |
| Custom model (BYOM) | `Default/Preferences`, `brave.ai_chat.custom_models` | Key `custom:e0a1b2c3`, name = `--llm`, endpoint `<ollama>/v1/chat/completions` (with `--proxy`: the proxy URL) |
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
| `--enable-features=AIChatLearnedMemory:...,HistoryEmbeddings` | The feature flag and all its Dreaming params (section 3), and `run_time_limit`, `decision_model_name`, `decision_model_url`, `local_llm_model_name`. `HistoryEmbeddings` makes Brave register the embedder component |
| `--enable-logging=stderr --vmodule=...` | Logs of `user_memory_manager` and `learned_memory_eval` |
| `--learned-memory-eval-chats=<json>` | The chat set. This switch and the next one turn on eval mode |
| `--learned-memory-eval-output=<json>` | The place of `report.json` |
| `about:blank` | One empty tab |

Eval mode (C++, `learned_memory_eval.cc`): when the database comes, the manager
imports the chats, runs Dreaming one time with `record_trace`, writes the
report, and does not start the daily timer. The factory makes `AIChatService`
at the start of the profile when the feature flag is on. The `config` of
`report.json` has every `DreamingConfig` value of the run. `run_eval.py` warns
when a param is missing there: the build is then older than the harness, and
the param had no effect.

## 5. Chat sets

A chat set is a JSON file in `datasets/e2e/dev/` or `datasets/e2e/holdout/`.
The sets there also have a `gold` key with the wanted facts, the leaks, the
replaces and merges, and the facts of each turn. `datasets/DATASETS.md`
describes it. `report.py` scores `expect` as before, and `gold` when it is there
(section 7).

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

## 6. The chat sets in `datasets/e2e/dev/`

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

The output goes to the results root, not to `/tmp` (macOS deletes `/tmp`).

```
<results root>/<YYYYMMDD-HHMMSS>-<tag>/
  manifest.json       git state, build, Ollama and model digests, params, datasets
  proxy.jsonl         every model request and answer (--proxy only)
  summary.md          one row for each set: mean [min, max] over the repeats
  summary.json        one entry for each set and repeat
  <set>/r<k>/         one folder for each repeat (r1, r2, ...)
    report.json       result, config, trace, memories (from the browser)
    report.md         the readable report
    brave.log         browser log
    chats.json        the chat set of the run
    profile/          deleted, unless --keep-profiles
```

`report.md` has these parts:

1. **Result.** Status, turns read and kept, memories added and updated, time.
2. **Expectations.** One row for each check, with pass or fail and the detail.
3. **Gold** (when the set has `gold`). The metrics of plan section 8.2, the
   missed facts with the step that lost each one, and lists of the leaks, stale
   memories, wrong and missed ops, duplicates, label errors and extras.
4. **Memories.** Text, category, type, previous text (undo), source chat.
5. **Latency.** Count, median, max and total for each kind of call.
6. **Turns.** For each user turn, in order, every step below.

### Gold metrics (`lib/gold.py`)

| Metric | Meaning |
|---|---|
| Recall | Required gold facts that a memory matches, divided by required gold facts |
| Precision | Memories that match a gold fact (required or optional), divided by all memories |
| Leaks | Memories whose text or old text matches a `forbidden` item. Must be 0 |
| Stale kept | `stale` items that a memory text still matches |
| Wrong ops | A replace or merge in the trace that no gold op allows. A change inside one gold fact (a plan that came true) is not wrong |
| Missed ops | A gold op, not `optional`, that did not happen |
| Duplicates | Gold facts that more than one memory matches |
| Label accuracy | Type and category of the memories that match a gold fact |
| Extras | Memories with no gold match, with their source turn and sentence |

The step that lost a missed fact comes from the trace of its source turns:
`not_read`, `gate`, `denylist`, `sentence:<reason>` (for example
`sentence:sensitive`), `rewrite:changed` (the LLM fact lost the words),
`rewrite:dropped`, `relation:not_certain`, `relation:no_llm_answer`,
`relation:wrong_same` (stored as "the same" as another memory), `merge`, or
`replaced` (stored, then a later replace removed it). The summary counts the
misses by step for all runs.

### The trace steps

Each step in `report.json` has `step`, `t_ms` (time from the start of the run)
and `entry` (the user turn). `latency_ms` is the time of the model call.

| Step | Shows |
|---|---|
| `index_current` | The index of the learned memories is current, and the run starts. `t_ms` is the wait |
| `loaded` | Memories and watermarks at the start |
| `turn` | Chat, date and the text of the user turn |
| `gate` | Probabilities, answer, keep or skip |
| `split` | Sentences, and which ones the denylist removes |
| `sentence_decisions` | For each sentence: fact, safety, category and temporary probabilities, and keep or the reason to drop (for example `drop: sensitive (0.58 >= 0.30)`) |
| `llm_call` | Purpose (`rewrite`, `relation`, `merge`), the request, the raw answer, the error |
| `rewrite_parsed` | Facts from the LLM, their sources, the flag `no_new_details` (only a flag, not a gate), `too_long` |
| `embed` | Purpose, passages, dimensions, latency, status |
| `rewrite_checked` | The facts that go to the relation step: the LLM facts, and the user's sentence for a sentence that no fact covers |
| `fact` | One fact: text, labels, links |
| `fact_dropped` | The reason: relation not certain, no LLM answer |
| `neighbors` | The closest old memories with similarity, and the best similarity. They come from the index search and from the memories that this run wrote. `latency_ms` is the search time |
| `relations` | The decision model answer for each old memory |
| `llm_relation` | The LLM answer, when the decision model is not certain |
| `relation_applied` | The relation, and if the rules allow the text change |
| `merge_parsed` | The merged text, the flag `no_new_details`, `too_long`. A merge has mechanical guards only |
| `store` | `add` or `update`, the text, the labels, the old text |
| `watermark` | The date that Dreaming saved for the chat |
| `done` | Final status and total time |

## 8. Add a chat set

1. Copy a file in `datasets/e2e/dev/`, and change the name, the chats, `expect`
   and `gold`. Every user turn must be in `gold.turns`.
2. Run `./validate_datasets.py`.
3. Run `./run_eval.py --sets <name>`.
4. Read `report.md`. Change the expectations only for a real mistake of the
   set, not to make a failed check pass. Add a line to the changelog of
   `datasets/DATASETS.md`.

## 9. Limits

- The decision model is repeatable, but the LLM is not (section 17). Use
  `--repeats 3` before you trust a difference.
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
./decision_eval.py                                      # all variants, dev cases
./decision_eval.py --split holdout                      # new cases, not used to tune
./decision_eval.py --variants A I --errors I            # list the wrong cases
```

- Cases: `datasets/steps/sentence/<split>.jsonl` and
  `datasets/steps/gate/<split>.jsonl`. Dev: 69 sentences, 56 gate turns.
  Holdout: 36 sentences, 46 gate turns. Each sentence has labels for fact,
  safety, category and temporary. Write new cases in the holdout before you
  look at results. `--cases FILE` still reads a file in the old format.
- Variants: the wordings of each question, in `lib/requests.py`. `A` is the
  first wording, `I` is the wording in the code. Add a wording there to try it.
- The script sends its own JSON text, not the exact bytes of the browser. The
  numbers are the same. `parity_check.py` checks the exact bytes (section 16).
- Rules: `FINAL` is the keep rule of `DreamingRun`. A good result keeps most of
  the sentences that should be kept, with 0 unsafe keeps (a sensitive or
  instruction sentence that is kept).
- The order of the questions and of the options is part of the wording. The
  script keeps the order of the dictionary, and the C++ client builds its
  request in the same order. Change both when you change the order.
- Results are cached in `/tmp/decision_eval_cache.json`.

## 11. A demo profile for the settings page

`make_demo_profile.py` makes a profile with a mock chat history, and starts a
browser on it. Use it to see the learned memory UI (Leo > Customization >
Memories) and to select "Dream now".

```sh
./make_demo_profile.py --force          # make the profile and start the browser
./make_demo_profile.py --no-launch      # make it only
/tmp/learned_memory_demo/launch.sh      # start it again later
```

```
demo/demo_history.json ──► eval mode imports the chats ──► Dreaming runs 1 time
                                                              │
                          stop the browser ◄──────────────────┘
                               │ remove learned memories and watermarks
                               │ set the last Dreaming time to now (no timer run)
                               ▼
   /tmp/learned_memory_demo/profile   23 chats, 0 learned memories
```

- The history is one mock user over about four months: a move from San
  Francisco to Berlin, a second dog, a diet, a marathon, a promotion, style
  wishes, short-lived states, sensitive data (health, email, phone, account
  number) and task questions. Edit `demo/demo_history.json` to change it.
- `--keep-memories` keeps what the first run learned. `--out` changes the
  folder. `--force` replaces an old demo folder (only a folder that the script
  made).
- `launch.sh` loads the Ollama models first, so the first "Dream now" is not
  slow. It writes the browser log to `browser.log` in the demo folder.
- The daily timer is one day away, so it does not run before you select "Dream
  now". "Dream now" has a time limit of at least 5 minutes. A second click on
  the same chats reads 0 new messages. To start over, run the script again with
  `--force`.

## 12. Test the relevance question alone

`relevance_eval.py` calls the decision model (Ollama `/v1/systemone`) with the
chat time relevance question, the same as `OllamaDecisionClient::AskRelevance()`.
It needs no browser. Use it before you change the question or the threshold
(`relevance_threshold`).

```sh
./relevance_eval.py                    # wordings A, B, C
./relevance_eval.py A --errors A       # list the memories that were not found
./relevance_eval.py A --thresholds 0.2 0.3 0.4
```

- `datasets/steps/relevance/memories.jsonl` has 14 memories (what Dreaming
  learned from `demo/demo_history.json`), and `dev.jsonl` has 9 messages. Each
  message names, by memory id, the memories that must be relevant and the ones
  that must not be. The other memories (style wishes) are neutral.
- The wordings are in `lib/requests.py`.
- The report shows, for each threshold, how many required memories were found,
  how many forbidden memories were found, and the mean number of memories kept.
  The latency of one request is about 0.1 s plus 0.08 s for each memory.
- Wording `A` is the wording in the C++ client. Change both when you change it.

## 13. See the request that Leo sends

`log_proxy.py` is a small proxy for the chat model endpoint. It writes each
request to a file and forwards it to Ollama. Use it to see the memory block with
the learned memories (Track B).

```sh
./log_proxy.py /tmp/requests.jsonl        # listens on 127.0.0.1:8899
# Set the endpoint of the custom model to http://127.0.0.1:8899/v1/chat/completions,
# send a message in Leo, then read the last line of /tmp/requests.jsonl.
```

## 14. Load the Ollama models

Ollama unloads a model after 5 minutes without a request. Then the next
Dreaming run or chat turn waits many seconds while the model loads, and a chat
turn can run into its time out. `ollama_setup.py` loads both models and keeps
them in memory. It does not install anything: the models must be in `ollama
list` already.

```sh
./ollama_setup.py                  # load both, keep them for 60 minutes
./ollama_setup.py --status         # what is loaded, and when Ollama unloads it
./ollama_setup.py --keep-alive 3h
./ollama_setup.py --unload
```

- `run_eval.py`, `make_demo_profile.py`, `decision_eval.py`,
  `relevance_eval.py` and `noise_check.py` call it before they use a model
  (`decision_eval.py` and `relevance_eval.py` load only the decision model).
  `--no-warmup` skips it.
- `launch.sh` of the demo profile calls it before it starts the browser.
- The decision model loads with a System One request, because it does not
  support `/api/generate`. The LLM loads with an empty `/api/generate` request.
- Ollama keeps the `keep_alive` of the first request for later requests that
  have none. So the browser keeps the models loaded for the same time, and the
  product code does not need to send `keep_alive`.

## 15. Datasets

All labeled cases are in `datasets/`. `datasets/DATASETS.md` is the data card:
the formats, the label rules, the coverage tags and the changelog.

```sh
./validate_datasets.py              # errors only
./validate_datasets.py --coverage   # also the coverage tags of each split
./validate_datasets.py --strict     # a missing coverage tag is an error
```

- The validator rejects bad JSON, a missing field, a bad label, a duplicate
  id, a split that does not agree with the file name, a text in both splits,
  an unknown memory id and bad e2e gold (for example a user turn that is not
  in `gold.turns`).
- Run it after every dataset change, and add a changelog line.
- Never tune on a holdout. Read only its totals.

## 16. Record the requests and check parity

`lib/requests.py` builds the requests of the C++ code in Python, for the step
evals. `parity_check.py` proves that they are the same as the requests of the
browser.

```
 run_eval.py --proxy ──► Brave ──► eval_proxy.py (127.0.0.1:8898) ──► Ollama
                                        │ one line per call
                                        ▼
                                  <run>/proxy.jsonl ──► parity_check.py
                                                         take the inputs from each request,
                                                         build it again with "current",
                                                         compare
```

```sh
./run_eval.py --chat-set datasets/parity/parity.json --proxy --tag parity
./parity_check.py <run>/proxy.jsonl --require gate sentence relation rewrite
./eval_proxy.py /tmp/calls.jsonl     # the proxy alone, for a manual session
```

- Decision model requests (gate, sentence, relation, relevance) must have the
  same bytes. LLM requests (rewrite, relation-llm, merge) must have the same
  message list. The report also says if the whole body is the same.
- `datasets/parity/parity.json` reaches every request kind of Dreaming in
  about 30 s. Relevance runs only at chat time, so no Dreaming set reaches it.
- When you change a question or a prompt in C++, change the "current" variant
  in `lib/requests.py` in the same change. Then rebuild, record, and run
  `parity_check.py`.
- A proxy line has `set`, `path`, `kind`, `body` (the exact request text),
  `status`, `answer` and `latency_ms`.

## 17. Measure the noise

`noise_check.py` sends the same requests many times, with no cache, and
reports how often the answer is the same.

```sh
./noise_check.py                     # 20 sentence and 20 rewrite requests, 5 times each
./noise_check.py --skip-llm --parallel 4
```

Result on 2026-10-08 (Ollama 0.40.1): `clef-flash:9b` gave the same
probabilities every time, also with 4 parallel requests. `qwen3.5:9b` gave a
different rewrite text for 20 of 20 requests, and different facts for 19 of
20. So decision model steps need 1 repeat, and LLM steps and end-to-end runs
need 3.

## 18. Shared code (`lib/`)

| Module | Has |
|---|---|
| `lib/requests.py` | The request builders of the C++ code (`current`) and all other wordings. A JSON writer with the escapes of `base::JSONWriter`. Ports of `GetCertainAnswer`, `ParseRewriteAnswer`, `ParseRelationAnswer`, `ParseMergeAnswer` and `IsFaithfulRewrite` |
| `lib/clients.py` | Clients for `/v1/systemone` and `/v1/chat/completions`: retries, latency, a disk cache. A 4xx answer is a result (Laya answers a long request with HTTP 400) |
| `lib/manifest.py` | The run folder and `manifest.json` (git state, build, Ollama version, model digests, params, variants, datasets, machine) |
| `lib/steps.py` | The step evals of `step_eval.py`: one class for each step, with its scoring |
| `lib/gold.py` | The e2e gold scoring and the step that lost each missed fact |
| `lib/scoring.py` | Wilson intervals, word matches, latency summaries, paired flips |

- The results root is `~/brave/learned_memory_eval_runs`. Set `$LM_EVAL_RUNS`
  or `--results` to change it.
- The cache is in `<results root>/cache/`. The key is the sha256 of the model
  digest, the endpoint, the exact body and the repeat index. A new model or a
  new wording gives a new key. A second run of 100 cases takes less than 0.1 s.
- The code uses only the Python standard library (3.9 or newer), and no
  personal paths. Each person can set the models and the paths with flags.

## 19. Test one step alone (`step_eval.py`)

`step_eval.py` runs one step of the pipeline on its labeled cases, with no
browser. It sends the exact request of the C++ code (lib/requests.py), and
scores the answers with the rules of the plan, section 8.1.

```sh
./step_eval.py gate                                   # current wording, dev cases
./step_eval.py sentence --variant current A --errors  # two wordings, paired flips
./step_eval.py relation --threshold 0.7               # another certain rule
./step_eval.py relevance --thresholds 0.2 0.3 0.4
./step_eval.py rewrite --model gemma3:4b              # another LLM, 3 repeats
./step_eval.py merge --serial --refresh               # new answers, real latency
./step_eval.py sentence --split holdout               # final candidates only
```

| Step | Model | Primary metric | Hard gate |
|---|---|---|---|
| `gate` | decision | Keep recall at `--gate-threshold` | Recall >= 0.98 on the holdout |
| `sentence` | decision | Keep recall of the `DreamingRun` keep rule | 0 unsafe keeps (sensitive or instruction kept) |
| `relation` | decision | Accuracy of the certain answers | Costly errors <= baseline |
| `relevance` | decision | Must recall (yes >= 0.3, best 5) | Forbidden rate <= baseline |
| `rewrite` | LLM | Key-fact rate (all `must` words in the facts) | JSON valid >= 0.99, 0 `must_not` |
| `relation-llm` | LLM | Accuracy | none (the code ignores its replace and merge) |
| `merge` | LLM | Both facts kept | 0 `must_not` |

- Options: `--split`, `--variant` (one or more; the first one is the base of
  the flips), `--model`, `--repeats` (default 1 for decision model steps, 3 for
  LLM steps), `--errors [VARIANT ...]`, `--tag`, `--parallel` (default 4),
  `--serial`, `--no-cache`, `--refresh`, `--cases FILE`, and the step params
  with the C++ defaults (`--gate-threshold`, `--fact-threshold`,
  `--max-sensitive`, `--max-instruction`, `--max-short-lived`, `--threshold`,
  `--margin`, `--relevance-threshold`, `--max-turn-memories`).
- The sentence report also has a keep-rule grid (`--grid-fact`,
  `--grid-sensitive`), from the same answers.
- Output: `<results root>/<time>-<step>-<split>-<tag>/` with `manifest.json`,
  `results.jsonl` (one line per case, variant and repeat), `metrics.json` and
  `report.md`. Proportions have a Wilson 95% interval.
- A cached answer has no latency. Use `--serial --refresh` to measure latency.
- `--errors` does not list the holdout cases (plan, data rule 2).
- The ports of `ParseRewriteAnswer()` and `IsFaithfulRewrite()` give the same
  results as C++ on the browser traces: `./parity_check.py --traces
  <run>/*/r*/report.json`.

## 20. Sweep the keep thresholds offline (`sweep.py`)

`sweep.py` reads the traces of an L2 run, and computes the keep decisions of
`DreamingRun` for a grid of thresholds. The decision model is repeatable, so
the result is exact for the keep decisions. No model runs.

```sh
./sweep.py <run> --gate 0.1 0.2 0.3 --fact 0.5 0.6 0.7 --sensitive 0.2 0.3
./sweep.py <run> --short-lived 0.4 0.5 0.6 --top 10
```

- It checks first that the thresholds of the run give the kept sentences of
  the trace.
- It scores each kept sentence against `gold.turns`: wanted (a sentence of a
  required fact), wrong (a sentence of a turn that must give no memory),
  forbidden (it matches a leak item, must be 0) and extra.
- A turn that the gate skipped has no sentence answers in the trace. For a
  gate threshold below the run's threshold, these turns are "unknown". To sweep
  the gate fully, run `run_eval.py --gate-threshold 0` one time.
- The sweep covers the keep decisions only. A new point needs an L2 run to
  show the effect on the rewrite and relation steps.

## 21. Compare runs (`compare.py`)

```sh
./compare.py <run A> <run B>                 # two runs of the same step, or two L2 runs
./compare.py --table --tag baseline-v1       # one row for each run with this tag
```

- Two L1 runs: the delta of each metric, the hard gates, the cases that B
  fixed and broke (paired by case id, a case is ok when at least half of its
  repeats are ok), and a two-sided sign test.
- Two L2 runs: the numbers of `summary.json` for each set.
- It warns when the manifests differ in more than one variable (models,
  params, variants, datasets, code, serial or parallel).
