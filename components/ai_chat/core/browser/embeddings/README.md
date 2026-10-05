# components/ai_chat/core/browser/embeddings/

An on-device index of embeddings of the user's stored Leo conversations and
their memories, and the semantic search over it that Leo's `conversation_search`
and `memory_semantic_search` tools use.

## Pieces

- **`AIChatEmbeddingsService`** — KeyedService that keeps the index in step with
  the stored conversations and memories, and answers searches. Built by
  `AIChatEmbeddingsServiceFactory` in `//brave/browser/ai_chat`.
- **`AIChatEmbeddingsDatabase`** — the SQLite index,
  `<profile>/AIChatEmbeddings`, on a background sequence.
- **`SplitIntoPassages()`** — splits an entry's text into passages for
  embedding.

The embeddings are computed by the EmbeddingGemma embedder that semantic history
search uses (see
[`//brave/browser/history_embeddings`](../../../../../browser/history_embeddings/README.md)):
indexing at `kPassive` priority, queries at `kUserInitiated`. As in semantic
history search, documents and queries are embedded as they are, so the model's
score threshold applies to them.

## What is indexed

- A conversation's title, and the text of the latest version of each of its
  entries: what the user wrote, and the assistant's response. Page content,
  uploaded files, tool output and search results aren't indexed.
- Each memory in `kBraveAIChatUserMemories`.

As in semantic history search, conversation text is split into passages of up to
100 words, passages of fewer than 5 words are left out, and a conversation is
ranked by its best passage. Memories are embedded whole.

Temporary conversations are never stored, so they are never indexed.

## Keeping the index in step

The index is derived data that can be rebuilt at any time.

- `AIChatService::Observer` reports each persisted change, which is applied to
  the index as it happens.
- When `AIChatService` storage becomes ready, the index is reconciled with the
  stored conversations: those updated since they were indexed are indexed again
  in full, and those no longer stored are removed. That picks up what changed
  while the service wasn't running.
- Embeddings of another model version, or from passages prepared differently
  (`kPassageVersion`), are deleted when the database opens, and the
  reconciliation then indexes everything afresh.
- Memories are synced with their pref whenever it changes.

## Privacy

- The text and the embedding of each passage are both encrypted with OSCrypt,
  since an embedding can give away the text it was computed from.
- Deleting conversations, or turning storage off, removes them from the index.
  Turning the Semantic history search setting off deletes the index.
- `conversation_search` sends the passages it finds to the model, so it asks for
  permission once per conversation. `memory_semantic_search` doesn't, since
  memories go with every request it is offered for.

## Gating

The Semantic history search setting turns this index on along with the index of
browsing history. The service starts with the profile when
`BraveHistoryEmbeddingsStatus` reports the setting on, so stored conversations
are indexed before Leo is first used, and turning the setting back on takes
effect on relaunch.

## Testing

`FakeEmbedder` embeds a passage by the keywords it mentions, so that tests can
check ranking. `AIChatEmbeddingsService::IsIndexingIdleForTesting()` and
`FlushForTesting()` let tests wait for indexing to settle.
