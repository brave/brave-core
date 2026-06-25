# AI Chat Sync

AI Chat conversations sync between a user's devices through Chromium Sync, as
one data type, `AI_CHAT_CONVERSATION` (EntitySpecifics field `10000001`). It is
behind `features::IsBraveSyncAIChatEnabled()` (`kBraveSyncAIChat`, off by
default).

## Terms

- **Record** (entity): one item that syncs on its own: a conversation's metadata
  or one turn.
- **Specifics**: a record's content, here an `AIChatConversationSpecifics`
  proto.
- **Storage key** and **client tag**: the record's local and server-side ids.
  Both are `c:<uuid>` for a conversation's metadata and `e:<uuid>` for a turn.
- **Bridge** (`AIChatSyncBridge`): translates between the AI Chat database and
  sync records.
- **Processor** (`ClientTagBasedDataTypeProcessor`): Chromium's code that talks
  to the sync engine and calls the bridge. It remembers the last server version
  of each record it has seen.
- **Tracking**: the processor is tracking while it keeps sync metadata for AI
  Chat, from its first sync until AI Chat sync is turned off. While it isn't,
  local changes aren't queued for upload.
- **Sync metadata**: the processor's bookkeeping, meaning each record's version
  plus the progress marker. Saved in the `ai_chat_sync_metadata` table and the
  database's meta table.
- **Progress marker**: "everything from the server up to here has been
  received". On start, sync asks the server only for changes after it.
- **Commit**: uploading a local change to the server. A **pending change** is a
  local change not yet committed.
- **Batch**: the AI Chat changes downloaded in one sync cycle, handed to the
  bridge in one call to `ApplyIncrementalSyncChanges`.
- **First sync** (merge): with no sync metadata, the processor downloads every
  AI Chat record in the account and hands them to `MergeFullSyncData`, which
  applies them and uploads the local records the server lacks.
- **Replay**: a first sync over data this device already has. Unchecking AI Chat
  in sync settings clears the sync metadata but keeps the conversations, so
  re-checking it replays every server record over them.
- **Tombstone**: a record that says "this was deleted".
- **`ModelError`**: how the bridge reports a failure. Sync then stops AI Chat
  sync for the rest of the browser session; other data types carry on.
- **Stub**: a hidden conversation row, created when a turn arrives before its
  conversation's metadata record.
- **Omitted value**: a value the sender left out to fit the size limit, sending
  its hash in its place.

## Overview

A conversation is split into two kinds of record:

1. **Conversation metadata**, one per conversation: title, model and token
   counts. Always small.
2. **Entry**, one per turn: the message, its events (completion text, web
   sources, tool calls), the pages it referenced (including their extracted
   text), and the files the user attached (with their bytes when they fit).

Three constraints shape the design:

- **The sync server rejects any record over 400 KB** (see "Size-budget policy").
  One record per conversation would often exceed that, so each turn gets its own
  record and its own budget.
- **Some values are routinely big** (page text, tool output, file bytes), so
  even one turn can exceed the budget. Long strings are gzipped when that helps,
  and a fixed priority order omits values until the turn fits. An omitted value
  carries a hash, so a receiver that already holds the value can put it back.
- **A device can hold changes the server never received**, for example ones made
  while AI Chat sync was off. Every record carries the time its synced content
  last changed, and when two copies of a record differ, the copy that changed
  last wins.

## Records

```
Conversation (c:<conv-uuid>)
   ├── title, model_key, total_tokens, trimmed_tokens, modified_time
   │
   └── (implicit parent of, via conversation_uuid backlink)
       ├── Entry (e:<entry-1-uuid>)
       │     ├── entry_text, prompt, character_type, action_type, modified_time, ...
       │     ├── events[ completion | search_queries | web_sources | inline_search | tool_use ]
       │     ├── associated_content[ uuid, url, title, content_type, last_contents ]
       │     └── uploaded_files[ filename, filesize, type, data, extracted_text ]
       ├── Entry (e:<entry-2-uuid>) ...
       └── ...
```

An entry names its conversation by `conversation_uuid`. There is no foreign key;
the relationship exists only at the sync level. Associated content and uploaded
files belong to a turn, not to the conversation, so they travel with the entry
that referenced them.

**Why one data type with two kinds.** One `DataType` value, one
`UserSelectableType`, one server-side type ID. The user-facing toggle ("Sync Leo
AI") is a single switch, and there is no case for syncing metadata without
turns. Both kinds go through the same `MergeFullSyncData` /
`ApplyIncrementalSyncChanges` path, and the bridge routes each record by its
`oneof kind`.

### Storage keys and client tags

```
storage_key = client_tag = "c:" + conversation_uuid   for Conversation records
storage_key = client_tag = "e:" + entry_uuid          for Entry records
```

`GetStorageKey()` and `GetClientTag()` both use `GetStorageKeyFromSpecifics()`.
The prefixes are disjoint, so a tombstone, which carries only its key, is routed
by prefix (`base::RemovePrefix`). `IsEntityDataValid()` rejects a record with an
empty uuid, or an entry with no `conversation_uuid`.

### What doesn't sync

- **Thread turns.** `GetConversationData()` reads only turns with no
  `thread_uuid`, and every upload path builds turns from it, so a `Put` for a
  thread turn finds nothing and is dropped.
- **Workspace associated content**, which is device-local. The sender leaves it
  out, a record carrying it is rejected, and an apply keeps the local rows.
- **Edit history**, beyond the latest revision (see "Why edit history doesn't
  sync").
- **Runtime-only content blocks.** Of `mojom::ContentBlock`'s 16 variants, only
  those the local store persists in `tool_use_serialized`
  (`proto_conversion.cc`) sync: `ImageContentBlock`, `TextContentBlock` and
  `WebSourcesContentBlock`. A tool use event whose output holds a block this
  client doesn't know makes the whole entry fail to decode, so it's skipped.
- **Temporary conversations.**

## Prior art in Chromium

**Polymorphic specifics.** `SAVED_TAB_GROUP` has the same shape:
`SavedTabGroupSpecifics` uses
`oneof entity { SavedTabGroup group; SavedTabGroupTab tab; }`, and its proto
says tabs are separate entities "due to size limitations of sync entities". The
child (`Tab`) names its parent (`group_guid`), as `Entry` names its
conversation. Unlike `SavedTabGroupSyncBridge`, our storage keys carry a `c:` /
`e:` prefix, so the kind is clear from the key alone, which is all a tombstone
carries.

**Compression.** The sync engine gzips the whole HTTP request
(`components/sync/engine/net/http_bridge.cc` sets `Content-Encoding: gzip` and
calls `compression::GzipCompress`), which doesn't help the per-record limit (see
"Size-budget policy"). No upstream type compresses inside its specifics, so AI
Chat does it per field, in `AIChatCompressibleString`.

**Omission.** `deletion_origin.proto`'s `*_possibly_truncated` fields tell a
receiver that a value may be incomplete, and `bookmark_specifics.proto` drops an
oversized favicon URL outright. Both lose the value on the receiver. An omitted
AI Chat value carries a hash of what was dropped, so a receiver that holds the
same value restores it.

**Modification times.** Types that must keep a device's newer copy over the
server's put a time in the specifics and compare it: passwords (`date_created`,
`PasswordSyncBridge`), reading list (`update_time_us` and
`update_title_time_us`), and saved tab groups
(`update_time_windows_epoch_micros`, also used in
`SavedTabGroupSyncBridge::ResolveConflict`).

## Compression and size strategy

### `AIChatCompressibleString`

```protobuf
message AIChatCompressibleString {
  oneof value {
    string raw = 1;
    bytes gzipped = 2;                 // RFC 1952 gzip of the UTF-8 bytes
    fixed32 omitted_content_hash = 3;  // base::PersistentHash of the UTF-8 bytes
  }
}
```

`WriteCompressibleString(value, out)` writes `raw` for a value shorter than
`kSyncCompressionThresholdBytes` (256 bytes), and otherwise `gzipped` when the
result is smaller, falling back to `raw`. `OmitCompressibleString(out)` replaces
the value with a hash of its plaintext, decompressing first if needed.
`ReadCompressibleString(in)` returns `std::nullopt` for an omitted value, an
unset field, failed decompression, or a gzipped value that would decompress to
more than `kSyncCompressionMaxDecompressedBytes` (100 × the record budget) or
the available memory. Otherwise it returns the decoded string, possibly empty.
They live in
`components/ai_chat/core/browser/sync/ai_chat_sync_conversions.{h,cc}`.

Wrapped fields, meaning the fields the local store fills with long generated or
fetched text:

- `AIChatAssociatedContentProto.last_contents`: extracted page text
- `AIChatEntryEventProto.completion`: the assistant's reply
- `AIChatWebSource.page_content`: a fetched page snippet
- `AIChatWebSourcesEvent.rich_results` and
  `AIChatWebSourcesContentBlock.rich_results` (repeated): search result JSON
- `AIChatInlineSearchEvent.results_json`
- `AIChatToolUseEvent.arguments_json`
- `AIChatTextContentBlock.text`: text in tool output
- `AIChatToolArtifact.content_json`
- `AIChatUploadedFile.extracted_text`

`entry_text`, `prompt` and `selected_text` can be long too, but stay plain
strings, so nothing can shrink them. Short fields (title, url, uuids, counters)
stay plain. `AIChatUploadedFile.data` is plain `bytes`, since image and PDF data
is already compressed, in a `oneof` with `omitted_data_hash`.

### Size-budget policy

The 400 KB limit comes from DynamoDB, where the Brave sync server stores each
record as one item. It applies to the whole item, uncompressed, so the sync
engine's transport gzip doesn't help.

`FitEntryWithinSyncBudget(Entry*)` runs after an entry is built and before it is
committed. It does nothing when the record already fits `kSyncMaxRecordBytes`
(256 KB). The cap is measured on the plaintext, but the type is encrypted: the
server receives `Nigori::Encrypt()` output (IV, AES-CBC ciphertext and HMAC),
base64-encoded, so a record at the cap commits at about `4/3 * (size + 64)`
bytes, near 341 KB, leaving room for the rest of the `SyncEntity` under 400 KB.

The order weighs a value's worth as context for continuing the conversation
against its cost. File bytes go first, largest first, stopping as soon as the
entry fits: without them the receiver can't open the attachment, but one file
can take most of the budget, and its name, size, type and extracted text still
travel. Then the nine `kOmissionPasses`, each with a comment giving its reason:

1. `associated_content[].last_contents`: page text. The replies already reflect
   it, and the receiver has the URL.
2. `uploaded_files[].extracted_text`: the same reasoning.
3. `inline_search.results_json`: never reaches the model.
4. `web_sources[].page_content`, at event level and in tool output.
5. `tool_use.output[*].text_content_block.text`.
6. `tool_use.arguments_json`: small, and the model can produce them again.
7. `web_sources[].rich_results`, at event level and in tool output: only used to
   re-render results.
8. `tool_use.artifacts[].content_json`: never reaches the model, but the user
   opens these.
9. `completion`: the conversation itself, so last.

It re-measures between passes and stops once the entry fits. It skips a value a
pass already omitted, and an explicitly empty raw string, which reads as a value
rather than an omission and is smaller than a hash.

`ForEachOmittableString` walks exactly the fields in `kOmissionPasses`, and the
receiver uses the same walk to restore them, so the two can't drift. A
compressible field missing from `kOmissionPasses` can never be omitted, and one
large value in it would make the turn unsendable.

### Turns too large to send

A turn that still doesn't fit isn't sent, for example one with a 400 KB
`entry_text` or `selected_text`. `FitEntryWithinSyncBudget` returns false, and:

- `GetDataForCommit()` untracks the entry (`UntrackEntityForStorageKey()`) and
  clears its saved metadata (`ClearEntityMetadata()`), as `HistorySyncBridge`
  does. Leaving the key out of the batch instead counts as a bridge bug
  (`Sync.DataTypeOrphanMetadata.GetData`), and untracking alone leaves the saved
  row behind.
- `PutEntry()` skips the `Put` and leaves the entry tracked. Untracking an entry
  committed earlier would turn a later local delete into a no-op and leave the
  server's copy behind.
- A first sync leaves it out of the upload, and so does the upload of a local
  copy kept because it's newer (see "Which copy wins").

Other devices keep the last version that fit. A replay on this device doesn't
apply that older version over the local one, because the local copy changed
later.

## Which copy wins: the modification time

### Why the server version isn't enough

The processor orders changes by server version, and in steady state that's
enough: it drops a version it already has, sees a conflict only when this device
has a pending change, and ignores a remote update identical to its own last
commit (`ClientTagBasedRemoteUpdateHandler`).

But a first sync has no versions to compare. It applies every server record over
the local data and uploads only the records the server lacks. That's lossless
only when the server already has this device's latest version, and a local
change misses the server in two ways:

- **It was made while AI Chat sync was off.** The bridge's local-change hooks
  return early while the processor isn't tracking.
- **The turn grew too large to send** after an earlier version was uploaded.

Without a time to compare, the replay would revert a rename, bring back cleared
page text, or add the server's older text as the newest revision of an edited
turn, hiding the edit.

### What the time records

`modified_time_windows_epoch_micros` (field 6 of `Conversation`, field 16 of
`Entry`) is when the record's synced content last changed, on whichever device
changed it. Unset means unknown, which counts as the oldest. Locally it's a
`modified_time` column on `conversation` and `conversation_entry` (NULL when
unknown, including every row from before the column existed), read with
`AIChatDatabase::GetConversationModifiedTime()` / `GetEntryModifiedTime()`.

- **Local writes move it to now:** `AddConversation`, `UpdateConversationTitle`,
  `AddConversationEntry`, `UpdateToolUseEvent` (when it matched a row), and
  `DeleteAssociatedWebContent` (for each turn it clears).
- **Only a real change moves it** for `UpdateConversationModelKey`
  (`WHERE ... AND model_key IS NOT ?`) and `AddOrUpdateAssociatedContent` (an
  insert, or a synced field that differs, which `IsAssociatedContentChanged`
  checks by decrypting the stored row). `AIChatService::HandleNewEntry` saves
  the model key on every new turn, and the conversation's associated content
  whenever it has some, mostly unchanged. Otherwise a device that only kept
  chatting would revert another device's rename, or its edit of an earlier turn.
- **Token counts never move it**, since they change on every reply. Token counts
  from another device are written only with a change that moves the
  conversation's time (a rename or a model change); otherwise they update with
  this device's next reply.
- **An edit moves the time of the turn it edits.** Sync sends an edited turn as
  one record under the original turn's uuid, so `SetEntryModifiedTime()`
  resolves an edit row to its original (`COALESCE` on `editing_entry_uuid`).
  Edit rows themselves stay NULL.
- **An applied record keeps the sender's time.**
  `ApplyRemoteConversationMetadata` and `ApplyRemoteEntry` store the record's
  time, not the arrival time. Otherwise this device's copy would look newer than
  the server's at the next first sync and be uploaded again for nothing.

The time is passed alongside the mojom structs
(`ConversationMetadataToSpecifics(conversation, modified_time)`,
`EntryToSpecifics(..., modified_time, ...)`), not added to them: the UI never
uses it.

### The comparison

`FindNewerCopy()` compares a remote record's time with this device's copy:

| This device's copy                                                   | Result                                                         |
| -------------------------------------------------------------------- | -------------------------------------------------------------- |
| None, or a stub conversation                                         | The remote record is written.                                  |
| Older than the remote record, or unknown while the remote has a time | The remote record is written.                                  |
| Same time                                                            | Same version: nothing is written.                              |
| Both unknown                                                         | Same version: nothing is written.                              |
| Newer than the remote record, or the remote has no time              | The local copy is kept and uploaded, so the server catches up. |

- **Equal times write nothing.** A replay of this device's own records is then a
  no-op, and the batch doesn't name those conversations, so open ones aren't
  reloaded.
- **Both unknown is the same version.** Every change the comparison covers moves
  the time, so two copies with no time are both the record from before records
  carried one. Treating them as "apply" would rewrite them on every replay.
- **A stub counts as no local copy.** The getters report `exists` false for a
  stub, so a metadata record always fills it in.
- **A failed read fails the batch.** The getters return `std::nullopt` for a
  failed read, distinct from a missing row or a NULL time; taking it for "no
  local copy" would write the remote record over a copy that may be newer.
  Upload paths send no time on a failed read (`TimeToSend()`), so that copy
  can't overwrite another device's.
- Clock skew between devices is accepted, as in the Chromium types that do the
  same.

### Where it's applied

- **`MergeFullSyncData`.** A kept local copy is left out of the set of keys the
  server has, so the first sync's existing upload pass (`ForEachLocalEntity()`)
  sends it.
- **`ApplyIncrementalSyncChanges`**, for a remote update when this device has no
  pending change. Kept copies are collected and sent by `PutKeptLocalCopies()`,
  posted to run after the apply returns, because the processor counts a `Put`
  made during a remote update (`Sync.LocalChangeDuringRemoteUpdate`), which
  upstream means to make a CHECK. The task sends only copies still stored with
  the time they were kept with: a local change since sends itself, and a newer
  remote record needn't go back. It also skips keys guarded after a failed apply
  (see "Local updates after a failed apply"). The upload isn't atomic with the
  batch's metadata save, so a crash in between leaves the copy unsent until it
  changes again. Normal shutdown runs the task, because the database's task
  runner is `BLOCK_SHUTDOWN`.
- **`ResolveConflict`**, for a remote update when this device has a pending
  change. Its answer must agree with the apply: `kUseRemote` drops the pending
  change and hands the record to `ApplyIncrementalSyncChanges`, which would then
  keep a newer local copy or, for the same version, write nothing, and the local
  change (such as new token counts) would never be sent. So it returns
  `kUseRemote` only when the remote copy is newer, and `kUseLocal` otherwise. A
  remote tombstone carries no time and keeps the default, `kUseLocal`. A failed
  read returns `kUseRemote`, so the apply reads again and fails the batch if it
  can't.

With `ResolveConflict` comparing times, a concurrent edit of the same record on
two devices goes to the later edit by wall clock, not to whichever reached the
server first. Turn display order is a separate matter: turns are ordered by
`created_time` (`ORDER BY date ASC`), the author's wall clock, so turns appended
on two devices at once interleave by unsynchronised clocks.

### Records arrive in any order

- **A turn before its conversation's metadata:** `ApplyRemoteEntry` inserts a
  stub conversation (`INSERT OR IGNORE`, `is_sync_stub = 1`).
  `GetAllConversations()` skips stubs, which keeps them out of the UI, the
  service's list and every upload path. The metadata record fills the stub in
  and clears the flag.
- **Metadata before any turns:** an empty conversation until they arrive.
- **A conversation deleted while its turns are in flight:** they form a stub,
  which stays hidden, so it can't reappear or be uploaded with an empty title.
- **A local conversation created over a stub:** `AddConversation` updates, then
  inserts only when no row matched, so it fills the stub in and lists it.

## Applying a batch

### The pipeline

`ApplyRemoteChanges()` first reads, once for a non-empty batch, which entries
have edit revisions (`GetEditedEntryUuids()`; see "How this device's revisions
are kept"). Then, for each change:

- **Tombstone** (`ApplyRemoteTombstone()`). A conversation's runs
  `DeleteConversation`. A turn's first looks up its conversation
  (`GetConversationUuidForEntry()`) while the row still exists, so the service
  can be told which conversation changed, then runs `DeleteConversationEntry`.
  Tombstones bypass `IsEntityDataValid()`, so an empty uuid is skipped here.
- **Record** (`ApplyRemoteRecord()`), in this order:
  1. Read the local copy's time and compare (see "Which copy wins"). Stop unless
     the remote copy is newer or there is no local copy.
  2. Conversation metadata: decode, then `ApplyRemoteConversationMetadata`.
  3. Entry: skip it if any of its associated content uuids belongs to another
     conversation (`GetConversationUuidForAssociatedContent()`).
  4. Read the local turn, only when one exists and the record omits a value or
     has revisions locally: `GetConversationEntryWithEdits()` when it has
     revisions, `GetConversationEntryWithoutEdits()` (by primary key) otherwise.
  5. Restore omitted values into a copy of the record, in proto space (see
     "Restoring omitted values").
  6. Decode (`SpecificsToEntry`). Skip a record that doesn't decode.
  7. Keep this device's edit history (`WithLocalEditHistory()`).
  8. Write (`ApplyRemoteEntry`), storing the record's time.

The bridge then reports which conversations the batch touched (see "Showing
remote changes").

### Sync metadata is saved only when the apply succeeds

The processor records a batch's new progress marker and each record's new
version in the metadata change list before it calls the bridge. Sync never
downloads again a change it thinks it already has, so if those were saved and
the apply then failed, the failed records would be lost after a restart.

So the bridge uses Chromium's default in-memory list (it doesn't override
`CreateMetadataChangeList()`), and `ApplyMetadataChanges()` moves the changes
into a `SyncMetadataStoreChangeList`, which writes to the database, only after
the whole batch is written, including the metadata-only calls the processor
makes after every commit. On failure the bridge drops them (`DropAllChanges()`),
returns `kAIChatFailedToApplyRemoteChange`, and the next start downloads the
batch again. Re-applying its successful records is safe: an equal time writes
nothing, and every write is a full replace or an update-then-insert. Without a
database, both calls drop the changes and succeed, so the records download again
later. An in-memory list destroyed with unsaved changes hits a CHECK
(`NotFatalUntil::M153`, so fatal now), so every path that doesn't save drops
them. Changes the bridge makes itself (local `Put`s and deletes) write through a
`SyncMetadataStoreChangeList` directly.

### Skip versus fail

- **Skip** when the record itself can't be stored: it doesn't decode (for
  example an event or tool output block this client doesn't know, as from a
  newer client, an invalid URL, or two associated content items with the same
  uuid), or its associated content uuid belongs to another conversation and
  would collide on the table's primary key. Retrying would fail the same way
  every time, so failing would stop AI Chat sync on every start. A skipped
  record still counts as received.
- **Fail** (`ModelError`) when this device can't store a good record: a failed
  read of the local time, the local turn, its page texts or the revision
  snapshot, or a failed database write.

### Local updates after a failed apply

A `ModelError` doesn't clear the versions the processor recorded in memory for
the batch. A local `Put` of one of those records would save that version with
its own metadata, so after a restart the re-downloaded change would count as
already received and be dropped, and the local copy would overwrite it on every
device. So `ShouldSkipLocalPut()` skips, for the rest of the session, local
updates to the failing record and every record after it in the batch, and every
local update after a failed first sync.

The skipped update isn't lost. The next start downloads the change again, and
the local copy, changed after it, is newer, so it's kept and sent then. Local
deletes aren't guarded: saving the tombstone at the batch's version is right,
since the delete came after the remote change.

## Restoring omitted values

A sender omits large values to fit the size limit, sending a hash of each.
Before decoding, `RestoreOmittedFieldsFromLocal()` puts back each omitted value
whose hash matches a value this device holds for the same turn:

- every omittable value of the local turn's latest revision, built into a record
  with `EntryToSpecifics` exactly as a sender builds it, since a sender sends an
  edited turn's latest revision;
- the page texts of the turn's own associated content
  (`AIChatDatabase::GetAssociatedContentTexts()`).

It runs in proto space, on a copy of the record, before `SpecificsToEntry`,
because once decoded an omitted value and an empty one look the same. Matching
is by hash rather than position, because the two copies can differ and most
values sit in events and lists with no identity to match by. Restoring runs only
for a record that is newer than the local copy, has a local row and omits
something. A failed read of the local turn or its page texts fails the batch.

**A value that isn't restored is lost on this device.** Several of these mojom
fields can't be absent (`completion`, tool `arguments_json`, tool output text,
artifact `content_json`, inline search `results_json`), so they read through
`ReadCompressibleStringOrEmpty()` as blank, and its comment says the restore
must come first. An unrestored `rich_results` item is dropped from its list.
Associated content whose `last_contents` is absent or still omitted is written
with blank text. A hash misses when this device's value has changed since, or
exists only in an earlier revision; the write then blanks it, and on an edited
turn the blank counts as a new revision. The TODO in `ApplyRemoteRecord()` links
https://github.com/brave/brave-browser/issues/53978.

## Edit history

Locally, an edited turn is the original row, which keeps the **original** text,
plus one revision row per edit (`editing_entry_uuid` → the original), ordered by
`created_time`, latest last. A revision is a full `conversation_entry` row, with
its own events and files. The UI shows only the latest revision (`edits?.at(-1)`
in `conversation_entries/index.tsx`).

### Why edit history doesn't sync

`EntryToSpecifics` sends only the latest revision's content, under the original
turn's identity (uuid and `created_time`), so the record stays stable across
edits and keeps its place in the conversation. Another device shows the current
text with no history. That's a deliberate fidelity gap, for two reasons:

- **The recursion is too complicated to carry over sync for now.** Each revision
  is a turn in its own right, which `AddConversationEntry` saves by calling
  itself and `DeleteEntryAndOwnedRows` walks, and edits are likely to move to
  Threads, which would change how they're stored. Syncing history waits for
  that.
- **It would need a stable identity per revision in the proto.** Each device
  gives a revision a random uuid, so two devices' copies of one revision
  couldn't be matched. The proto's `editing_entry_uuid` (field 7) is neither
  written nor read.

### How this device's revisions are kept

A full-replace apply of the incoming record would delete this device's
revisions, including the only copy of the original text.
`WithLocalEditHistory()` prevents that: when the local turn has revisions, it
keeps the original and its revisions, and adds the incoming content as a new
latest revision, unless it already matches the latest.

- **Read once per batch.** Nothing is indexed on `editing_entry_uuid`, so asking
  per record would scan the table for every record of a first sync. The batch
  reads the set of edited turns once (`GetEditedEntryUuids()`), and only a turn
  in it is read with its revisions. A failed snapshot read fails the batch,
  since "no revisions" would let the writes delete them. A turn a tombstone
  deleted earlier in the same batch has no local row, so it isn't read at all,
  and the record is written as it is.
- **Compared after a sync round trip.** `HasLatestRevisionContent()` builds the
  local turn into a record and decodes it again, then compares that with the
  decoded remote turn, so both hold only what sync carries: a field sync doesn't
  carry, such as a tool artifact's id, can't add a revision. Decoded mojom is
  compared, not protos, because gzip output can differ for the same text.
- **Dated to sort last.** The new revision gets a fresh uuid and
  `max(record's modified time, latest local revision + 1µs)` as its
  `created_time`. The record carries the original turn's `created_time`, so
  without this it could sort before an existing revision, and the next record
  would be compared with the wrong one and appended again on every batch. An
  untimed record reaches this only with no local row, since "Which copy wins"
  keeps a timed local copy over it.
- **The database stays a full-replace writer.** `ApplyRemoteEntry` hands the
  revisions to `AddConversationEntry`, and its delete
  (`DeleteEntryAndOwnedRows()`) removes each revision's own event and file rows,
  walking revisions iteratively with a visited set: uuids come from the wire, so
  an `editing_entry_uuid` cycle is possible and recursion would overflow.

**Known gaps:**

- A revision that isn't the latest is kept but the UI can't show it.
- An omitted value with no local match adds a blank revision (see "Restoring
  omitted values").
- `GetConversationEntryWithEdits()` reports only a failed read of the turn rows.
  A failed read of their events or files, or a failed decryption, reads as
  blank, and the apply writes that blank back over this device's own revisions.

## Outbound changes

`AIChatService` posts each local change to the bridge through the backend:

| Local change                        | Records sent                                                                                                                                                       |
| ----------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| First turn of a new conversation    | Conversation `Put` + Entry `Put`                                                                                                                                   |
| Later turn                          | Entry `Put` + Conversation `Put` (the model may have changed)                                                                                                      |
| Tool use output on an existing turn | Entry `Put`                                                                                                                                                        |
| Edit of an assistant turn           | The turn is removed and added again with its revisions: Entry `Delete`, then Entry `Put` of the same key + Conversation `Put`                                      |
| Edit of a human turn                | Entry `Delete` for the turn and every later one, then the turn is submitted again with its revisions: Entry `Put` + Conversation `Put`, then the new reply's turns |
| Regenerating an answer              | Entry `Delete` for the question and every later turn, then the question is submitted again: Entry `Put` + Conversation `Put`, then the new answer                  |
| Turn removed                        | Entry `Delete`                                                                                                                                                     |
| Rename                              | Conversation `Put`                                                                                                                                                 |
| Token counts                        | None of their own; they go with the next metadata `Put`                                                                                                            |
| Browsing data cleared               | Entry `Put` for each turn whose associated content was cleared                                                                                                     |
| Conversation deleted                | Entry `Delete` per turn, then Conversation `Delete`, sent before the database delete so the turns can still be listed                                              |
| Every conversation deleted          | Entry and Conversation `Delete` for every tracked record (see "Delete all")                                                                                        |

Each Entry `Put` rebuilds the record from the database
(`BuildCommittableEntry()`), including page text, and fits it within the budget
(see "Turns too large to send"). `GetDataForCommit()` does the same when the
processor has no cached copy, finding the turn's conversation with
`GetConversationUuidForEntry()`. A first sync uploads the local records the
server lacks.

### Delete all

Deleting every conversation (`AIChatService::DeleteConversations()` with no time
range) goes through `AIChatSyncBackend::DeleteAllData()` when sync is enabled.
While the processor is tracking, `AIChatSyncBridge::DeleteAllConversations()`
sends a `Delete()` for every tracked storage key, then
`AIChatDatabase::DeleteAllDataExceptSyncMetadata()` deletes every conversation
and saves those deletions, which wait in the sync metadata until they're sent,
in one transaction. Wiping the whole database would also wipe the sync metadata,
so the deletions would never be sent, and the next start's first sync would
bring every conversation back. If the transaction fails, neither is saved and
the bridge reports `kAIChatFailedToDeleteAllData`, since the processor already
counts the deletions as made. Without tracking, the database is wiped. Nothing
waits for the result, so a failure is only logged.

## Bridge lifecycle and threading

The bridge lives on `db_task_runner_` with the database, holds a raw pointer to
`AIChatDatabase`, and calls it synchronously. The UI thread reaches it in two
ways:

- **Sync engine calls**, through two `ProxyDataTypeControllerDelegate`s. The
  controller's posts to the UI sequence (see "Looking up `AIChatService`
  lazily") and forwards to the one from
  `AIChatService::CreateSyncControllerDelegate()`, which posts to
  `db_task_runner_` and resolves to the processor's own delegate
  (`AIChatSyncBackend::GetControllerDelegate()` returns
  `change_processor()->GetControllerDelegate()`). The processor then calls the
  bridge.
- **Local-change notifications** from the service, posted to `db_task_runner_`.

### `AIChatSyncBackend`

The service holds a refcounted `AIChatSyncBackend`
(`sync/ai_chat_sync_backend.{h,cc}`) that owns the bridge and is only used on
`db_task_runner_`. Local-change notifications are bound to the backend rather
than a `WeakPtr` to the bridge, so the posted task keeps it alive across the
hop.

`MaybeInitStorage()` creates `db_task_runner_` and, when AI Chat sync is
enabled, `sync_backend_`, synchronously and on the feature flag alone, whatever
the storage pref says. It posts the bridge's creation (`SetBridge()`) at once,
before the database exists, so whenever AI Chat sync is enabled,
`CreateSyncControllerDelegate()` returns a delegate, even with storage off.
**Keep installing the bridge eagerly:** the backend returns a null delegate
while it has no bridge, and the proxy then drops `OnSyncStarting()`, leaving the
type stuck in `MODEL_STARTING`.

The database depends on an `os_crypt_async::Encryptor`, which arrives
asynchronously. `OnOsCryptAsyncReady()` posts `SetDatabase()` to the backend.
The first attach loads the sync metadata and calls `ModelReadyToSync()`; if the
load fails, it reports `kAIChatFailedToLoadMetadata` and loads again on the next
attach. `os_crypt_init_pending_` stops a second encryptor request while one is
outstanding.

Turning storage off posts `ClearDatabase()`, which nulls the bridge's database
pointer, then wipes the database (`AIChatDatabase::DeleteAllData()`), sync
metadata included. The backend, bridge and processor stay alive across the
toggle so the delegates sync holds keep resolving. `ClearDatabase()` doesn't
reset the bridge's `model_ready_to_sync_`: the processor stays model ready
across a stop (its `OnSyncStopping()` CHECKs that), so calling
`ModelReadyToSync()` again on the next attach would crash.

When sync stops the type and clears its metadata, as when the user unchecks AI
Chat, the processor calls `ApplyDisableSyncChanges()`. The bridge's override
clears the sync metadata table and the data type state but keeps the
conversations, which is why the next first sync is a replay. With no database,
it does nothing.

## Data type controller

`AIChatDataTypeController`
(`components/ai_chat/core/browser/sync/ai_chat_data_type_controller.{h,cc}`) is
a `syncer::DataTypeController`. Sync asks a type's controller whether the type
can run (its precondition), and starts and stops the type through the
controller's delegates.

### Registration

`SyncServiceFactory` builds the list of controllers in `CreateControllers()`. A
one-line patch adds `BRAVE_CREATE_CONTROLLERS` there, and the `chromium_src`
override defines it as
`base::Extend(controllers, CreateBraveControllers(profile, sync_service));`,
forward-declaring `CreateBraveControllers()` so the override needs only
upstream's `data_type_controller.h`. It's implemented once per platform:

- desktop and Android: `browser/sync/brave_sync_controllers.cc`
  (`//brave/browser/sync:chromium_impl`), for
  `chrome/browser/sync/sync_service_factory.cc`;
- iOS: `ios/browser/sync/brave_sync_controllers.mm`, for
  `ios/chrome/browser/sync/model/sync_service_factory.mm`.

Each registers the controller when `features::IsAIChatEnabled()` and
`features::IsBraveSyncAIChatEnabled()` are both on. AI Chat's prefs, including
the storage pref the precondition reads, are registered only while AI Chat is
enabled. `BraveSyncServiceImpl` selects `kAIChat` when the device joins a sync
chain only if the type is registered, since `SetSelectedTypes()` takes only
registered types.

**Registration follows the feature flags, never the storage pref or policy.**
The set of registered types is fixed for the `SyncService`'s lifetime:
`SyncServiceImpl::Initialize()` runs once, `DataTypeManagerImpl` holds a `const`
controller map, and `SetSelectedTypes()` / `GetSelectedTypes()` keep only
registered types. A type left out at startup couldn't sync until a restart, and
the "Sync Leo AI" checkbox is hidden when the type isn't registered
(`aiChatRegistered` in `sync_controls.html.ts.lit_mangler.ts`). Upstream
`SkillDataTypeController` and `AutofillWalletDataTypeController` likewise
register on a feature and gate the precondition on a pref.

### Looking up `AIChatService` lazily

The controller is created while `SyncServiceFactory` builds the `SyncService`.
On desktop, looking up `AIChatService` then would build it, and its dependencies
(`ProfileMiscMetricsService`, then `PersonalDataManager`) would get a null
`SyncService` from the factory still building it. So the controller gets a
getter (`AIChatServiceFactory::GetForBrowserContext` bound to the profile, or
`GetForProfile` on iOS), as upstream gives `CommonControllerBuilder` a
`ContextualTasksService` getter to avoid a dependency cycle. It runs the getter
only once the `SyncService` is built:

- in a task the constructor posts (`ConnectToAIChatService()`), which gets the
  service's delegate and subscribes to the database-ready signal. That task runs
  before sync first checks the precondition, which waits for the sync engine to
  start;
- in `GetPreconditionState()`;
- from its delegates. `SyncServiceImpl::Initialize()` calls them during the
  build, to clear the sync metadata of types the user hasn't selected, so they
  are `ProxyDataTypeControllerDelegate`s that post each call to the UI sequence
  and run the getter only in that task (`GetBridgeDelegate()`).

`AIChatSyncBrowserTest.PersonalDataManagerGetsSyncService` pins this. Declaring
`DependsOn(AIChatServiceFactory)` on `SyncServiceFactory` instead aborts
startup: `AIChatServiceFactory`'s constructor reaches `SyncServiceFactory` again
through `ProfileMiscMetricsServiceFactory` → `PersonalDataManagerFactory`.

### Precondition

| State                                                                                                                                        | `GetPreconditionState()` |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------ |
| The chat history feature (`kAIChatHistory`) or the storage pref (`kBraveChatStorageEnabled`) is off                                          | `kMustStopAndClearData`  |
| A policy disables AI Chat (`IsAIChatEnabled(prefs)` is false)                                                                                | `kMustStopAndKeepData`   |
| No `AIChatService`, or it hasn't created its database yet (`IsSyncDatabaseReady()`, true once `OnOsCryptAsyncReady()` has posted the attach) | `kMustStopAndKeepData`   |
| Otherwise                                                                                                                                    | `kPreconditionsMet`      |

- **Storage off clears the type.** Detaching the database tells sync nothing, so
  across an off→on toggle the processor would still think the new, empty
  database is fully synced. With `kMustStopAndClearData`, sync stops the type
  and clears its metadata, so turning storage on runs a first sync that
  downloads every conversation. The controller repeats the history feature check
  because without that feature `AIChatService` never opens a database, though
  the pref defaults to on.
- **Off is immediate; on waits.** The controller watches the storage pref and
  calls `DataTypePreconditionChanged()` at once when it turns off. Turning it on
  waits for `AIChatService::RegisterSyncDatabaseReadyCallback()`, which
  `OnOsCryptAsyncReady()` notifies right after posting `SetDatabase()`, so on
  the database sequence the attach comes before any first sync the restart
  posts. Until then `kMustStopAndKeepData` keeps a first sync from merging into
  no database.
- **Policy is checked at runtime.** The controller also watches
  `kEnabledByPolicy`. On a change it tries to connect (on desktop the getter
  returns null while policy disables AI Chat, so enabling it is when the
  controller can first connect) and calls `DataTypePreconditionChanged()`. While
  a policy disables AI Chat the type stays registered, with a toggle that does
  nothing.

A bridge with no database never calls `ModelReadyToSync()`, which is fine: sync
doesn't start a type whose precondition isn't met, and the processor holds a
clear-metadata request made before the model is ready until it is
(`ClientTagBasedDataTypeProcessor::ClearMetadataIfStopped()`).

## Showing remote changes

The bridge writes remote changes straight to the database, so it tells the
service which conversations changed. `AIChatService::MaybeInitStorage()` builds
its `RemoteChangesAppliedCallback` with `base::BindPostTask` to the UI sequence.
`NotifyRemoteChangesApplied()` runs it once per batch that touched a
conversation, also when the batch failed part-way, since the records written
before the failure stay written. It takes two lists:

- `conversation_uuids`: each conversation a record was written to, each one a
  conversation tombstone named (even one already gone), and the conversation of
  each turn a tombstone deleted. A turn tombstone whose row is already gone, and
  a record the comparison didn't write (same or older), name none.
- `deleted_conversation_uuids`: the conversations a conversation tombstone
  deleted, which the first list also names.

| Change                         | Source of the conversation uuid                         |
| ------------------------------ | ------------------------------------------------------- |
| `c:<uuid>` record or tombstone | the storage key                                         |
| `e:<uuid>` record              | `specifics.entry().conversation_uuid()`                 |
| `e:<uuid>` tombstone           | `GetConversationUuidForEntry()`, read before the delete |

**Only the named conversations are reloaded.** An open conversation sync didn't
touch can hold turns not yet in the database (`CreateConversation()` defers the
first write until there is content, and temporary conversations never persist),
and reloading it would discard them along with any reply in progress.

`AIChatService::OnRemoteSyncDataApplied()`:

1. Returns if `ai_chat_db_` is null: storage can have been turned off since the
   batch was applied.
2. Marks each open conversation the batch deleted as not saved
   (`has_content = false`), so its next turn creates it again
   (`HandleFirstEntry`) instead of adding to a conversation that's gone.
3. Reloads each named conversation that has a `ConversationHandler`
   (`ReloadRemotelyChangedConversation()`).
4. Reloads the conversation list (`ReloadConversations()`), which notifies
   observers itself once the read completes, so the caller doesn't broadcast the
   stale list. That load also erases named conversations the database no longer
   lists, unless one is open, in which case it's marked not saved. A load only
   sync asked for leaves alone the content of open conversations the batch
   didn't name, which can hold unsaved attachments.

`ConversationHandler::OnRemoteSyncDataApplied()` then takes the archive read for
it, with the handler's `local_change_count()` from when the read started:

- a null archive (the database couldn't be read) changes nothing;
- a request or tool use loop in progress is stopped
  (`StopInProgressGeneration()`), since its reply was built on replaced history;
- if the handler saved a change since the read started, it reads again;
- otherwise it reloads associated content (`LoadArchivedContent()`, called even
  for an empty list, since it's also what drops content the archive no longer
  lists), replaces `chat_history_`, and calls `OnHistoryUpdate(nullptr)` so the
  UI fetches the history again.

## Database layer

**`ApplyRemoteConversationMetadata`** updates the row, and inserts only when the
update matched none. `UPDATE` leaves columns it doesn't name alone, so a
local-only column added later survives remote applies. Chromium builds SQLite
with `SQLITE_OMIT_UPSERT`, and `INSERT OR REPLACE` would delete and re-insert
the row, resetting every column it doesn't name. It clears `is_sync_stub` and
stores the record's time. The title write fails on an encryption error rather
than binding NULL over the stored title.

**`ApplyRemoteEntry`** runs in one transaction:

1. `INSERT OR IGNORE` a stub conversation, in case the metadata hasn't arrived.
2. `DeleteEntryAndOwnedRows(uuid, keep_workspace_content=true)`, then
   `AddConversationEntry(...)`: a full replace of the turn, its revisions and
   every event, file and associated content row they own, through the local
   insert path. Workspace associated content stays attached to the re-inserted
   uuid. Every per-turn table must be in `kEntryEventTables`, which the delete,
   a migration and `DeleteAllDataExceptSyncMetadata()` iterate. A table missing
   from it survives the delete and collides with the re-insert on its primary
   key, `(conversation_entry_uuid, event_order)` for events and
   `(conversation_entry_uuid, file_order)` for uploaded files.
3. `AddOrUpdateAssociatedContent(...)` with the caller's texts.
4. `SetEntryModifiedTime()` with the record's time, replacing the "now" the
   local paths set.

A failed child insert fails the whole apply and rolls it back.
`AddConversationEntry` skips three event shapes it can't store (web sources with
no sources, inline search with no query, tool use with no id or tool name);
since a sender builds its record from its own database, only a buggy or newer
sender can send one.

**Schema.** `AIChatDatabase::InitInternal()` runs `CreateSchema()` before the
migrations, and it creates any missing table (`CREATE TABLE IF NOT EXISTS`), so
a new table needs only a `CreateSchema()` change. A new column on an existing
table also needs a `MigrateFromXToY` helper, a bump of
`kCurrentDatabaseVersion`, and a SQL fixture of the previous version under
`components/test/data/ai_chat/` for `AIChatDatabaseMigrationTest`. Sync added:

- `ai_chat_sync_metadata(storage_key, value)`, with no version bump. It's its
  own table so tombstones outlive the data rows, as in
  `HistorySyncMetadataDatabase`. `AIChatDatabase` implements
  `syncer::SyncMetadataStore`, and the data type state lives in the meta table.
- v12 → v13: `conversation.is_sync_stub` (`INTEGER NOT NULL DEFAULT 0`). Stubs
  are marked by a flag, not a NULL title, because local conversations also start
  with a NULL title.
- v13 → v14: `modified_time INTEGER` on `conversation` and `conversation_entry`.
  Existing rows stay NULL, meaning unknown.

## Server side (go-sync)

The Brave sync server stores the specifics as opaque bytes
(`SyncEntity.Specifics` in `go-sync/datastore/sync_entity.go`), each record as
one DynamoDB item (see "Size-budget policy" for the limit). It allows each sync
chain `maxClientObjectQuota` (50,000) objects across all types. The server's
generated `ai_chat_specifics.pb.go` needs regenerating when the proto changes,
for anything that inspects it.

## Proto schema

```protobuf
message AIChatConversationSpecifics {
  message Conversation {
    optional string uuid = 1;
    optional string title = 2;
    optional string model_key = 3;
    optional uint64 total_tokens = 4;
    optional uint64 trimmed_tokens = 5;
    optional int64 modified_time_windows_epoch_micros = 6;
  }
  message Entry {
    optional string uuid = 1;
    optional string conversation_uuid = 2;  // parent backlink
    optional int64 created_time_windows_epoch_micros = 3;
    optional string entry_text = 4;
    optional string prompt = 5;
    optional int32 character_type = 6;       // HUMAN | ASSISTANT
    optional string editing_entry_uuid = 7;  // unused
    optional int32 action_type = 8;
    optional string selected_text = 9;
    optional string model_key = 10;
    repeated AIChatEntryEventProto events = 11;
    repeated AIChatAssociatedContentProto associated_content = 12;
    repeated AIChatUploadedFile uploaded_files = 13;
    optional AIChatSkillEntry skill = 14;
    optional AIChatNEARVerificationStatus near_verification_status = 15;
    optional int64 modified_time_windows_epoch_micros = 16;
  }
  oneof kind {
    Conversation conversation = 1;
    Entry entry = 2;
  }
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

The event messages follow the same convention: long strings are
`AIChatCompressibleString`. See
`components/sync/protocol/ai_chat_specifics.proto` for the full schema. A new
field also needs adding to
`chromium_src/components/sync/protocol/proto_visitors.h`.

## Testing

| Fixture                                                                       | File                                                                            |
| ----------------------------------------------------------------------------- | ------------------------------------------------------------------------------- |
| `AIChatSyncConversionsTest`                                                   | `components/ai_chat/core/browser/sync/ai_chat_sync_conversions_unittest.cc`     |
| `AIChatSyncBridgeTest`                                                        | `components/ai_chat/core/browser/sync/ai_chat_sync_bridge_unittest.cc`          |
| `AIChatDataTypeControllerTest`                                                | `components/ai_chat/core/browser/sync/ai_chat_data_type_controller_unittest.cc` |
| `AIChatDatabaseSyncTest`, `AIChatDatabaseTest`, `AIChatDatabaseMigrationTest` | `components/ai_chat/core/browser/ai_chat_database_unittest.cc`                  |
| `AIChatServiceUnitTest`                                                       | `components/ai_chat/core/browser/ai_chat_service_unittest.cc`                   |
| `BraveSyncServiceImplTest`                                                    | `components/sync/service/brave_sync_service_impl_unittest.cc`                   |
| `AIChatSyncBrowserTest`, `AIChatSyncWithoutAIChatBrowserTest`                 | `browser/sync/ai_chat_sync_browsertest.cc`                                      |

`BraveSyncServiceImplTest` runs in `brave_unit_tests`, the browser tests in
`brave_browser_tests`, and the rest in `brave_components_unittests`.

A test that applies a record through a replay must give it a newer time than the
local copy, or the comparison writes nothing and the test passes without
applying anything.

### Known testing limitations

1. **No fake-server test drives a sync session.** Brave has no `SyncTest`
   harness, so nothing runs a real configuration: injecting records, checking
   they land, or `DataTypeManager` stopping and clearing the type when storage
   turns off and restarting it when storage turns on. The controller test calls
   `Stop(CLEAR_METADATA, …)` itself.
2. **No test replays a turn with every field filled.** Nothing sends a turn too
   large to fit, with every stored field set, through the real send path and
   back through the apply path to check the result is lossless.
3. **Bridge tests use a fresh schema**, never the result of the migration chain.
4. **The `BindPostTask` wrapping isn't exercised.** Service tests call
   `OnRemoteSyncDataApplied()` directly.
5. **Stopping a tool use loop on remote apply isn't tested**; stopping a request
   is (`RemoteSyncReloadStopsReplyBeforeReadingAgain`).

## Future improvements

1. **Values a hash can't restore** (see "Restoring omitted values"). Search
   every revision, not only the latest, fall back to the local value at the same
   position, and keep an unrestorable value's hash so this device sends it on
   rather than a blank. Chromium's per-record cache of fields from newer clients
   (`TrimAllSupportedFieldsFromRemoteSpecifics()` /
   `GetPossiblyTrimmedRemoteSpecifics()`) could hold those hashes.
2. **Fields from newer clients.** `TrimAllSupportedFieldsFromRemoteSpecifics()`
   returns empty, so nothing of a record is cached. This device rebuilds each
   record it commits from its database, so an older client that commits a record
   drops any field a newer client added.
3. **Deletes made while AI Chat sync is off come back.** No tombstone is sent
   while the processor isn't tracking, and the comparison can't help because no
   local row is left. A local log of deletes, compared with each record's time
   at the next first sync, would close it.
4. **A remote conversation tombstone deletes local turns** the deleting device
   never saw, without telling the processor, so the server keeps their records.
   The tombstone path could `Delete()` each local turn.
5. **A skipped record isn't fetched again** after an upgrade that could read it,
   until it changes or AI Chat sync is reset.
6. **Edit history sync**, once edits move to Threads or revisions get a stable
   identity (see "Why edit history doesn't sync").
7. **Smarter active-conversation merge.** A remote change to an open
   conversation stops a reply in progress. Remote changes to a conversation that
   is streaming could wait until the reply completes.
8. **Records for values over the budget.** A value that alone exceeds the budget
   is omitted. It could be split into ordered side records (a third `oneof`
   kind) and reassembled, once telemetry shows omission is common.
9. **Telemetry** on omission rates per field, to decide whether side records,
   per-event or per-file records are worth it.
10. **Quota.** 1,000 conversations of 50 turns each is 51,000 records, over the
    server's 50,000. Lifecycle policies (sync only recent conversations) may be
    needed.
11. **Conversation ownership.** Treating a conversation as owned by the device
    that started it, read-only elsewhere or needing an explicit takeover, would
    avoid interleaved turns from two devices. The cost is that continuing a
    conversation on another device, much of why sync is wanted, becomes awkward.
