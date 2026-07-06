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

// Marks |out| as truncated for sync (no value bytes). The receiver MUST
// preserve any existing local value for this field rather than overwriting
// it with empty.
void MarkCompressibleStringTruncated(sync_pb::AIChatCompressibleString* out);

// Reads a value from |in|. Returns std::nullopt when there is no usable value
// to apply, for any of three reasons: the field was marked truncated for sync
// (preserve-local signal), gzip decompression failed, or no value was set at
// all. Callers treat all three the same way — leave the target field unset and
// preserve any existing local value. Returns an empty string (not nullopt) when
// the field was set to an empty raw string.
std::optional<std::string> ReadCompressibleString(
    const sync_pb::AIChatCompressibleString& in);

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
