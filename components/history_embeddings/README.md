# History Embeddings in Brave

Semantic history search. User types a natural-language query; Brave returns
previously visited pages whose content is semantically close to the query, even
when no keyword matches.

Built on top of Chromium's `history_embeddings` + `passage_embeddings`
components, but Brave swaps the embedder for a local Rust/Candle WASM model
(EmbeddingGemma) and disables the upstream visibility filter that depends on
remote classification.

This doc is the big-picture overview of the whole feature — upstream + Brave.
For the Brave embedder's internals (process model, `BindPassageEmbedder` vs
`LoadModels`, factory init handshake, file lifecycle), see
[`//brave/browser/history_embeddings/README.md`](../../browser/history_embeddings/README.md).

## Tunable parameters (defaults)

| Knob                              | Value    | Defined in                                                                                                    |
| --------------------------------- | -------- | ------------------------------------------------------------------------------------------------------------- |
| Max words per passage             | 100      | `//components/passage_embeddings/core/passage_embeddings_features.cc` — `kMaxWordsPerAggregatePassage`        |
| Max passages per page             | 10       | `//components/passage_embeddings/core/passage_embeddings_features.cc` — `kMaxPassagesPerPage`                 |
| Min words per passage             | 5        | `//components/passage_embeddings/core/passage_embeddings_features.cc` — `kMinWordsPerPassage`                 |
| Min query word count              | 2        | `//components/history_embeddings/core/history_embeddings_features.cc` — `kSearchQueryMinimumWordCount`        |
| Min passage word count for search | 5        | `//components/history_embeddings/core/history_embeddings_features.cc` — `kSearchPassageMinimumWordCount`      |
| Cosine score threshold (Brave)    | **0.45** | `//brave/browser/history_embeddings/brave_passage_embeddings_service_controller.cc` — `GetEmbedderMetadata()` |
| Embedding output size             | 768      | `//brave/browser/history_embeddings/brave_passage_embeddings_service_controller.cc` — `GetEmbedderMetadata()` |
| Per-passage score aggregation     | **max**  | `//components/history_embeddings/core/vector_database.cc` — `UrlData::BestScoreWith()`                        |
| Word-match boost                  | additive | `//components/history_embeddings/core/vector_database.cc` — capped at 5 matched terms, factor 0.2             |

`kSearchScoreThreshold` (also in
`//components/history_embeddings/core/history_embeddings_features.cc`) defaults
to `-1`, meaning the runtime falls back to the embedder metadata threshold —
i.e., Brave's 0.45.

### Finch overrides (brave-variations)

Study `HistoryEmbeddingsParamsStudy` in
[`brave-variations`](https://github.com/brave/brave-variations) tunes the
word-match path for users who have force-enabled `HistoryEmbeddings` (flag/CLI).
The study is currently set up with `probability_weight: 0` on the `Tuned` arm —
wired in but not rolled out.

| Param                        | Upstream default | Brave Tuned | What it gates                                                                |
| ---------------------------- | ---------------- | ----------- | ---------------------------------------------------------------------------- |
| `WordMatchMinEmbeddingScore` | 0.7              | **0.4**     | Min cosine score below which the word-match path is allowed to surface a hit |
| `WordMatchRequiredTermRatio` | 1.0              | **0.8**     | Fraction of query terms that must appear in the passage for word-match       |

Both loosen the word-match path so it can rescue results that the pure cosine
score would have dropped: a passage with ~0.4 cosine can still surface if 80% of
the query terms appear in it.

Brave-variations PR: <https://github.com/brave/brave-variations/pull/1687>.

---

## Flow 1: Indexing a visited page

```
nav finishes
   │
   ▼
[renderer]  AIPageContentAgent walks the DOM, returns a structured
            AIPageContent tree (text nodes, image captions, table
            headers, anchors, ...) over mojo
            └─ //third_party/blink/renderer/modules/content_extraction/
                  ai_page_content_agent.{h,cc}
            └─ mojom: //third_party/blink/public/mojom/content_extraction/
                  ai_page_content.mojom
   │
   ▼
[browser]   PageContentExtractionService receives it, converts to
            AnnotatedPageContent protobuf
            └─ //components/page_content_annotations/content/
                  page_content_extraction_service.{h,cc}
            └─ proto: //components/optimization_guide/proto/
                  features/common_quality_data.proto
   │
   ▼
[browser]   PageEmbeddingsService chunks the proto into passages
            (algorithm below)
            └─ //components/page_content_annotations/content/
                  page_embeddings_service.{h,cc}
            └─ chunker: //components/page_content_annotations/content/
                  embeddings_candidate_generator.cc
                  (CreatePassagesFromAnnotatedPageContent)
   │
   ▼
[browser]   SchedulingEmbedder queues passages by priority
            (UserInitiated > Urgent > Passive > Latent)
            └─ //components/passage_embeddings/core/
                  scheduling_embedder.{h,cc}
   │
   ▼
[Brave]     BravePassageEmbeddingsServiceController::GetEmbeddings()
            • verifies the EmbeddingGemma component is installed
            • reads 5 model files (weights, weights_dense1,
              weights_dense2, tokenizer, config) off disk
            • hands them to BravePassageEmbeddingsService
            └─ //brave/browser/history_embeddings/
                  brave_passage_embeddings_service_controller.{h,cc}
   │
   ▼
[Brave]     BraveBatchPassageEmbedder spins up a background WebContents
            on a guest OTR profile pointing at chrome-untrusted://local-ai.
            That page loads the WASM bundle, instantiates Gemma3Embedder,
            and registers a PassageEmbedderFactory back over mojo.
            └─ //brave/browser/history_embeddings/
                  brave_batch_passage_embedder.{h,cc}
                  brave_passage_embeddings_service.{h,cc}
   │
   ▼
[WASM]      Gemma3Embedder.ComputeEmbedding(passage) → 768-dim vector
            (Brave runs this one passage at a time, not batched)
            └─ //brave/components/local_ai/resources/
                  candle_embedding_gemma/  (Rust + WASM bundle)
   │
   ▼
[browser]   HistoryEmbeddingsService::OnPassagesEmbeddingsComputed()
            validates dimensions / model version, then writes:
              passages   table (url_id, visit_id, visit_time,
                                passages_blob — encrypted via os_crypt_async)
              embeddings table (url_id, visit_id, visit_time,
                                embeddings_blob — serialized proto)
            via SqlDatabase on a worker sequence.
            └─ service: //components/history_embeddings/content/
                  history_embeddings_service.{h,cc}
            └─ storage: //components/history_embeddings/core/
                  sql_database.{h,cc}, vector_database.{h,cc}
```

### Chunking algorithm

Lives in
`//components/page_content_annotations/content/embeddings_candidate_generator.cc`,
`CreatePassagesFromAnnotatedPageContent()`.

1. Recursively flatten the `AnnotatedPageContent` tree into an ordered list of
   text snippets (text nodes, image captions, table names, ...).
2. Greedily pack snippets into the current passage, separated by a single space.
3. Always append if the current passage has fewer than 5 words (guarantees the
   min, even if it overshoots 100).
4. Otherwise append only if it stays under 100 words. Items that would overflow
   are **dropped, not split**.
5. When the passage hits the 100-word cap, start a new one. Stop after 10
   passages.

So a passage is roughly "the next ~100 words of visible content in document
order," not a semantic block. PDFs use the same idea but tokenize raw whitespace
and skip the 5-word minimum.

### Trigger timing

Two modes (Finch-controlled):

- **Continuous**: extraction runs eagerly when the page finishes loading.
- **On-demand**: extraction waits until the tab is hidden, to avoid competing
  with foreground work.

A short `kPassageExtractionDelay` (5s) is applied either way to let the page
settle.

---

## Flow 2: Semantic search query

```
user enters query
   │
   ▼
HistoryEmbeddingsService::Search(query, time_range, count, ...)
   └─ //components/history_embeddings/content/
         history_embeddings_service.{h,cc}
   │
   ├─ reject if query has < 2 words
   │
   ▼
embed the query through the same WASM pipeline as indexing
(priority = UserInitiated, jumps the queue)
   └─ //components/passage_embeddings/core/scheduling_embedder.{h,cc}
       → //brave/browser/history_embeddings/
           brave_passage_embeddings_service_controller.cc
   │
   ▼
Storage::Search() on the DB worker sequence:
   for each UrlData in range:
       UrlData::BestScoreWith(query_embedding):
           best = 0
           for each passage_embedding:
               score = cosine(query, passage_embedding)
               best  = max(best, score)
           return best + word_match_boost
   └─ //components/history_embeddings/core/vector_database.{h,cc}
   │
   ▼
filter: keep only urls where score >= 0.45
   └─ threshold resolved in
      //components/history_embeddings/content/history_embeddings_service.cc
      (GetScoreThreshold)
   │
   ▼
visibility filter
   • upstream: PageContentAnnotationsService scores each page
     (ads / paywalls / etc.) and drops low-visibility hits
     └─ //components/page_content_annotations/core/
           page_content_annotations_service.{h,cc}
   • Brave: BraveHistoryEmbeddingsService overrides this and
     synthesizes 1.0 visibility for every url (no remote
     classification, no filtering)
     └─ //brave/browser/history_embeddings/
           brave_history_embeddings_service.{h,cc}
        //brave/components/history_embeddings/content/
           brave_history_embeddings_helpers.h
   │
   ▼
optional answerer step (if enabled and not skipped):
   pick the top passages (min 3 rows, min 1000 total words of context),
   feed to Answerer → returns synthesized answer text + source url
   └─ //components/history_embeddings/core/answerer.{h,cc}
       //components/history_embeddings/core/ml_answerer.{h,cc}
   │
   ▼
SearchResult { scored_url_rows (sorted, capped at count), answerer_result }
   └─ //components/history_embeddings/core/history_embeddings_search.h
```

### Scoring detail

Final score per URL =

```
max(cosine(query, passage_i)) over all i  +  word_match_boost
```

The "max" is what matters most: one strong passage is enough to surface the
page. This is why the 10-passage cap is reasonable — extra middling passages
would not change the result.

#### Word-match boost

Implemented in `UrlData::BestScoreWith()`
(`//components/history_embeddings/core/vector_database.cc`). It's a text-overlap
bonus added on top of the cosine score, with three gates:

1. **Length gate** — if the query has more than `word_match_max_term_count`
   (default 10) terms, word-match is disabled entirely for this URL.
2. **Per-passage cosine gate** — a passage only gets its terms counted if its
   own cosine score is ≥ `WordMatchMinEmbeddingScore` (upstream default **0.7**,
   Brave's tuned study sets this to **0.4**). Passages below the gate contribute
   0 to the term tally.
3. **Required-term-ratio gate** — after summing term counts across **all
   eligible passages** of the URL, if the fraction of query terms that appeared
   at least once is below `WordMatchRequiredTermRatio` (upstream default
   **1.0**, Brave's study **0.8**), the boost is zeroed.

If all gates pass, the boost is built additively from per-term contributions:

```
per_term_boost  = word_match_score_boost_factor       # 0.2
                  * term_count                         # occurrences across
                                                       # eligible passages,
                                                       # capped per scan at
                                                       # word_match_limit (5)
                  / word_match_limit                   # 5

word_match_boost = sum(per_term_boost over query terms)
                   / max(1, num_query_terms + word_match_smoothing_factor)
                                                       # smoothing default 0,
                                                       # so this is just
                                                       # num_query_terms
```

Practical bounds: each term contributes at most `0.2 * 5 / 5 = 0.2`. After
normalization by query length, the total boost is at most `0.2` regardless of
query size. So word-match can lift a borderline 0.4-cosine passage to ~0.6 —
enough to clear Brave's 0.45 threshold and surface the result.

Note: term counts are accumulated **across all of the URL's eligible passages**,
not just the best one. So a URL where the query terms are sprinkled across
several passages still gets credit, even if no single passage is a strong cosine
match.

#### What Brave's tuning changes

| Knob                         | Upstream | Brave Tuned | Effect                                                                                                   |
| ---------------------------- | -------- | ----------- | -------------------------------------------------------------------------------------------------------- |
| `WordMatchMinEmbeddingScore` | 0.7      | 0.4         | More passages qualify for term scanning, so word-match can rescue moderately-relevant pages              |
| `WordMatchRequiredTermRatio` | 1.0      | 0.8         | Partial-term-match queries (e.g. 4 of 5 terms found) still receive the boost instead of being zeroed out |

The net effect is a more forgiving search: pages that are weakly related
semantically but contain most of the query words can still surface.

---

## How Brave differs from upstream

| Upstream                                                                  | Brave                                                                                                                                                |
| ------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| TFLite model delivered via OptimizationGuide (server fetch)               | Static Rust/Candle **EmbeddingGemma** model delivered via component updater, run as WASM                                                             |
| Embedder runs in a sandboxed utility process, talks mojo                  | Embedder runs in a **dedicated renderer process** (background WebContents on a guest OTR profile, shown as "Tool: On-Device AI" in the task manager) |
| PageContentAnnotationsService filters low-visibility pages out of results | Visibility filter bypassed — `SynthesizePassingVisibilityResults()` returns 1.0 for everything                                                       |
| `kHistoryEmbeddings` / `kPassageEmbedder` enabled by Finch                | Both DISABLED_BY_DEFAULT; user opts in via `chrome://flags`. brave-variations study tunes word-match params for opt-in users (see above)             |
| `search_score_threshold` from model metadata, fallback 0.9                | Hardcoded **0.45** in `GetEmbedderMetadata()`                                                                                                        |
| Answerer uses OptimizationGuide-delivered model                           | Same plumbing, but gated behind the flag                                                                                                             |

### Brave-side code

- [`//brave/browser/history_embeddings/`](../../browser/history_embeddings/README.md)
  — the local embedder: controller, in-process service, batch embedder driving
  the WASM renderer. See its README for the full breakdown.
- `//brave/browser/history_embeddings/`
  - `brave_history_embeddings_service.{h,cc}` — subclasses upstream's
    `HistoryEmbeddingsService` to short-circuit the visibility step.
- `//brave/components/history_embeddings/content/`
  - `brave_history_embeddings_helpers.h` —
    `SynthesizePassingVisibilityResults()`.
- `//brave/components/local_ai/resources/candle_embedding_gemma/` — Rust crate
  that builds to WASM. Uses `candle-core`, `candle-nn`, `tokenizers`,
  `safetensors`.
- `//brave/chromium_src/`
  - Shims that inject `virtual` and `friend` declarations into upstream headers
    so the Brave singleton can subclass cleanly without patches.
  - Default-disables both feature flags.
  - Swaps the keyed service factory to instantiate
    `BraveHistoryEmbeddingsService`.

### Process model

- Browser-side controller and the mojo `PassageEmbedder` impl live in the
  browser process — no extra utility process is launched.
- Actual model execution runs in a **separate renderer process** hosting a
  background WebContents on a guest OTR profile, pointed at
  `chrome-untrusted://local-ai`. That process loads the WASM bundle and runs the
  Candle/Gemma inference; it shows up in Chrome's task manager as "Tool:
  On-Device AI".
- Communication between the browser and that renderer is mojo, same as any other
  renderer.
- Sandbox: renderer sandbox + `chrome-untrusted` origin (no extension APIs, no
  privileged bindings).

---

## Things to remember

- Passages are **chunks of words in DOM order**, not paragraphs or sentences.
  Overflowing items are dropped, not split — so very long single text nodes can
  be skipped entirely.
- One page can contribute at most 10 passages and ~1000 words to the index.
- The "score" you see is `max passage cosine + small word-match bonus`. If one
  passage hits 0.7 cosine, the page is in regardless of the other nine.
- 0.45 is the entire gate. Tuning it is the most direct lever for
  precision/recall.
- Visibility is not a factor in Brave — anything indexed can be returned.
- The query path uses the **same WASM embedder** as indexing, just at
  `UserInitiated` priority. First search after launch can be slow because the
  WASM page has to load and the model has to deserialize.
