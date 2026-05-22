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
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"

namespace sync_pb {
class AIChatCompressibleString;
class AIChatConversationSpecifics;
class AIChatConversationSpecifics_Entry;
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

// Soft cap on the serialized size of a single sync record. Leaves headroom
// under the 400 KB-per-entity server limit for sync framing and encryption
// overhead. When an Entry would exceed this size, the truncation policy
// drops bytes from low-priority fields until it fits.
inline constexpr size_t kSyncMaxRecordBytes = 350 * 1024;

// Writes |value| into |out|. Gzips when the input exceeds the threshold and
// compression actually shrinks it; otherwise stores the raw string.
void WriteCompressibleString(std::string_view value,
                             sync_pb::AIChatCompressibleString* out);

// Marks |out| as truncated for sync (no value bytes). The receiver MUST
// preserve any existing local value for this field rather than overwriting
// it with empty.
void MarkCompressibleStringTruncated(sync_pb::AIChatCompressibleString* out);

// Reads a value from |in|. Returns std::nullopt when the field was marked
// truncated for sync (preserve-local signal) or when gzip decompression
// fails. Returns an empty string when the field was set to an empty raw
// string.
std::optional<std::string> ReadCompressibleString(
    const sync_pb::AIChatCompressibleString& in);

// Walks a priority-ordered list of long-text and binary fields on |entry|,
// truncating them (marking the per-field sentinel so the receiver
// preserves any local value) until the serialized record fits under the
// per-record size budget. Returns true if the entry now fits (either
// because no truncation was needed or because it succeeded), and false if
// the entry remains too large even after every truncatable field has been
// dropped — callers should refuse to commit such records.
bool TruncateEntryForSync(sync_pb::AIChatConversationSpecifics_Entry* entry);

// Builds a sync entity containing only conversation metadata.
sync_pb::AIChatConversationSpecifics ConversationMetadataToSpecifics(
    const mojom::Conversation& conversation);

// Builds a sync entity for a single conversation entry. |conversation_uuid|
// is the parent conversation's UUID. |associated_content| is filtered to
// only the items that belong to this entry. |associated_content_texts|
// holds the optional extracted text for each AC, keyed by AC UUID; entries
// without a text in the map have their last_contents field left absent on
// the wire so the receiver preserves any existing local text.
sync_pb::AIChatConversationSpecifics EntryToSpecifics(
    const std::string& conversation_uuid,
    const mojom::ConversationTurn& entry,
    const std::vector<mojom::AssociatedContentPtr>& associated_content,
    const base::flat_map<std::string, std::string>& associated_content_texts =
        {});

// Reverses ConversationMetadataToSpecifics. The returned Conversation has no
// entries or associated content (those are carried by Entry records).
mojom::ConversationPtr SpecificsToConversationMetadata(
    const sync_pb::AIChatConversationSpecifics& specifics);

// Reverses EntryToSpecifics. |associated_content| is populated from the
// entry's associated_content field, each tagged with this entry's UUID via
// |conversation_turn_uuid|. When non-null, |associated_content_texts|
// receives the last_contents value for each AC where the sender provided
// one; absent map entries mean the caller should preserve any existing
// local text (forward-compat or truncated-for-sync).
mojom::ConversationTurnPtr SpecificsToEntry(
    const sync_pb::AIChatConversationSpecifics& specifics,
    std::vector<mojom::AssociatedContentPtr>* associated_content,
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
