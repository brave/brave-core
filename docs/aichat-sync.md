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
types — no benefit from wrapping and the overhead would be net negative. Every
field in the list above is reachable by the size-budget policy below; wrapping a
field without listing it there would make it un-omittable.

### Uploaded file bytes

`AIChatUploadedFile.data` is plain `bytes` (no gzip — image/PDF bytes are
already compressed). It shares a `oneof` with `omitted_data_hash` (a
`base::PersistentHash` of the bytes), the content-hash restore signal for binary
attachments.

### Size-budget policy

`FitEntryWithinSyncBudget(Entry*)` is called after building an Entry proto and
before committing it. If the serialized size already fits under
`kSyncMaxRecordBytes` (256 KB by default), it is a no-op. Otherwise it drops
fields (replacing each with a content hash) until the entry fits.

The cap is measured on the plaintext specifics, but AI_CHAT_CONVERSATION is an
encryptable type: what reaches the server is `Nigori::Encrypt()` of those bytes
— an IV, AES-CBC ciphertext and an HMAC, base64-encoded. Base64 makes that
overhead multiplicative rather than additive, so a record at the cap commits at
roughly `4/3 * (size + 64)` bytes: 256 KB arrives near 341 KB, leaving ~59 KB
under the 400 KB per-entity limit for the rest of the SyncEntity.

The drop order weighs what a field is worth as context for continuing the
conversation on another device against how much of the budget it costs to carry.
Most of it the model can do without or regenerate; what it cannot replace is
what the conversation actually said, so that goes last.

`uploaded_files[].data` goes first, ahead of everything else. The bytes are real
context — without them the receiver cannot open the attachment at all — but one
file can take most of the budget, and already-compressed image or PDF data will
not shrink any further. The filename, size, type and extracted text still
travel. Within that category the largest file goes first and the pass stops as
soon as the entry fits, so one oversized attachment does not cost the small
ones; only the visit order is sorted, and `uploaded_files` keeps its original
order on the wire.

The remaining categories are `AIChatCompressibleString` fields, dropped in this
order:

1. `associated_content[].last_contents` — page text. Bulk context the replies
   already reflect, and the receiver still has the URL to re-derive it from.
2. `uploaded_files[].extracted_text` — text extracted from an attachment. Bulk
   context, same reasoning.
3. `inline_search.results_json` — never reaches the model, and not read
   directly.
4. `web_sources[].page_content` — snippets fetched for cited pages, both on a
   standalone `web_sources` event and inside a `web_sources_content_block` in
   tool output. Costly, and the replies drew on them already.
5. `tool_use.output[*].text_content_block.text` — tool output the model worked
   from, likewise reflected in the replies.
6. `tool_use.arguments_json` — small, and the model can produce them again.
7. `web_sources[].rich_results` — rich search result JSON, again both at event
   level and inside tool output. Costly, and only used to re-render results.
8. `tool_use.artifacts[].content_json` — never reaches the model, but the user
   opens these.
9. `completion` — the assistant's replies. Last: the conversation itself, and
   the one thing nothing else can stand in for.

The function re-measures `ByteSizeLong()` between categories and returns as soon
as the entry fits, so it drops no more than it has to. A field already omitted
by an earlier pass is skipped, and so is an explicitly-empty raw string:
`ReadCompressibleString()` reports that as a value rather than an omission, and
replacing it with a hash would both lose that distinction and cost more bytes
than the empty string it replaced.

`ForEachOmittableString(Entry*, visitor)` enumerates exactly the fields in that
list. `AIChatSyncBridge::RestoreOmittedFieldsFromLocal` uses it rather than
keeping a second copy of the list, so the restore side cannot drift from what
the policy actually drops. A compressible field added to the proto but not to
the list can never be omitted — and an entry that only that field makes
oversized becomes uncommittable.

If every omittable field has been omitted and the entry is _still_ over budget
(e.g. a 400 KB user-typed message in `entry_text`, or a large `selected_text` —
both plain proto strings that no omission can shrink), the function returns
`false` and the entry cannot be committed at all; it stays local-only until the
user edits it down.

Both callers must then **untrack the entity explicitly**, via
`change_processor()->UntrackEntityForStorageKey()` — `PutEntry()` before
returning, and `GetDataForCommit()` instead of leaving the key out of the batch.
Silently omitting a key the processor asked for is not neutral:
`ClientTagBasedDataTypeProcessor::ConsumeDataBatch` treats a
requested-but-absent key as a bridge bug, records
`Sync.DataTypeOrphanMetadata.GetData`, and untracks it anyway. The end state is
the same either way, so this is about not emitting a Chromium histogram that
reads as a defect in Brave's bridge.

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

**Known gap: a hash miss loses a diverging local value.** "A hash miss leaves
the field omitted" is only safe when the receiver has no copy. If the receiver
holds a copy that has since diverged, the omitted field decodes to `""` (see the
`ReadRestoredString` note below) and the diverging local value is **overwritten
with empty**, not left alone — the hash can confirm a match but cannot
distinguish "receiver has nothing" from "receiver has something different." Same
for the associated-content texts, where a missing map entry yields an empty
string (`ai_chat_sync_bridge.cc:583-586`). Narrow in practice, because it needs
an over-budget field _and_ a locally-modified copy of that same field, but it is
real data loss rather than a fidelity gap. Tracked in
https://github.com/brave/brave-browser/issues/53978.

**`RestoreOmittedFieldsFromLocal` is a precondition of `SpecificsToEntry`, not
an optimisation.** Several mojom fields the proto marks compressible are
non-optional strings (`completion`, tool-use `arguments_json`,
`text_content_block.text`, artifact `content_json`, inline-search
`results_json`), so there is no mojom representation for "absent" and they
decode to `""`. Without the restore pass having run first, a re-sync of an
existing entry would blank locally-present content. The five sites go through a
named `ReadRestoredString()` helper in `ai_chat_sync_conversions.cc` whose
comment says so, because the restore happens in a different file and three
readers in review have misread the bare `.value_or(std::string())` as a plain
default.

### Edit history on apply

`EntryToSpecifics` collapses an edited turn to its latest revision's content
under the **original** turn's identity, so edit history never goes out over the
wire and an incoming record never carries `edits`. A plain full-replace would
therefore delete revisions the receiving device may be the only holder of —
including the original text, which lives in the head row while the remote
carries the latest revision there. Note the distinction:

- **Fidelity gap, by design.** A device that receives a turn it never edited
  gets the latest revision as its head text and no history. It never held the
  original; the authoring device still does.
- **Destruction, not acceptable.** A device that holds revisions only it has
  must not lose them to an incoming update.

So `ApplyRemoteRecord` rebuilds the entry around the local copy
(`PreserveLocalEditHistory`) before the apply, whenever
`GetConversationEntryWithEdits` finds local revisions: the local head and its
revisions are kept, and the remote content is appended as one more revision
unless it already **is** the latest one.

**That read is gated on a per-batch snapshot, not done per record.** Nothing in
this database is indexed on `editing_entry_uuid` (there are no indices at all),
so a lookup by it is a full scan of `conversation_entry` — and an initial sync
applies every record, which would make it O(entries²). Each batch therefore
calls `GetEntryUuidsWithEditRevisions()` once, and `PreserveLocalEditHistory`
returns immediately unless that set names the entry. The snapshot cannot go
stale within a batch: an apply only writes the revisions it was handed, sync
delivers at most one change per storage key per batch, and a stale "has
revisions" just leads to a read that finds none.

Comparison is on decoded mojom with identity and unsynced fields normalised away
— never on serialized protos, since gzip output can differ across zlib versions
for identical plaintext. Its failure modes are asymmetric in the safe direction:
a false "differs" appends a redundant revision (ugly, not lossy), a false
"matches" fails to converge content. Neither destroys anything.

The appended revision is dated at arrival time, because revisions are ordered by
`created_time` and the remote carries the original turn's. Without that, the
appended revision could sort before an existing one, and the next apply would
compare against the wrong latest and append again on every batch.

This is not a remote-device-only path. `ApplyDisableSyncChanges` clears metadata
but not data, so unchecking "AI Chat" in sync settings and re-checking it
replays every record through `MergeFullSyncData` against a populated database —
one purely local action with no remote device involved.
`AIChatSyncBridgeTest.MergeFullSyncDataDoesNotDuplicateLocalEdits` is the
regression test for exactly that path.

The DB layer stays a dumb full-replace writer and needs no change for this:
`ApplyRemoteEntry` hands `entry->edits` to `AddConversationEntry`, which already
persists a revision by calling itself.

**This does depend on `DeleteConversationEntry` disposing of a revision's own
child rows.** A revision is a `conversation_entry` row in its own right and owns
a full set of event and file rows keyed by its **own** uuid, so a flat
`DELETE … WHERE editing_entry_uuid = ?` orphans those, and re-inserting the same
revision then hits the silent
`PRIMARY KEY(conversation_entry_uuid, event_order)` collision documented under
`ApplyRemoteEntry` below. `AIChatDatabase::DeleteEntryAndOwnedRows` is what
makes that safe; it walks revisions iteratively with a visited set rather than
recursing, because entry uuids are caller-supplied and an `editing_entry_uuid`
cycle cannot be ruled out.
`AIChatDatabaseSyncTest. ApplyRemoteEntryReplacesEditChildRows` is the
regression test, and it fails if that walk is reduced to a flat delete.

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

**Last-writer-wins per record** in the common case, arbitrated by Chromium
Sync's per-entity server version plus the processor's stored hash of the
last-committed ("base") data. **Nothing here is timestamp-based, and nothing
should become timestamp-based.** `created_time` is part of an entry's _identity_
— always the original turn's, so the entity stays stable across edits — not a
clock to compare versions with. Where this code does compare content, it
normalises `created_time` away on both sides (`IsSameRevisionContent`).

"Last writer" means the last writer the **server accepted**, which inverts under
true concurrency. If two devices both edit entry E from base version 5: A
commits and the server moves to v6; B's commit is rejected as stale; B then
receives v6, and since B's entity is still unsynced it resolves as a conflict →
`kUseRemote` → `ClientTagBasedRemoteUpdateHandler` **squashes B's pending
commit** and applies A's data. B does not retry. So under genuine concurrency
the **first** accepted write wins and the loser's edit is destroyed — not merely
rejected. Sequential writes behave the way "last writer wins" implies, because B
receives v6 cleanly, edits from it, and commits v7.

An entity is one Entry or one Conversation metadata record, never a whole
conversation, so two devices editing _different_ entries of the same
conversation do not conflict at all. A conflict needs both to touch the same
entry, or both to touch title/model/tokens.

**Not conflicting is not the same as reading well.** Entries are ordered for
display by `ORDER BY date ASC` (`ai_chat_database.cc:66`), i.e. by each entry's
`created_time` as stamped by the device that authored it. Two devices appending
to the same conversation concurrently therefore interleave by unsynchronised
wall clocks, and clock skew can order a reply before the message it answers.
This is the one place a timestamp decides anything, and it decides _presentation
order_, not which write wins — it does not contradict the rule above.

The bridge does not override `ResolveConflict`, so the
`DataTypeSyncBridge::ResolveConflict` default applies: remote wins, **except**
that a remote deletion resolves to `kUseLocal` — a locally-edited entry survives
a remote tombstone and is undeleted.

A stale record the server already accepted from this device cannot clobber a
newer local edit. In `ClientTagBasedRemoteUpdateHandler`: an update whose
version the entity already knows is dropped before the bridge sees it; a
conflict requires `IsUnsynced()`; and the triage resolves a remote record
identical to our own base data as `kIgnoreRemoteNoOpUpdate`, leaving the local
edit in place.

**The initial merge is the exception.** `MergeFullSyncData` applies every remote
record unconditionally and uploads only local keys the server lacks — with sync
metadata cleared there is no base data, so a conflict is undetectable in
principle. This is reachable with no second device: unchecking "AI Chat" in sync
settings stops the type with `CLEAR_METADATA`, so re-checking replays every
server record over a populated database. Two refinements keep that from losing
data: the restore-local apply above, and the edit-history merge in "Edit history
on apply".

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

Two things about how the controller gets registered are easy to get wrong:

- **The `BRAVE_BUILD_SYNC_CONTROLLERS` block must check `disabled_types`
  first.** `CommonControllerBuilder::SafeOptional::value()` CHECKs, and
  `ios/web_view/internal/sync/web_view_sync_service_factory.mm` — which _is_
  compiled into Brave iOS — calls `Build()` without ever calling
  `SetAIChatService()`. Reading `ai_chat_service_.value()` before
  `!disabled_types.Has(syncer::AI_CHAT_CONVERSATION)` crashes there. The guard
  works because `AI_CHAT_CONVERSATION` is a user type and that builder disables
  every user type it does not use; every other type in `Build()` is guarded the
  same way.
- **Do not add `DependsOn(AIChatServiceFactory)` to `SyncServiceFactory`**, even
  though it injects the service into the builder and upstream declares
  `DependsOn` for every other injected service. `AIChatServiceFactory`'s
  constructor reaches `SyncServiceFactory::GetInstance()` back through
  `ProfileMiscMetricsServiceFactory` → `PersonalDataManagerFactory`, so the edge
  recurses into the constructor being run and aborts in `__cxa_guard_acquire`.
  It is not needed: the controller's only hold on `AIChatService` is a
  `base::CallbackListSubscription`, and `CallbackListBase::Add()` binds the
  cancellation closure to a `WeakPtr` of the list, so the subscription no-ops if
  the service is destroyed first.

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

### Registration is gated on the feature, never on the storage pref

The sync backend (and with it the bridge) comes up on
`features::IsBraveSyncAIChatEnabled()` **alone** — `MaybeInitStorage()` creates
`db_task_runner_` and `sync_backend_` outside the `IsAIChatHistoryEnabled()`
branch — so `CreateSyncControllerDelegate()` always returns a working
`ProxyDataTypeControllerDelegate` and the controller is always registered.

This is not an optimisation; it is load-bearing. The registered type set is
fixed for the life of the SyncService: `SyncServiceImpl::Initialize()` "must be
called at most once", `DataTypeManagerImpl` holds
`const DataTypeController::TypeMap controllers_`, and `SyncUserSettingsImpl`
holds `const DataTypeSet registered_data_types_`. There is no API to add a
controller later. So if registration were gated on the storage pref, launching
with chat history off would leave the type absent for the entire session, and
turning history on could never turn sync on without a restart — not even
cosmetically, since `SetSelectedTypes()`/`GetSelectedTypes()` do
`types.RetainAll(GetRegisteredSelectableTypes())`, so an unregistered type
cannot be selected at all.

Splitting it this way matches upstream: `skills::SkillDataTypeController` gates
registration on `base::FeatureList::IsEnabled(features::kSkillsEnabled)` and its
precondition on the user pref `kChromeSkillsEnabled`;
`AutofillWalletDataTypeController` does the same with
`kAutofillCreditCardEnabled`. No upstream type gates registration on a
runtime-flippable pref.

**A bridge with no database — and therefore no `ModelReadyToSync()` — is a
supported state, not a tolerated one.** While the precondition is unmet,
`DataTypeManagerImpl` records the type in `data_type_status_table_` as failed,
so it never calls `LoadModels()`/`OnSyncStarting()`. It does still call `Stop()`
(`ModelLoadManager`: _"Call Stop() even on types not running to allow clearing
metadata"_), and `DataTypeController::Stop()` in `NOT_RUNNING` does only
`ClearMetadataIfStopped()` — its `CHECK` explicitly permits a null delegate in
that state, and `OnSyncStopping()`, the one that CHECKs `model_ready_to_sync_`,
is unreachable. `ClientTagBasedDataTypeProcessor::ClearMetadataIfStopped()` has
a dedicated `if (!model_ready_to_sync_) { pending_clear_metadata_ = true; }`
branch and honours it from `ModelReadyToSync()` later. Upstream unit-tests that
exact ordering (`ShouldClearMetadataIfStoppedUponModelReadyToSync`).

Two things to preserve if this area is touched:

- **Keep installing the bridge eagerly.**
  `AIChatSyncBackend::GetControllerDelegate()` returns null while `bridge_` is
  null, and a null delegate means `ProxyDataTypeControllerDelegate` silently
  drops `OnSyncStarting()` — the bug szilardszaloki found on #37540, leaving the
  type stuck in `MODEL_STARTING`. "Backend but no bridge" is the one shape to
  avoid.
- **Keep `GetPreconditionState()` reading the storage pref, not the feature
  flag.** If it ever reports `kPreconditionsMet` while the database is still
  detached, `OnSyncStarting()` sets `activation_request_` and `ConnectIfReady()`
  defers forever: the controller parks in `MODEL_STARTING`, and a later `Stop()`
  parks it in `STOPPING` awaiting a start that never completes. No crash, but
  the type hangs. The `RegisterSyncDatabaseReadyCallback` ordering above is what
  closes that window.

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
UI thread that the world changed. To close the loop, the bridge takes an
`AIChatSyncBridge::RemoteChangesAppliedCallback on_remote_changes_applied` at
construction time. `AIChatService` builds it as

```cpp
base::BindPostTask(
    base::SequencedTaskRunner::GetCurrentDefault(),
    base::BindRepeating(&AIChatService::OnRemoteSyncDataApplied, weak_ptr));
```

so it marshals from the bridge sequence to the UI sequence by itself. The bridge
invokes it **once at the end of each batch** that actually mutated the DB — not
per-entity (would thrash) and not at all on empty / delete-of-missing batches.

### The callback names the conversations that changed

The callback argument is the set of affected conversation uuids, and it is
load-bearing rather than informational. A payload-free closure forces the
listener to refresh every open conversation, and that **loses data**: an open
conversation can hold history that is not in the database — a temporary
conversation never persists, and `CreateConversation()` defers the first write
until there is content — while `GetConversationData()` returns a non-null
archive with **empty** `entries` for a conversation it has no rows for.
Refreshing an untouched conversation from the database therefore replaces live
history with nothing and kills any in-flight response.

The bridge resolves the uuid per record kind:

| Change                         | Source of the conversation uuid                                                                         |
| ------------------------------ | ------------------------------------------------------------------------------------------------------- |
| `c:<uuid>` upsert or tombstone | the storage key                                                                                         |
| `e:<uuid>` upsert              | `specifics.entry().conversation_uuid()`                                                                 |
| `e:<uuid>` tombstone           | `AIChatDatabase::GetConversationUuidForEntry()`, **before** the delete, while the row is still readable |

That same `GetConversationUuidForEntry()` is what `GetDataForCommit` uses to
find an entry's parent; it replaced a scan over every conversation's full
archive.

On the UI thread, `AIChatService::OnRemoteSyncDataApplied`:

1. Returns early if `ai_chat_db_` is null — the hop from the bridge sequence can
   land after storage was turned off and the database reset.
2. For each named conversation that currently has a `ConversationHandler`,
   async-fetches a fresh `ConversationArchive` and calls
   `handler->OnRemoteSyncDataApplied(archive)`. The handler:
   - Stops any in-flight LLM request (`is_request_in_progress_ = false`,
     `engine_->ClearAllQueries()`, `OnAPIRequestInProgressChanged()`).
   - Stops any tool-use loop (`StopTask()`).
   - Replaces `chat_history_` and reloads associated content from the archive.
     The `LoadArchivedContent()` call is unconditional: it is also the
     _clearing_ path (it drops owned content before re-adding from the archive),
     so skipping it for an empty archive would keep content the remote side
     detached.
   - Calls `OnHistoryUpdate(nullptr)` so the WebUI re-fetches.
3. Calls `ReloadConversations()` so the sidebar reflects conversations that
   newly appeared or got deleted. That is async and broadcasts
   `OnConversationListChanged()` itself once the read lands, so the caller must
   **not** also broadcast — doing so sends observers the pre-sync list, which
   omits exactly the conversations that just arrived.

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
  auto entry = SpecificsToEntry(merged, ac_list, &ac_texts);
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
   the entry and all of its child event/file rows, reusing the existing local
   insert path. Every per-entry table must be listed in
   `DeleteConversationEntry`; a table missing from it survives the delete and
   its rows then collide with the re-insert on
   `PRIMARY KEY(conversation_entry_uuid, event_order)`. The insert ignores that
   failure, so the stale row is read back instead — a silent wrong-data bug.
   `kEntryEventTables` is the single list both `DeleteEntryAndOwnedRows` and the
   migration iterate, which is what keeps it from drifting. **Add new per-entry
   tables there as well as to `AddConversationEntry`.** The same delete walks
   the entry's revisions, which `ApplyRemoteEntry` relies on — see "Edit history
   on apply".
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
    optional int64 created_time_windows_epoch_micros = 3;
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
    falls through to lower-priority categories while sparing the completion, and
    refuses an entry oversized by a plain (un-omittable) field.
  - `ForEachOmittableString` reaches every omittable category, pinning the field
    set the policy can drop and the restore side has to handle.
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
  - `on_remote_changes_applied` fires once per batch that actually changed the
    DB, never on empty / no-op batches, and names exactly the conversations the
    batch wrote — including resolving an entry tombstone's parent conversation.
  - `GetDataForCommit` untracks an entry that a plain, un-omittable field has
    pushed over the budget, rather than silently omitting its key.
- **`AIChatDatabaseSyncTest`** (`ai_chat_database_unittest.cc`)
  - Sync metadata read/write/clear against `ai_chat_sync_metadata`.
  - `ApplyRemoteConversationMetadata` insert, upsert-preserves-entries, and
    fill-in of a stub row left by an earlier entry record.
  - `ApplyRemoteEntry` stub-conversation creation, same-uuid replace,
    keeps-existing-metadata, and associated-content persistence.
  - `ApplyRemoteEntryReplacesChildRows` re-applies a trimmed revision of an
    entry carrying uploaded files plus one event of every kind that lands in its
    own table, pinning the full-replace contract against orphaned child rows.
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
  - `RemoteSyncDataRefreshesOnlyNamedConversations`: a remote batch empties the
    history of the conversation it names and leaves an unnamed open conversation
    untouched. Both are temporary (so neither persists), which is what makes the
    negative half observable — a single loop over every open handler would have
    emptied them in the same pass.
  - `SyncControllerDelegateExistsWithStorageOff`:
    `CreateSyncControllerDelegate()` returns non-null with chat history off, so
    the type still gets registered.
  - `SyncControllerStopsAndClearsWithStorageOff`: with no database attached, the
    controller reports `kMustStopAndClearData`, survives
    `Stop(CLEAR_METADATA, …)` through the real proxy delegates, and then flips
    to `kPreconditionsMet` off the database-ready signal once storage is
    enabled.
- **`AIChatSyncBrowserTest`** (`browser/sync/ai_chat_sync_browsertest.cc`)
  - End-to-end registration: with `BraveSyncAIChat` enabled and storage on,
    `GetUserSettings()->GetRegisteredSelectableTypes()` contains `kAIChat`.
  - `PRE_DataTypeIsRegisteredWithStorageOffAtStartup` /
    `DataTypeIsRegisteredWithStorageOffAtStartup`: the `PRE_` test turns chat
    history off, the browser restarts with that pref already false, and the type
    is still registered. This is the regression test for registration being
    gated on the feature rather than the pref; it needs no account and no fake
    server, because registration does not require sync to be on.
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
   nothing asserts that a record arriving off the wire lands correctly on a
   populated conversation — only `AIChatDatabaseSyncTest` covers that, and it
   calls the DB methods directly rather than going through the bridge.
2. **Bridge unit tests use a clean schema only.** Every test builds a fresh
   `AIChatDatabase` via `CreateSchema()`, which always produces the
   current-version schema with every column present. They never run against the
   artifact of a migration chain, so a "migration ran but didn't actually add
   the column" failure mode wouldn't be caught here.
3. **The stop-in-flight half of the active-handler refresh is not asserted.**
   Bridge tests verify that `on_remote_changes_applied` fires with the right
   conversations, and `RemoteSyncDataRefreshesOnlyNamedConversations` verifies
   that the service reloads exactly those handlers. Nothing asserts that a
   refresh arriving mid-generation actually stops the engine request and the
   tool-use loop — which is the one branch in
   `ConversationHandler::OnRemoteSyncDataApplied` still uncovered.
4. **Async-init race is covered by inspection, not by test.** The
   `AIChatSyncBackend` design specifically targets the case where the sync
   engine asks for a controller delegate before the bridge exists. No test
   simulates that ordering; the browser test incidentally exercises it but
   doesn't pin the ordering invariant.
5. **No coverage for closure marshaling.** Bridge tests use `base::DoNothing()`
   or a direct counter as the closure. The `BindPostTask` wrapping happens only
   in `AIChatService` and is not exercised.
6. **The `kMustStopAndClearData` path is covered at the controller boundary, not
   through a real configuration.** `SyncControllerStopsAndClearsWithStorageOff`
   calls `controller.Stop(CLEAR_METADATA, …)` directly on a `NOT_RUNNING`
   controller wired to the real proxy delegates, which is the part that is
   Brave's wiring. What is _not_ covered is `DataTypeManager` reaching that call
   itself during a live configuration, then restarting the type when the
   precondition flips — that needs the fake-server harness in #1 below.

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
   the whole entry. A future iteration could union-merge `events` (whose order
   is carried by their position in the repeated field) so streaming tool-use
   updates from two clients don't clobber each other. Realistically only matters
   if two clients are actively driving the same conversation simultaneously.
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
7. **Full edit-history sync.** Only the latest revision of an edited turn goes
   over the wire (see "Edit history on apply"), so a receiving device shows the
   current text without the history behind it. Local revisions are preserved on
   apply, so this is a fidelity gap rather than data loss. Syncing the history
   itself needs a stable identity per revision in the proto.
8. **Quota awareness.** The server enforces a 50,000-object per-client quota
   (`maxClientObjectQuota`). A heavy user with 1,000 conversations × 50 turns =
   51,000 objects. If telemetry shows users approaching it, we may need
   lifecycle policies (archive cold conversations, sync only last N).
9. **Conversation ownership / soft locking.** Under consideration rather than
   designed. Appending to a conversation from a device that did not originate it
   is where the awkward shapes cluster: entries interleave by unsynchronised
   wall clocks (see "Conflict resolution"), and concurrent edits to one entry
   are resolved by squashing the loser. Treating a conversation as owned by its
   originating device — read-only elsewhere, or requiring an explicit takeover —
   would sidestep both, and would also give a natural point to surface
   over-budget fields that could not be restored locally. Cost is the obvious
   one: a conversation becomes awkward to continue on a second device, which is
   much of why sync is wanted in the first place. Worth deciding against the
   union-merge option in item 1 rather than in isolation.
