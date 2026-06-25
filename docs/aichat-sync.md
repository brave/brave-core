# AI Chat Sync — Data Model

## TL;DR

An AI Chat conversation is broken into two kinds of sync records:

1. **Conversation metadata** — one per conversation. Title, model, token counts,
   timestamps. Always small.
2. **Conversation entry** — one per turn. The user/assistant message, its events
   (completion text, web sources, tool calls), the metadata for any pages it
   referenced (including the extracted page text), and any files the user
   attached (with their bytes when small enough to fit).

Both kinds flow through a single sync DataType (`AI_CHAT_CONVERSATION`,
EntitySpecifics field number `2000001`). The discriminator is a `oneof kind`
inside the specifics proto. The bridge tells which kind a record is by looking
at the storage key prefix: `c:<uuid>` for a conversation, `e:<uuid>` for an
entry.

Two related concerns shape the design:

- **DynamoDB enforces a 400 KB hard cap on every sync entity row.** A single
  conversation as one record would frequently exceed that. Splitting per turn
  gives each turn its own 400 KB budget.
- **Some fields are routinely big** (extracted page text, tool output, image
  bytes), so even a single turn can blow the cap. To keep all the data the user
  actually needs to _continue the conversation on another device_, long string
  fields are wrapped in `AIChatCompressibleString` (gzip-on-write when worth it)
  and a deterministic size-budget policy omits fields in priority order when an
  entry still doesn't fit. An omitted field carries a content hash so the
  receiver can restore the value from a byte-identical local copy instead of
  overwriting it with empty.

## Record relationships

```
Conversation (c:<conv-uuid>)
   ├── title, model_key, total_tokens, trimmed_tokens, timestamps
   │
   └── (implicit parent of, via conversation_uuid backlink)
       ├── Entry (e:<entry-1-uuid>)
       │     ├── entry_text, prompt, character_type, action_type, ...
       │     ├── events[ completion | search_queries | web_sources | inline_search | tool_use ]
       │     ├── associated_content[ uuid, url, title, content_type, last_contents ]
       │     └── uploaded_files[ filename, filesize, type, data, extracted_text ]
       ├── Entry (e:<entry-2-uuid>) ...
       └── ...
```

Entries reference their parent via `conversation_uuid` on the Entry message
itself. There is no foreign-key constraint at the storage layer — the
relationship is sync-application-level only. Associated content and uploaded
files are **per-entry**, not per-conversation, so they travel with the Entry
that referenced them.

### Why one DataType, two kinds

A single DataType keeps the registration footprint small: one `DataType` enum
value, one `UserSelectableType`, one `chromium_src` override in `data_type.cc`,
one server-side type ID. The two kinds of records still flow through the same
`MergeFullSyncData` / `ApplyIncrementalSyncChanges` pipeline; the bridge
inspects each record and routes it.

The user-facing toggle ("Sync Leo AI") is a single switch — there is no scenario
where a user would enable conversation metadata sync but disable entries — which
lines up with a single DataType.

### Simple model

If you imagine each conversation as a notebook:

- The **Conversation record** is the cover sheet — title of the notebook, who
  wrote it, when it was last touched.
- Each **Entry record** is one page in the notebook — what the user asked, what
  the assistant answered, what files were stapled to that page, what
  pages-from-the-web were cited.
- The whole notebook is rebuilt on another device by reading the cover sheet and
  each page in any order.

The reason for splitting is that the postal service shipping pages (DynamoDB)
rejects any envelope heavier than 400 KB. One page per envelope, gzipped where
it helps, and if a single page is _still_ too heavy, drop the least-important
attachments to that page until the envelope fits.

## Prior art in Chromium

### Polymorphic specifics (one DataType, multiple kinds)

This is the same shape as `SAVED_TAB_GROUP`:

- `components/sync/protocol/saved_tab_group_specifics.proto` —
  `SavedTabGroupSpecifics` uses
  `oneof entity { SavedTabGroup group; SavedTabGroupTab tab; }`. The comment on
  `SavedTabGroupTab` says verbatim: _"they are stored as separate entities due
  to size limitations of sync entities"_ — the same motivation.
- `components/saved_tab_groups/internal/saved_tab_group_sync_bridge.cc` —
  `SavedTabGroupSyncBridge` dispatches on `specifics.has_group()` /
  `specifics.has_tab()`. Parent (`group_guid`) is carried on the child (`Tab`),
  just as we carry `conversation_uuid` on `Entry`.

A second example is `ACCESSIBILITY_ANNOTATION`
(`accessibility_annotation_specifics.proto`), which uses a more elaborate
`oneof entity { Order, Shipment, DriversLicense, … }` inside a single DataType.
Polymorphic specifics under one DataType is a sanctioned upstream pattern.

The one deviation: our storage keys carry an explicit `c:` / `e:` prefix.
`SavedTabGroupSyncBridge` doesn't prefix — it relies on GUIDs being unique
across both kinds. We took the safer route. An explicit prefix makes the kind
self-evident from the storage key alone, which simplifies `GetDataForCommit` and
the DELETE dispatch in `ApplyIncrementalSyncChanges`.

### Compression in Chromium Sync

Chromium's sync engine already gzip-compresses the whole HTTP commit/poll batch
at the transport layer (`components/sync/engine/net/http_bridge.cc:243` sets
`Content-Encoding: gzip`, and `:256` calls `compression::GzipCompress` on the
serialized request bytes). That saves wire bandwidth but **does not help the 400
KB per-record limit**: the server decompresses the batch and stores each
`SyncEntity.specifics` as raw bytes in a DynamoDB row. The DynamoDB cap applies
to the uncompressed item.

There is no upstream precedent for compressing inside a DataType's specifics. AI
Chat is the first Brave-side DataType where field-level gzip is worth it,
implemented as the `AIChatCompressibleString` wrapper rather than as a
sync-engine-level feature so the cost stays local to fields that actually
benefit from it.

### Omitting oversized fields

Chromium has precedent for _marking_ a field so receivers know it may be
incomplete — see `components/sync/protocol/deletion_origin.proto`, whose
`*_possibly_truncated` fields tell the receiver "trust the value if present, but
treat its absence as 'I didn't have room', not 'it was empty'".

AI Chat goes one step further: instead of a bare sentinel, an omitted field
carries a **hash of the value that was dropped**. The receiver looks that hash
up against the values it already holds locally and restores the original only
when a byte-identical copy is found. This is _content-addressed_ preserve-local:
it never fabricates divergent text, and it works even for values nested in
arrays (event completions, tool output) whose items have no stable identity to
match on. See `AIChatCompressibleString.omitted_content_hash` and
`AIChatUploadedFile.omitted_data_hash`.

`components/sync/protocol/bookmark_specifics.proto` also documents a
length-based drop policy (favicon URL is dropped when it exceeds
`kMaxFaviconUrlSize`). The difference: bookmark truncation is _destructive_ for
the receiver (the field is just gone), whereas AI Chat's omission is
_restore-local_ on the receiver via the content hash.

## Compression and size strategy

### `AIChatCompressibleString`

Long string fields are wrapped in:

```protobuf
message AIChatCompressibleString {
  oneof value {
    string raw = 1;
    bytes gzipped = 2;             // RFC 1952 gzip of the UTF-8 bytes
    fixed32 omitted_content_hash = 3;  // base::PersistentHash of the UTF-8 bytes
  }
}
```

Exactly one arm is set. `WriteCompressibleString(value, out)` picks between the
two value arms:

1. If `value.size() < kSyncCompressionThresholdBytes` (256 bytes by default):
   write `raw`. Small inputs gain nothing from gzip and often grow under
   compression overhead.
2. Otherwise, gzip. If the gzipped bytes are smaller than the input, write
   `gzipped`. Else fall back to `raw`.

`OmitCompressibleString(out)` sets the third arm: it hashes `out`'s current
plaintext (decompressing first if it was `gzipped`) and stores that in
`omitted_content_hash`, which — being part of the `oneof` — clears the value.

`ReadCompressibleString(in)` returns:

- `std::nullopt` if `omitted_content_hash` is set, _or_ if gzip decompression
  failed.
- The decoded string otherwise (which may be empty).

Both helpers live in
`brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.{h,cc}`.

### Fields wrapped in `AIChatCompressibleString`

Anywhere the local store can hold a long-form string:

- `AIChatAssociatedContentProto.last_contents` — extracted page text
- `AIChatEntryEventProto.completion` — assistant's response text
- `AIChatWebSource.page_content` — fetched web-page snippet
- `AIChatWebSourcesEvent.rich_results` (repeated) — JSON SERP payloads
- `AIChatWebSourcesContentBlock.rich_results` (repeated) — same, inside tool
  output content blocks
- `AIChatInlineSearchEvent.results_json`
- `AIChatToolUseEvent.arguments_json`
- `AIChatTextContentBlock.text` — text inside tool-output content blocks
- `AIChatToolArtifact.content_json`
- `AIChatUploadedFile.extracted_text`

Short fields (`title`, `url`, UUIDs, integer counters, etc.) stay as plain proto
types — no benefit from wrapping and the overhead would be net negative.

### Uploaded file bytes

`AIChatUploadedFile.data` is plain `bytes` (no gzip — image/PDF bytes are
already compressed). It shares a `oneof` with `omitted_data_hash` (a
`base::PersistentHash` of the bytes), the content-hash restore signal for binary
attachments.

### Size-budget policy

`FitEntryWithinSyncBudget(Entry*)` is called after building an Entry proto and
before committing it. If the serialized size already fits under
`kSyncMaxRecordBytes` (350 KB by default; 50 KB headroom under the 400 KB hard
cap for sync framing and encryption overhead), it is a no-op. Otherwise it walks
fields in priority order, omitting each (replacing it with a content hash) until
the entry fits:

1. `uploaded_files[].data` — raw file bytes. Omitted first because the receiver
   gets a degraded experience either way (no image / no PDF preview) and we
   still keep the file metadata + extracted text.
2. `associated_content[].last_contents` — page text. Most re-derivable since the
   receiver still has the URL.
3. `uploaded_files[].extracted_text` — derivable from the file bytes (which may
   already have been omitted).
4. `web_sources[].page_content` — search-result snippets.
5. `tool_use.output[*].text_content_block.text` — tool output text.
6. `tool_use.arguments_json` — tool call arguments.
7. `web_sources[].rich_results` — JSON SERP payloads.
8. `completion` — assistant's response text. Last because losing this
   significantly degrades a re-synced conversation.

After every step the function re-measures `ByteSizeLong()` and returns early as
soon as the entry fits.

If every omittable field has been omitted and the entry is _still_ over budget
(e.g. a 400 KB user-typed message in `entry_text`), the function returns
`false`. Callers refuse the commit. The bridge logs the entry UUID and
continues; the entry remains local-only until the user edits it down.

### Restore-local apply

An omitted field (`omitted_content_hash` / `omitted_data_hash` set) means "I had
this locally but couldn't fit it on the wire — restore your copy if it matches
this hash."

The bridge handles restore-local in proto space before converting to mojom:

```
ApplyRemoteRecord(specifics):
   if entry kind:
     merged = specifics
     RestoreOmittedFieldsFromLocal(merged.mutable_entry())
     // If nothing was omitted, returns immediately. Otherwise builds a
     // hash -> content map of every value the local copy of this entry still
     // holds (its compressible strings + uploaded-file bytes via
     // GetConversationData, plus archived AC texts via
     // GetArchiveContentsForConversation), then rewrites each omitted field
     // whose hash matches a local value. A hash miss leaves the field omitted.

     (entry, ac_list, ac_texts) = SpecificsToEntry(merged)
     database->ApplyRemoteEntry(conv_uuid, entry, ac_list, ac_texts)
```

This keeps the DB layer simple — it does the usual full-replace of the entry —
and centralizes the restore-local logic in one place. Matching by content hash
(rather than by field position) means a value is only ever restored when the
local copy is byte-identical to what the sender dropped.

A field that is _absent_ on the wire **and** carries no hash is treated as
preserve-local (forward-compat: an older sender that doesn't know about the
field). Only an explicitly-set empty value is interpreted as "the sender meant
to clear this."

### ContentBlock variants

`mojom::ContentBlock` is a union with 16 variants. Only the variants the local
store persists are synced; the rest are runtime-only and intentionally not
represented in the proto:

| Synced | Variant                                   |
| ------ | ----------------------------------------- |
| ✓      | `ImageContentBlock`                       |
| ✓      | `TextContentBlock`                        |
| ✓      | `WebSourcesContentBlock`                  |
| —      | `FileContentBlock`                        |
| —      | `FileExtractedTextContentBlock`           |
| —      | `PageExcerptContentBlock`                 |
| —      | `PageTextContentBlock`                    |
| —      | `VideoTranscriptContentBlock`             |
| —      | `RequestTitleContentBlock`                |
| —      | `ChangeToneContentBlock`                  |
| —      | `MemoryContentBlock`                      |
| —      | `FilterTabsContentBlock`                  |
| —      | `SuggestFocusTopicsContentBlock`          |
| —      | `SuggestFocusTopicsWithEmojiContentBlock` |
| —      | `ReduceFocusTopicsContentBlock`           |
| —      | `SimpleRequestContentBlock`               |

The persisted set matches what `proto_conversion.cc` writes into the local
`tool_use_serialized` BLOB.

## Merging strategy

### Outbound (local → remote)

| Local event                                    | Records emitted                                                                                                                         |
| ---------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------- |
| First entry in a new conversation              | One Conversation `Put` + one Entry `Put`                                                                                                |
| Subsequent entry                               | One Entry `Put` + one Conversation `Put` (metadata may have moved: model, tokens)                                                       |
| Streaming tool-use update on an existing entry | One Entry `Put` (full re-serialize of that entry; gzip + size-budget omission applied if needed)                                        |
| Entry edit removal                             | One Entry `Delete`                                                                                                                      |
| Conversation deletion                          | One Entry `Delete` per child entry + one Conversation `Delete`. Must run **before** the DB delete so the bridge can enumerate children. |
| Title / token / model change                   | One Conversation `Put`                                                                                                                  |

### Inbound (remote → local)

- `MergeFullSyncData`: walks remote changes, calls `ApplyRemoteRecord` for each
  ADD/UPDATE, then uploads any local records whose storage keys aren't already
  in the remote set.
- `ApplyIncrementalSyncChanges`: DELETE dispatches on prefix
  (`DeleteConversation` for `c:`, `DeleteConversationEntry` for `e:`);
  ADD/UPDATE dispatches via `ApplyRemoteRecord` (which applies the restore-local
  merge first).

### Conflict resolution

**Last-writer-wins per record**, with one refinement: fields the sender omitted
do not participate in the last-writer-wins comparison — a byte-identical local
value is restored for those.

Because each record is independently keyed and there's no inter-record
consistency check, the apply path is order-independent:

- Entry arrives before its parent Conversation? `ApplyRemoteEntry` inserts a
  stub conversation row (`INSERT OR IGNORE`) so the entry has a parent. The
  later metadata record fills in title/model/tokens via `INSERT OR REPLACE`.
- Conversation arrives before any entries? Sits with empty entry list until
  entries come in.
- Conversation deleted while entries are still in flight? The entries arrive and
  resurrect a stub conversation; the next remote-delete sweep for those entries
  cleans them up. (Bounded staleness, not a correctness issue.)

## Bridge lifecycle and threading

The bridge lives on a dedicated `db_task_runner_` alongside the database. All
`DataTypeSyncBridge` methods (`MergeFullSyncData`,
`ApplyIncrementalSyncChanges`, `GetDataForCommit`, etc.) run on that sequence,
and the bridge holds a raw pointer to `AIChatDatabase`. The UI thread interacts
with the bridge through:

- `ProxyDataTypeControllerDelegate` for everything sync-engine-driven
  (controller registration, sync starting/stopping, getting data on commit).
- The outbound notification path (`AIChatService::OnConversation*` /
  `OnConversationEntry*`), which `PostTask`s into the bridge sequence.

### `AIChatSyncBackend` and async bridge construction

`AIChatService::CreateSyncControllerDelegate()` is called by
`CommonControllerBuilder` during sync-service startup. That is **earlier** than
the bridge can exist: the bridge depends on an `AIChatDatabase`, which depends
on an `os_crypt_async::Encryptor`, which is asynchronous under platforms like
macOS Keychain. If the delegate factory returned `nullptr` while the bridge
wasn't yet built, the `AI_CHAT_CONVERSATION` controller would never be added to
the controller list and the type wouldn't appear in
`GetRegisteredSelectableTypes()` (the desktop "Leo AI" checkbox is gated on that
via `hidden="[[!syncPrefs.aiChatRegistered]]"`).

To make registration succeed before the bridge is ready, the service holds an
`AIChatSyncBackend` (its own file, `sync/ai_chat_sync_backend.{h,cc}`). It is
owned UI-side but only ever touches the bridge on `db_task_runner_`, and
outbound local-change notifications are also routed through it (rather than a
`WeakPtr` to the bridge) so the posting closure keeps it alive across the hop:

```cpp
class AIChatSyncBackend
    : public base::RefCountedDeleteOnSequence<AIChatSyncBackend> {
  explicit AIChatSyncBackend(scoped_refptr<base::SequencedTaskRunner> runner);
  void SetBridge(std::unique_ptr<AIChatSyncBridge> bridge);  // on db sequence
  base::WeakPtr<syncer::DataTypeControllerDelegate>
      GetControllerDelegate();                               // on db sequence
  void Shutdown();                                           // on db sequence
  // ... OnConversation* notification forwarders (on db sequence) ...
 private:
  std::unique_ptr<AIChatSyncBridge> bridge_;
  SEQUENCE_CHECKER(sequence_checker_);
};
```

`MaybeInitStorage()` creates `db_task_runner_` and `sync_backend_`
**synchronously** the first time it runs. From then on,
`CreateSyncControllerDelegate()` can always hand out a working delegate:

```cpp
return std::make_unique<syncer::ProxyDataTypeControllerDelegate>(
    db_task_runner_,
    base::BindRepeating(&AIChatSyncBackend::GetControllerDelegate, sync_backend_));
```

When `OnOsCryptAsyncReady` later fires, it `PostTask`s to the DB sequence to
construct the bridge and call `sync_backend_->SetBridge(...)`. Any proxy
delegate already handed to the sync engine starts returning the real bridge from
that moment on; before then, the backend returns a null weak_ptr and the sync
engine treats the controller as not-yet-running.

A `os_crypt_init_pending_` flag guards `MaybeInitStorage()` against firing a
second `os_crypt_async::GetInstance()` request before the first one returns —
without it, a pref-change ping during init could race with the constructor's
init and try to install the bridge twice.

### Outbound notification path

`AIChatService::OnConversation*` and `OnConversationEntry*` on the UI thread
`PostTask` into the bridge sequence. They reference the bridge via a weak
pointer cached on the service. Crucially, `OnConversationDeleted` runs
**before** the DB delete: the bridge enumerates surviving entries via
`GetConversationData`, emits one `Delete` per entry storage key, then a `Delete`
for the conversation storage key, and only then does the DB row deletion run on
the same task runner.

### Storage toggles and re-sync

On-disk conversation storage is itself a user pref (`kBraveChatStorageEnabled`).
The bridge and backend outlive a storage toggle: disabling storage detaches the
database (`AIChatSyncBackend::ClearDatabase()` →
`AIChatSyncBridge::ClearDatabase()`, which nulls the bridge's `database_`) and
deletes it, while re-enabling re-attaches a fresh one via `SetDatabase()`. The
backend, bridge, and change processor are deliberately kept alive so the
`ProxyDataTypeControllerDelegate` the sync engine holds keeps resolving.

That keep-alive creates a problem of its own: detaching and re-attaching the
database tells the sync engine nothing. Across an off→on toggle the change
processor still believes the (now wiped and recreated) database is fully synced,
so it never re-runs `MergeFullSyncData`, and remote conversations are not
re-downloaded until the next browser restart reads the empty metadata.

The fix lives at the sync-service layer, in `AIChatDataTypeController`
(`sync/ai_chat_data_type_controller.{h,cc}`), a `syncer::DataTypeController`
subclass that gates the type on the storage pref:

- `GetPreconditionState()` returns `kMustStopAndClearData` while storage is off
  and `kPreconditionsMet` while on. When the engine sees `kMustStopAndClearData`
  it stops the type and clears its metadata (`OnSyncStopping(CLEAR_METADATA)` on
  the processor + `ApplyDisableSyncChanges` on the bridge), so the processor's
  in-memory entity tracker no longer disagrees with the emptied database.
- The controller owns a `PrefChangeRegistrar` on `kBraveChatStorageEnabled` and
  drives `SyncService::DataTypePreconditionChanged(AI_CHAT_CONVERSATION)` so the
  engine re-reads the precondition.

The two transitions are handled asymmetrically:

- **off** → the controller calls `DataTypePreconditionChanged` immediately from
  the pref observer; there is nothing to wait for, and the engine stops and
  clears the type.
- **on** → the controller does **not** trigger from the pref observer. Re-enable
  re-attaches the database asynchronously (`os_crypt` → `OnOsCryptAsyncReady` →
  `SetDatabase`), and restarting the type before that attach completes would let
  `MergeFullSyncData` run against a still-detached database and silently drop
  the merge. Instead the controller subscribes to
  `AIChatService::RegisterSyncDatabaseReadyCallback()` and calls
  `DataTypePreconditionChanged` from there — fired right after `SetDatabase` is
  posted, so on the database sequence the re-attach is ordered before any
  `MergeFullSyncData` the restarted type schedules. Because the metadata was
  cleared on the off transition, that restart is a fresh initial sync that
  re-downloads (and re-uploads) everything.

`AIChatSyncBridge::ClearDatabase()` deliberately does **not** reset its
`model_ready_to_sync_` guard: the change processor stays model-ready across a
sync stop (its `OnSyncStopping()` CHECKs that), so re-firing
`ModelReadyToSync()` on the next `SetDatabase()` would crash. The precondition
stop clears the processor's entity tracker without disturbing
`model_ready_to_sync_`, which is exactly what a fresh initial sync needs. See
https://github.com/brave/brave-browser/issues/53978.

## Notifying the UI on remote apply

`MergeFullSyncData` / `ApplyIncrementalSyncChanges` write remote rows directly
into the DB on the bridge sequence; nothing about that path inherently tells the
UI thread that the world changed. To close the loop, the bridge takes a
`base::RepeatingClosure on_remote_changes_applied` at construction time.
`AIChatService` builds it as

```cpp
base::BindPostTask(
    base::SequencedTaskRunner::GetCurrentDefault(),
    base::BindRepeating(&AIChatService::OnRemoteSyncDataApplied, weak_ptr));
```

so the closure marshals from the bridge sequence to the UI sequence by itself.
The bridge invokes it **once at the end of each batch** that actually mutated
the DB — not per-entity (would thrash) and not at all on empty /
delete-of-missing batches.

On the UI thread, `AIChatService::OnRemoteSyncDataApplied`:

1. For each active `ConversationHandler` in `conversation_handlers_`,
   async-fetches a fresh `ConversationArchive` from the DB and calls
   `handler->OnRemoteSyncDataApplied(archive)`. The handler:
   - Stops any in-flight LLM request (`is_request_in_progress_ = false`,
     `engine_->ClearAllQueries()`, `OnAPIRequestInProgressChanged()`).
   - Stops any tool-use loop (`StopTask()`).
   - Replaces `chat_history_` and reloads associated content from the archive.
   - Calls `OnHistoryUpdate(nullptr)` so the WebUI re-fetches.
2. Calls `ReloadConversations()` + `OnConversationListChanged()` so the sidebar
   reflects any conversations that newly appeared or got deleted.

The stop-in-flight step is deliberately blunt: a streaming reply that was about
to land would have been generated against history that the remote write just
replaced, so committing it would corrupt the new state. A smarter buffered-merge
is listed under future improvements.

## Storage keys and client tags

```
storage_key = client_tag = "c:" + conversation_uuid   for Conversation records
storage_key = client_tag = "e:" + entry_uuid          for Entry records
```

The bridge's `GetStorageKey` / `GetClientTag` both delegate to
`GetStorageKeyFromSpecifics`, which inspects `has_conversation()` /
`has_entry()` and concatenates the prefix.

The two prefix namespaces (`c:` and `e:`) are disjoint, so
`ApplyIncrementalSyncChanges` can dispatch DELETEs on the storage key alone
without re-parsing the proto:

```cpp
if (storage_key.starts_with(kConversationStorageKeyPrefix)) {
  database_->DeleteConversation(storage_key.substr(2));
} else if (storage_key.starts_with(kEntryStorageKeyPrefix)) {
  database_->DeleteConversationEntry(storage_key.substr(2));
}
```

## Bridge dispatch

`AIChatSyncBridge::ApplyRemoteRecord` is the single funnel for inbound
ADD/UPDATE. For Entry records it first runs the restore-local merge in proto
space, then converts to mojom for the DB call:

```cpp
void AIChatSyncBridge::ApplyRemoteRecord(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  if (specifics.has_conversation()) {
    auto conv = SpecificsToConversationMetadata(specifics);
    if (conv) database_->ApplyRemoteConversationMetadata(std::move(conv));
    return;
  }
  if (!specifics.has_entry()) return;

  sync_pb::AIChatConversationSpecifics merged = specifics;
  RestoreOmittedFieldsFromLocal(merged.mutable_entry());

  std::vector<mojom::AssociatedContentPtr> ac_list;
  base::flat_map<std::string, std::string> ac_texts;
  auto entry = SpecificsToEntry(merged, &ac_list, &ac_texts);
  if (!entry) return;

  std::vector<std::string> contents;
  contents.reserve(ac_list.size());
  for (const auto& ac : ac_list) {
    auto it = ac_texts.find(ac->uuid);
    contents.emplace_back(it != ac_texts.end() ? it->second : std::string());
  }
  database_->ApplyRemoteEntry(merged.entry().conversation_uuid(),
                              std::move(entry), std::move(ac_list),
                              std::move(contents));
}
```

## Database layer

### Upserts

`ApplyRemoteConversationMetadata` uses `INSERT OR REPLACE` on the `conversation`
table. Because `PRAGMA foreign_keys` is OFF on this DB and the child tables
(`conversation_entry`, `associated_content`,
`conversation_entry_uploaded_files`) reference `conversation_uuid` as a plain
string column rather than a real FK, the REPLACE does **not** cascade — child
rows are managed independently via their own per-record sync.

`ApplyRemoteEntry` runs three steps inside a transaction:

1. `INSERT OR IGNORE INTO conversation(uuid, NULL, NULL, 0, 0)` — creates a stub
   row if the parent metadata hasn't arrived yet. The fields are nullable
   defaults; the next `ApplyRemoteConversationMetadata` call REPLACEs them with
   real values.
2. `DeleteConversationEntry(uuid)` + `AddConversationEntry(...)` — full-replace
   the entry (and any child event/file rows). This reuses the existing local
   insert path so all event tables and the uploaded-files table are handled
   correctly.
3. `AddOrUpdateAssociatedContent(conv_uuid, ac_list, contents)` — re-adds any
   associated content rows with the caller-supplied content texts. The caller
   has already restored local values for any AC the sender omitted.

`INSERT OR REPLACE` was chosen over the SQLite UPSERT form
(`INSERT … ON CONFLICT(uuid) DO UPDATE SET …`) because Chromium's bundled SQLite
revision rejects the UPSERT syntax at `Statement::Run`. The REPLACE form has
equivalent semantics here.

### Schema versioning

The database schema is migrated forward by `AIChatDatabase::InitInternal` using
a per-version chain (`MigrateFromXToY`). Adding a new sync-related table or
column requires:

1. A `MigrateFromXToY` helper for the column/table addition.
2. A bump of `kCurrentDatabaseVersion`.
3. A SQL fixture under `components/test/data/ai_chat/` for the new "from"
   version so `AIChatDatabaseMigrationTest` exercises the migration.

The sync metadata table (`ai_chat_sync_metadata`) is the v10 → v11 migration;
`AIChatDatabase` implements `syncer::SyncMetadataStore` so the bridge's
`change_processor()` can persist entity metadata alongside the conversation
data.

## Server side (go-sync)

The Brave sync server is schema-blind for the inner proto. From
`go-sync/datastore/sync_entity.go`:

```go
type SyncEntity struct {
    ...
    Specifics []byte    // opaque serialized proto
    DataType  *int      // 2000001 for AI Chat
    ...
}
```

`InsertSyncEntity` does one `dynamodb.PutItem` per entity. DynamoDB enforces the
400 KB hard limit on the entire row (all attributes combined). Each Conversation
or Entry produces a separate `CommitMessage.Entries[i]` → separate
`entityToCommit` → separate row, so each one independently fits within the
budget once size-budget omission has run.

The sync engine's HTTP-transport gzip (`Content-Encoding: gzip` in
`http_bridge.cc`) layered _over_ this reduces wire bandwidth but does not relax
the per-row 400 KB cap — DynamoDB stores the bytes as the application emitted
them.

The server's `ai_chat_specifics.pb.go` must be regenerated whenever the `.proto`
schema changes. The DB layer doesn't care about the inner shape (opaque bytes),
but anything that visits the proto (debug tooling, future server-side filters)
requires the regenerated stub.

## Proto schema

```protobuf
message AIChatCompressibleString {
  oneof value {
    string raw = 1;
    bytes gzipped = 2;                 // RFC 1952 gzip of the UTF-8 bytes
    fixed32 omitted_content_hash = 3;  // base::PersistentHash of the UTF-8 bytes
  }
}

message AIChatConversationSpecifics {
  message Conversation {
    optional string uuid = 1;
    optional string title = 2;
    optional string model_key = 3;
    optional uint64 total_tokens = 4;
    optional uint64 trimmed_tokens = 5;
  }
  message Entry {
    optional string uuid = 1;
    optional string conversation_uuid = 2;  // parent backlink
    optional int64 date_unix_epoch_micros = 3;
    optional string entry_text = 4;
    optional string prompt = 5;
    optional int32 character_type = 6;       // HUMAN | ASSISTANT
    optional string editing_entry_uuid = 7;
    optional int32 action_type = 8;
    optional string selected_text = 9;
    optional string model_key = 10;
    repeated AIChatEntryEventProto events = 11;
    repeated AIChatAssociatedContentProto associated_content = 12;
    repeated AIChatUploadedFile uploaded_files = 13;
    optional AIChatSkillEntry skill = 14;
    optional AIChatNEARVerificationStatus near_verification_status = 15;
  }
  oneof kind {
    Conversation conversation = 1;
    Entry entry = 2;
  }
}

message AIChatSkillEntry {
  optional string shortcut = 1;
  optional string prompt = 2;
}

message AIChatNEARVerificationStatus {
  optional bool verified = 1;
}

message AIChatAssociatedContentProto {
  optional string uuid = 1;
  optional string title = 2;
  optional string url = 3;
  optional int32 content_type = 4;          // mojom::ContentType
  optional int32 content_used_percentage = 5;
  optional AIChatCompressibleString last_contents = 6;
}

message AIChatUploadedFile {
  optional string filename = 1;
  optional uint32 filesize = 2;
  optional int32 type = 3;                  // mojom::UploadedFileType
  oneof data_or_hash {
    bytes data = 4;
    fixed32 omitted_data_hash = 5;          // base::PersistentHash of the bytes
  }
  optional AIChatCompressibleString extracted_text = 6;
}
```

`AIChatEntryEventProto`, its event variants, and `AIChatToolUseEvent` follow the
same convention: long strings inside them are `AIChatCompressibleString`. See
`ai_chat_specifics.proto` for the full schema.

The outer `AIChatConversationSpecifics` is the field on `EntitySpecifics` (field
number 2000001). Every sync entity for this DataType has exactly one of
`conversation` or `entry` set.

## Testing

### What's covered

- **`AIChatSyncConversionsTest`** (`ai_chat_sync_conversions_unittest.cc`)
  - Round-trip every proto event type (completion / search queries / web sources
    / inline search / tool use) and every persisted ContentBlock variant (text,
    image, web sources).
  - Round-trip uploaded files (with both raw bytes and gzipped extracted text),
    associated content text.
  - `AIChatCompressibleString` small-value bypass, large-value gzip, and the
    omitted-content hash (hashing gzipped plaintext).
  - Round-trip `skill` and NEAR verification status.
  - `FitEntryWithinSyncBudget` no-op below threshold, omits file bytes first,
    falls through multiple categories, refuses pathological entries.
  - Storage key prefix encoding/decoding, null returns for wrong-kind specifics.
- **`AIChatSyncBridgeTest`** (`ai_chat_sync_bridge_unittest.cc`)
  - Merge uploads both kinds; incremental DELETE for each kind.
  - `OnConversationDeleted` emits child-then-parent deletes,
    `OnConversationEntryAdded` / `Deleted` emits the right kind.
  - `ApplyIncrementalSyncChanges` upsert path for both kinds.
  - Orphan-entry stub creation, stub upgrade by a later metadata record, no-op
    on delete of an entry that never existed locally.
  - Restore-local on AC `last_contents` and on an event `completion` when the
    remote sender omitted them (matched by content hash).
  - `ApplyDisableSyncChanges` clears metadata.
  - `on_remote_changes_applied` closure fires once per batch that actually
    changed the DB, never on empty / no-op batches.
- **`AIChatServiceUnitTest`** (`ai_chat_service_unittest.cc`)
  - `SyncBackendSurvivesStorageToggle`: the backend and its controller delegate
    stay valid across a storage off→on toggle.
  - `SyncDatabaseReadyCallbackFiresOnStorageReEnable`: re-enabling storage
    re-attaches the database and notifies `RegisterSyncDatabaseReadyCallback()`
    listeners.
  - `DataTypeControllerPreconditionTracksStoragePref`:
    `AIChatDataTypeController` reports
    `kMustStopAndClearData`/`kPreconditionsMet` from the storage pref, calls
    `DataTypePreconditionChanged` immediately on disable, and re-evaluates on
    re-enable via the database-ready signal (not directly from the pref).
- **`AIChatSyncBrowserTest`** (`browser/sync/ai_chat_sync_browsertest.cc`)
  - End-to-end registration: with `BraveSyncAIChat` enabled and storage on,
    `GetUserSettings()->GetRegisteredSelectableTypes()` contains `kAIChat`.
- **`AIChatDatabaseMigrationTest`** walks the full v1 → vN migration chain.

### Known testing limitations

The current coverage tests the conversion layer, the bridge logic, the
controller-registration plumbing, and the migration chain, but it does **not**
cover the full end-to-end data flow against real sync infrastructure:

1. **No fake-server browser test that drives a sync session.** The browser test
   asserts `kAIChat` is registered; it does not call `SetupSync()`, inject
   entities, or verify that the inbound apply path succeeds against arbitrary
   local DB content. Bridge unit tests cover the apply logic but use minimal
   seed conversations (no events, no associated content, no uploaded files), so
   the inner SELECTs in `GetConversationEntries` short-circuit to zero rows and
   any schema / query mismatch on a populated DB goes uncaught.
2. **Bridge unit tests use a clean schema only.** Every test builds a fresh
   `AIChatDatabase` via `CreateSchema()`, which always produces the
   current-version schema with every column present. They never run against the
   artifact of a migration chain, so a "migration ran but didn't actually add
   the column" failure mode wouldn't be caught here.
3. **Active-handler refresh on remote apply is not asserted end-to-end.** Bridge
   tests verify that the `on_remote_changes_applied` closure fires when it
   should, but nothing asserts that the closure causes
   `ConversationHandler::OnRemoteSyncDataApplied` to actually stop an in-flight
   engine request and reload history.
4. **Async-init race is covered by inspection, not by test.** The
   `AIChatSyncBackend` design specifically targets the case where the sync
   engine asks for a controller delegate before the bridge exists. No test
   simulates that ordering; the browser test incidentally exercises it but
   doesn't pin the ordering invariant.
5. **No coverage for closure marshaling.** Bridge tests use `base::DoNothing()`
   or a direct counter as the closure. The `BindPostTask` wrapping happens only
   in `AIChatService` and is not exercised.

### Recommended follow-up testing

In priority order:

1. **End-to-end `SyncTest` browser test.** Subclass `SyncTest`, work around the
   Brave signin model (alternate identity flow or short-circuit the harness
   check), `SetupSync()`, inject one Conversation and a few Entries via the fake
   server's `InjectEntity`, then assert:

   - The local DB has the conversation + entries.
   - `OnRemoteSyncDataApplied` fired on the UI thread.
   - The conversation appears in `GetAllConversations()`.
   - If a `ConversationHandler` was open for it, its `chat_history_` refreshed
     and any in-flight request was stopped.
   - After toggling `kBraveChatStorageEnabled` off then on, the seeded remote
     conversation is re-downloaded into the recreated database (the
     `AIChatDataTypeController` precondition re-sync path). This is the
     meaningful regression test for the storage-toggle behavior;
     `DataTypeControllerPreconditionTracksStoragePref` covers the controller
     logic without an engine, but only a fake server exercises the actual
     re-download.

   This single test, done right, closes most of the gaps above.

2. **Rich-conversation bridge test.** A test helper that seeds an entry with one
   of each event type, one AC row with `last_contents`, and one uploaded file
   with `extracted_text`. Re-run the merge tests against that — every inner
   SELECT in `GetConversationEntries` now actually steps through rows. Catches
   schema-vs-code drift without needing a fake server.
3. **Migration-then-sync test.** Add a variant of `AIChatDatabaseMigrationTest`
   that, after migration, runs `MergeFullSyncData` and confirms it completes
   without invalid statements. Catches "ALTER TABLE silently failed but
   meta-version was bumped" classes of bugs.
4. **`ConversationHandler::OnRemoteSyncDataApplied` unit test** with a mock
   engine that reports `is_request_in_progress_` true and a mock tool provider
   with a `kRunning` task — assert both are stopped, history is replaced, and
   the UI is notified.
5. **`AIChatSyncBackend` ordering test.** Create the backend without a bridge,
   call `GetControllerDelegate()` on its sequence (expect null), then
   `SetBridge(...)`, then re-call `GetControllerDelegate()` (expect the real
   delegate). Pins down the contract the proxy delegate relies on.
6. **Bridge fuzz / malformed-payload test.** Throw invalid or truncated
   specifics at `ApplyIncrementalSyncChanges` and confirm the bridge ignores
   them without crashing or corrupting the DB. Particularly valuable now that
   the proto schema is large enough that round-trip tests can't reasonably cover
   every shape.

## Future improvements

1. **Entry-level union merge.** `ApplyRemoteEntry` is full-replace per entry, so
   concurrent edits to the same entry from two clients are last-writer-wins on
   the whole entry. A future iteration could union-merge `events` (each event
   has `event_order`) so streaming tool-use updates from two clients don't
   clobber each other. Realistically only matters if two clients are actively
   driving the same conversation simultaneously.
2. **Smarter active-conversation merge.** When a remote batch lands on a
   conversation the user has open, the active `ConversationHandler` stops any
   in-flight LLM request or tool-use loop and reloads from the DB. That's
   correct but blunt — a streaming response gets cut off rather than merged. A
   future iteration could buffer remote ADD/UPDATE for _actively-streaming_
   conversations and apply on stream completion (deletes still apply
   immediately, to allow remote cleanup).
3. **Chunked records for oversized fields.** When even a single field exceeds
   400 KB (e.g. a 5 MB tool fetch output, a 2 MB image), the size-budget policy
   omits the field. A future iteration could spill the offending field into
   ordered side-records keyed `k:<entry-uuid>:<chunk-index>` and reassemble on
   the receiver. Adds a third record kind to the `oneof` and a fourth stub level
   (chunk → entry → conversation), worth the complexity once telemetry indicates
   omission fires often.
4. **Per-event records.** A natural extension of the per-entry split, worth it
   if event re-sync churn becomes a measured problem.
5. **Per-file records for multi-file uploads.** Most turns have at most one
   uploaded file. When multiple are attached, the size-budget policy omits bytes
   from files in array order. A future iteration could give each file its own
   400 KB budget.
6. **Telemetry.** Histograms on omission rates per field would tell us whether
   chunking, per-event records, or per-file records are worth the complexity. A
   comment in `ai_chat_sync_conversions.cc` marks the natural insertion point
   near the size-budget policy.
7. **Quota awareness.** The server enforces a 50,000-object per-client quota
   (`maxClientObjectQuota`). A heavy user with 1,000 conversations × 50 turns =
   51,000 objects. If telemetry shows users approaching it, we may need
   lifecycle policies (archive cold conversations, sync only last N).
