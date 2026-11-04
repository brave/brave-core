/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_CONVERSIONS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_CONVERSIONS_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/function_ref.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"

namespace sync_pb {
class AIChatCompressibleString;
class AIChatConversationSpecifics;
class AIChatConversationSpecifics_Entry;
class AIChatUploadedFile;
class EntitySpecifics;
}  // namespace sync_pb

namespace syncer {
struct EntityData;
}  // namespace syncer

namespace ai_chat {

// Storage key / client tag prefixes that distinguish the two record kinds.
inline constexpr std::string_view kConversationStorageKeyPrefix = "c:";
inline constexpr std::string_view kEntryStorageKeyPrefix = "e:";

// Minimum input size at which it is worth attempting gzip. Short strings
// gain little and may grow under compression overhead.
inline constexpr size_t kSyncCompressionThresholdBytes = 256;

// Soft cap on the serialized size of a single sync record, measured on the
// plaintext specifics. AI_CHAT_CONVERSATION is an encryptable type, so what
// reaches the server is Nigori::Encrypt() of these bytes: an IV, AES-CBC
// ciphertext and an HMAC, the whole thing base64-encoded. Base64 makes that
// overhead multiplicative rather than additive — a record at this cap commits
// at roughly 4/3 * (size + 64) bytes — so 256 KB arrives near 341 KB, leaving
// ~59 KB under the 400 KB-per-entity server limit for the rest of the
// SyncEntity. When an Entry would exceed this size, the size-budget policy
// omits low-priority fields until it fits.
inline constexpr size_t kSyncMaxRecordBytes = 256 * 1024;

// Hard cap on the decompressed size of a single field. Protection against
// malicious or corrupted data that might otherwise allocate unbounded memory.
// Theoretical compression ratio max is approximately 1032:1 but upper limit on
// practical data with high repetition is under 100:1.
inline constexpr size_t kSyncCompressionMaxDecompressedBytes =
    kSyncMaxRecordBytes * 100;

// Writes |value| into |out|. Gzips when the input exceeds the threshold and
// compression actually shrinks it; otherwise stores the raw string.
void WriteCompressibleString(std::string_view value,
                             sync_pb::AIChatCompressibleString* out);

// Omits |out|'s value, replacing it with a hash of its current content (see
// AIChatCompressibleString::omitted_content_hash). The receiver restores the
// value from a local copy that hashes identically. Without one, the value
// decodes as blank.
void OmitCompressibleString(sync_pb::AIChatCompressibleString* out);

// Omits |file|'s raw bytes, replacing them with a hash of their content (see
// AIChatUploadedFile::omitted_data_hash). Mirrors OmitCompressibleString for
// the binary-attachment case; the receiver restores the bytes from a local
// copy that hashes identically.
void OmitUploadedFileData(sync_pb::AIChatUploadedFile* file);

// Reads a value from |in|. Returns std::nullopt when there is no usable value
// to apply, for any of three reasons: the value was omitted for sync
// (restore-from-local signal), gzip decompression failed, or no value was set
// at all. Decoding treats all three the same way: the target field is left
// unset, or empty when it can't be absent, so an omitted value that wasn't
// restored first decodes as blank. Returns an empty string (not nullopt) when
// the field was set to an empty raw string.
std::optional<std::string> ReadCompressibleString(
    const sync_pb::AIChatCompressibleString& in);

// Invokes |visit| for every AIChatCompressibleString on |entry| that
// FitEntryWithinSyncBudget is allowed to omit. Code on the receiving side that
// has to recognise or undo an omission uses this so it covers exactly the field
// set the budget policy can drop, rather than maintaining a second list of its
// own. Uploaded file bytes are not covered — they are raw bytes rather than an
// AIChatCompressibleString, so callers handle
// AIChatUploadedFile::omitted_data_hash separately. A field added here must
// also be reached by ExtractOmissionMarkers() and AddOmissionMarkers(), which
// find values by position; AIChatSyncConversionsTest.
// OmissionMarkersCoverEveryOmittableValue fails until it is.
using CompressibleStringVisitor =
    base::FunctionRef<void(sync_pb::AIChatCompressibleString&)>;
void ForEachOmittableString(sync_pb::AIChatConversationSpecifics_Entry* entry,
                            CompressibleStringVisitor visit);
// The same walk, for reading a const |entry|.
using ConstCompressibleStringVisitor =
    base::FunctionRef<void(const sync_pb::AIChatCompressibleString&)>;
void ForEachOmittableString(
    const sync_pb::AIChatConversationSpecifics_Entry& entry,
    ConstCompressibleStringVisitor visit);

// Omission markers are the hashes of the values a record of an entry omitted,
// kept at the same positions, so that a device that doesn't hold a value can
// send its hash in the blank value's place. A position follows the record's
// structure, so a value added or cleared elsewhere doesn't move it:
// associated content by uuid, an uploaded file or event by index, an event or
// tool output block only when it's of the same kind, and an item of a list in
// either by index. Omission replaces values and leaves this structure alone.

// Returns the omission markers of |entry|: a record holding only hashes, with
// the associated content whose text is omitted, by uuid, the same events,
// tool output blocks and files, and each list's items up to its last omitted
// one. Returns nullopt when |entry| omits nothing.
std::optional<sync_pb::AIChatConversationSpecifics_Entry>
ExtractOmissionMarkers(const sync_pb::AIChatConversationSpecifics_Entry& entry);

// Copies each hash in |omission_markers| into the value of |entry| at the same
// position, when that value is empty: the record doesn't carry it, or carries
// an empty one. An associated content's text gets one only when the record
// doesn't carry it, since an empty text there means it was cleared.
void AddOmissionMarkers(
    const sync_pb::AIChatConversationSpecifics_Entry& omission_markers,
    sync_pb::AIChatConversationSpecifics_Entry& entry);

// Drops long-text and binary fields from |entry| (replacing each with a
// content hash so the receiver can restore it from a byte-identical local
// copy) until the serialized record fits under the per-record size budget.
// Fields are dropped in a fixed priority order — see the omission passes in
// the implementation. Returns true if the entry fits, either because no
// omission was needed or because omitting brought it under budget, and false
// if it remains too large even after every omittable field is gone — callers
// must refuse to commit such records.
bool FitEntryWithinSyncBudget(
    sync_pb::AIChatConversationSpecifics_Entry* entry);

// Builds a sync entity containing only conversation metadata. |modified_time|
// is when that metadata last changed, and a null time is left unset.
sync_pb::AIChatConversationSpecifics ConversationMetadataToSpecifics(
    const mojom::Conversation& conversation,
    base::Time modified_time);

// Builds a sync entity for a single conversation entry. |conversation_uuid|
// is the parent conversation's UUID. |modified_time| is when the entry's
// content last changed, and a null time is left unset. |associated_content| is
// filtered to only the items that belong to this entry.
// |associated_content_texts| holds the optional extracted text for each AC,
// keyed by AC UUID. An AC without a text in the map has its last_contents
// left absent, meaning the record doesn't carry it, and the receiver keeps its
// own text. An AC with no url was cleared, as clearing browsing data leaves
// it, so it's sent an explicit empty text instead, which the receiver writes.
sync_pb::AIChatConversationSpecifics EntryToSpecifics(
    const std::string& conversation_uuid,
    const mojom::ConversationTurn& entry,
    base::Time modified_time,
    const std::vector<mojom::AssociatedContentPtr>& associated_content,
    const base::flat_map<std::string, std::string>& associated_content_texts =
        {});

// Builds a record of |turn|'s own values, as EntryToSpecifics() does for an
// entry's latest revision, with no conversation, modified time or associated
// content. |turn|'s revisions are ignored, so this also builds an edited
// entry's original values. Used to find a value a sender omitted among this
// device's revisions.
sync_pb::AIChatConversationSpecifics_Entry TurnToSpecificsEntry(
    const mojom::ConversationTurn& turn);

// Returns when the record's content last changed on the device that sent it,
// as set by ConversationMetadataToSpecifics() or EntryToSpecifics(). Returns a
// null time, meaning the oldest, when the record doesn't carry one.
base::Time GetModifiedTimeFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics);

// Reverses ConversationMetadataToSpecifics. The returned Conversation has no
// entries or associated content (those are carried by Entry records).
mojom::ConversationPtr SpecificsToConversationMetadata(
    const sync_pb::AIChatConversationSpecifics& specifics);

// Reverses EntryToSpecifics. |associated_content| is populated from the
// entry's associated_content field, each tagged with this entry's UUID via
// |conversation_turn_uuid|. When non-null, |associated_content_texts|
// receives the last_contents value for each AC where the sender provided
// one. An AC with no map entry, because its text was absent or omitted, gets
// a blank text when the sync bridge writes it, unless the bridge filled in
// this device's text first, as it does for an absent one.
// A value the sender omitted to fit the size limit decodes as blank, since
// most such fields can't be absent in mojom, and an omitted list item as an
// empty one, so the items keep their positions. A caller that holds a copy of
// the entry puts its values back into |specifics| first, as the sync bridge
// does.
mojom::ConversationTurnPtr SpecificsToEntry(
    const sync_pb::AIChatConversationSpecifics& specifics,
    std::vector<mojom::AssociatedContentPtr>& associated_content,
    base::flat_map<std::string, std::string>* associated_content_texts =
        nullptr);

// Wraps an AIChatConversationSpecifics in EntityData for change_processor()
// to consume. Sets the entity name to a human-readable string.
std::unique_ptr<syncer::EntityData> CreateEntityDataFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics);

// Returns the storage key for the record, including the kind prefix.
std::string GetStorageKeyFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics);

// Same as GetStorageKeyFromSpecifics — the client tag is identical to the
// storage key since both are derived from a stable UUID + kind prefix.
std::string GetClientTagFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics);

// Extracts the storage key from an EntitySpecifics wrapper.
std::string GetStorageKeyFromEntitySpecifics(
    const sync_pb::EntitySpecifics& specifics);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_CONVERSIONS_H_
