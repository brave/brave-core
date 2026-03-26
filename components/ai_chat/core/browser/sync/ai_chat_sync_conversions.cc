/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/sync/protocol/entity_data.h"
#include "components/sync/protocol/entity_specifics.pb.h"

namespace ai_chat {

namespace {

void ConvertEntryToProto(const mojom::ConversationTurn& entry,
                         sync_pb::AIChatConversationEntryProto* proto) {
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

  // Convert events
  if (entry.events) {
    for (const auto& event : *entry.events) {
      auto* event_proto = proto->add_events();
      if (event->is_completion_event()) {
        event_proto->set_completion_text(
            event->get_completion_event()->completion);
      } else if (event->is_search_queries_event()) {
        const auto& queries = event->get_search_queries_event()->search_queries;
        auto* sq = event_proto->mutable_search_queries();
        for (const auto& q : queries) {
          sq->add_queries(q);
        }
      }
      // web_sources, inline_search, and tool_use events use serialized bytes
      // which would need the raw bytes from the database. For now, these are
      // handled at the database layer when reading for sync.
    }
  }

  // Edits are stored as separate conversation entries with editing_entry_uuid
  // set. They are included in the entries list from the archive, so no special
  // handling is needed here.
}

}  // namespace

sync_pb::AIChatConversationSpecifics ConversationToSpecifics(
    const mojom::Conversation& conversation,
    const mojom::ConversationArchive& archive) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.set_uuid(conversation.uuid);
  specifics.set_title(conversation.title);
  if (conversation.model_key) {
    specifics.set_model_key(*conversation.model_key);
  }
  specifics.set_total_tokens(conversation.total_tokens);
  specifics.set_trimmed_tokens(conversation.trimmed_tokens);

  // Convert associated content metadata (not full text)
  for (const auto& content : conversation.associated_content) {
    auto* content_proto = specifics.add_associated_content();
    content_proto->set_uuid(content->uuid);
    content_proto->set_title(content->title);
    if (content->url.is_valid()) {
      content_proto->set_url(content->url.spec());
    }
    content_proto->set_content_type(
        static_cast<int32_t>(content->content_type));
    content_proto->set_content_used_percentage(
        content->content_used_percentage);
    if (content->conversation_turn_uuid) {
      content_proto->set_conversation_entry_uuid(
          *content->conversation_turn_uuid);
    }
  }

  // Convert entries
  for (const auto& entry : archive.entries) {
    auto* entry_proto = specifics.add_entries();
    ConvertEntryToProto(*entry, entry_proto);
  }

  return specifics;
}

std::unique_ptr<syncer::EntityData> CreateEntityDataFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  auto entity_data = std::make_unique<syncer::EntityData>();
  *entity_data->specifics.mutable_ai_chat_conversation() = specifics;
  entity_data->name = specifics.uuid();
  return entity_data;
}

std::string GetStorageKeyFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  return specifics.uuid();
}

std::string GetClientTagFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  return specifics.uuid();
}

std::string GetStorageKeyFromEntitySpecifics(
    const sync_pb::EntitySpecifics& specifics) {
  return specifics.ai_chat_conversation().uuid();
}

}  // namespace ai_chat
