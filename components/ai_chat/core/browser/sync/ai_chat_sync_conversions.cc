/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/sync/protocol/entity_data.h"
#include "components/sync/protocol/entity_specifics.pb.h"
#include "third_party/zlib/google/compression_utils.h"

namespace ai_chat {

void WriteCompressibleString(std::string_view value,
                             sync_pb::AIChatCompressibleString* out) {
  if (value.size() < kSyncCompressionThresholdBytes) {
    out->set_raw(std::string(value));
    return;
  }
  std::string compressed;
  if (compression::GzipCompress(value, &compressed) &&
      compressed.size() < value.size()) {
    out->set_gzipped(std::move(compressed));
  } else {
    out->set_raw(std::string(value));
  }
}

void MarkCompressibleStringTruncated(sync_pb::AIChatCompressibleString* out) {
  out->clear_value();
  out->set_was_truncated_for_sync(true);
}

std::optional<std::string> ReadCompressibleString(
    const sync_pb::AIChatCompressibleString& in) {
  if (in.was_truncated_for_sync()) {
    return std::nullopt;
  }
  if (in.has_gzipped()) {
    std::string out;
    if (!compression::GzipUncompress(in.gzipped(), &out)) {
      return std::nullopt;
    }
    return out;
  }
  if (in.has_raw()) {
    return in.raw();
  }
  return std::nullopt;
}

namespace {

void WriteAssociatedContent(const mojom::AssociatedContent& content,
                            sync_pb::AIChatAssociatedContentProto* proto) {
  proto->set_uuid(content.uuid);
  proto->set_title(content.title);
  if (content.url.is_valid()) {
    proto->set_url(content.url.spec());
  }
  proto->set_content_type(static_cast<int32_t>(content.content_type));
  proto->set_content_used_percentage(content.content_used_percentage);
}

void WriteEntryFields(const mojom::ConversationTurn& entry,
                      sync_pb::AIChatConversationSpecifics::Entry* proto) {
  if (entry.uuid) {
    proto->set_uuid(*entry.uuid);
  }
  proto->set_date_unix_epoch_micros(
      entry.created_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
  proto->set_entry_text(entry.text);
  if (entry.prompt) {
    proto->set_prompt(*entry.prompt);
  }
  proto->set_character_type(static_cast<int32_t>(entry.character_type));
  proto->set_action_type(static_cast<int32_t>(entry.action_type));
  if (entry.selected_text) {
    proto->set_selected_text(*entry.selected_text);
  }
  if (entry.model_key) {
    proto->set_model_key(*entry.model_key);
  }

  // Events. Web sources, inline search, and tool use events depend on
  // serialized bytes from the database and are populated in the apply path
  // (see -flow branch).
  if (entry.events) {
    for (const auto& event : *entry.events) {
      auto* event_proto = proto->add_events();
      if (event->is_completion_event()) {
        WriteCompressibleString(event->get_completion_event()->completion,
                                event_proto->mutable_completion());
      } else if (event->is_search_queries_event()) {
        const auto& queries = event->get_search_queries_event()->search_queries;
        auto* sq = event_proto->mutable_search_queries();
        for (const auto& q : queries) {
          sq->add_queries(q);
        }
      }
    }
  }
}

}  // namespace

sync_pb::AIChatConversationSpecifics ConversationMetadataToSpecifics(
    const mojom::Conversation& conversation) {
  sync_pb::AIChatConversationSpecifics specifics;
  auto* meta = specifics.mutable_conversation();
  meta->set_uuid(conversation.uuid);
  meta->set_title(conversation.title);
  if (conversation.model_key) {
    meta->set_model_key(*conversation.model_key);
  }
  meta->set_total_tokens(conversation.total_tokens);
  meta->set_trimmed_tokens(conversation.trimmed_tokens);
  meta->set_last_modified_time_unix_epoch_micros(
      conversation.updated_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
  return specifics;
}

sync_pb::AIChatConversationSpecifics EntryToSpecifics(
    const std::string& conversation_uuid,
    const mojom::ConversationTurn& entry,
    const std::vector<mojom::AssociatedContentPtr>& associated_content) {
  sync_pb::AIChatConversationSpecifics specifics;
  auto* entry_proto = specifics.mutable_entry();
  entry_proto->set_conversation_uuid(conversation_uuid);
  WriteEntryFields(entry, entry_proto);

  // Filter associated content to items tied to this entry only.
  if (entry.uuid) {
    for (const auto& content : associated_content) {
      if (!content->conversation_turn_uuid ||
          *content->conversation_turn_uuid != *entry.uuid) {
        continue;
      }
      WriteAssociatedContent(*content, entry_proto->add_associated_content());
    }
  }

  return specifics;
}

std::unique_ptr<syncer::EntityData> CreateEntityDataFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  auto entity_data = std::make_unique<syncer::EntityData>();
  *entity_data->specifics.mutable_ai_chat_conversation() = specifics;
  if (specifics.has_conversation()) {
    entity_data->name =
        base::StrCat({"conversation:", specifics.conversation().uuid()});
  } else if (specifics.has_entry()) {
    entity_data->name = base::StrCat({"entry:", specifics.entry().uuid()});
  }
  return entity_data;
}

std::string GetStorageKeyFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  if (specifics.has_conversation()) {
    return base::StrCat(
        {kConversationStorageKeyPrefix, specifics.conversation().uuid()});
  }
  if (specifics.has_entry()) {
    return base::StrCat({kEntryStorageKeyPrefix, specifics.entry().uuid()});
  }
  return std::string();
}

std::string GetClientTagFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  return GetStorageKeyFromSpecifics(specifics);
}

std::string GetStorageKeyFromEntitySpecifics(
    const sync_pb::EntitySpecifics& specifics) {
  return GetStorageKeyFromSpecifics(specifics.ai_chat_conversation());
}

}  // namespace ai_chat
