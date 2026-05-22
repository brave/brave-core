/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/sync/protocol/entity_data.h"
#include "components/sync/protocol/entity_specifics.pb.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

// --- Upload helpers: mojom → sync proto ---

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

void WriteWebSource(const mojom::WebSource& source,
                    sync_pb::AIChatWebSource* proto) {
  proto->set_title(source.title);
  if (source.url.is_valid()) {
    proto->set_url(source.url.spec());
  }
  if (source.favicon_url.is_valid()) {
    proto->set_favicon_url(source.favicon_url.spec());
  }
  if (source.page_content) {
    proto->set_page_content(*source.page_content);
  }
  if (source.extra_snippets) {
    for (const auto& snippet : *source.extra_snippets) {
      proto->add_extra_snippets(snippet);
    }
  }
}

void WriteToolUse(const mojom::ToolUseEvent& tool_use,
                  sync_pb::AIChatToolUseEvent* proto) {
  proto->set_tool_name(tool_use.tool_name);
  proto->set_id(tool_use.id);
  proto->set_arguments_json(tool_use.arguments_json);
  proto->set_is_server_result(tool_use.is_server_result);
  if (tool_use.output) {
    for (const auto& block : *tool_use.output) {
      auto* block_proto = proto->add_output();
      if (block->is_text_content_block()) {
        block_proto->mutable_text_content_block()->set_text(
            block->get_text_content_block()->text);
      } else if (block->is_image_content_block()) {
        block_proto->mutable_image_content_block()->set_image_url(
            block->get_image_content_block()->image_url.spec());
      }
    }
  }
  if (tool_use.artifacts) {
    for (const auto& artifact : *tool_use.artifacts) {
      auto* a = proto->add_artifacts();
      a->set_type(artifact->type);
      a->set_content_json(artifact->content_json);
    }
  }
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

  if (entry.events) {
    int order = 0;
    for (const auto& event : *entry.events) {
      auto* event_proto = proto->add_events();
      event_proto->set_event_order(order++);
      if (event->is_completion_event()) {
        event_proto->set_completion_text(
            event->get_completion_event()->completion);
      } else if (event->is_search_queries_event()) {
        const auto& queries = event->get_search_queries_event()->search_queries;
        auto* sq = event_proto->mutable_search_queries();
        for (const auto& q : queries) {
          sq->add_queries(q);
        }
      } else if (event->is_sources_event()) {
        auto* ws = event_proto->mutable_web_sources();
        const auto& sources_event = event->get_sources_event();
        for (const auto& source : sources_event->sources) {
          WriteWebSource(*source, ws->add_sources());
        }
        for (const auto& rr : sources_event->rich_results) {
          ws->add_rich_results(rr);
        }
      } else if (event->is_inline_search_event()) {
        auto* is = event_proto->mutable_inline_search();
        is->set_query(event->get_inline_search_event()->query);
        is->set_results_json(event->get_inline_search_event()->results_json);
      } else if (event->is_tool_use_event()) {
        WriteToolUse(*event->get_tool_use_event(),
                     event_proto->mutable_tool_use());
      }
    }
  }
}

// --- Download helpers: sync proto → mojom ---

mojom::WebSourcePtr ProtoToWebSource(const sync_pb::AIChatWebSource& proto) {
  auto source = mojom::WebSource::New();
  source->title = proto.title();
  if (proto.has_url()) {
    source->url = GURL(proto.url());
  }
  if (proto.has_favicon_url()) {
    source->favicon_url = GURL(proto.favicon_url());
  }
  if (proto.has_page_content()) {
    source->page_content = proto.page_content();
  }
  if (proto.extra_snippets_size() > 0) {
    std::vector<std::string> snippets;
    snippets.reserve(proto.extra_snippets_size());
    for (const auto& snippet : proto.extra_snippets()) {
      snippets.push_back(snippet);
    }
    source->extra_snippets = std::move(snippets);
  }
  return source;
}

mojom::ToolUseEventPtr ProtoToToolUse(
    const sync_pb::AIChatToolUseEvent& proto) {
  auto tool_use = mojom::ToolUseEvent::New();
  tool_use->tool_name = proto.tool_name();
  tool_use->id = proto.id();
  tool_use->arguments_json = proto.arguments_json();
  tool_use->is_server_result = proto.is_server_result();
  if (proto.output_size() > 0) {
    std::vector<mojom::ContentBlockPtr> blocks;
    blocks.reserve(proto.output_size());
    for (const auto& block_proto : proto.output()) {
      mojom::ContentBlockPtr block;
      if (block_proto.has_text_content_block()) {
        auto text = mojom::TextContentBlock::New();
        text->text = block_proto.text_content_block().text();
        block = mojom::ContentBlock::NewTextContentBlock(std::move(text));
      } else if (block_proto.has_image_content_block()) {
        auto image = mojom::ImageContentBlock::New();
        image->image_url = GURL(block_proto.image_content_block().image_url());
        block = mojom::ContentBlock::NewImageContentBlock(std::move(image));
      } else {
        continue;
      }
      blocks.push_back(std::move(block));
    }
    tool_use->output = std::move(blocks);
  }
  if (proto.artifacts_size() > 0) {
    std::vector<mojom::ToolArtifactPtr> artifacts;
    artifacts.reserve(proto.artifacts_size());
    for (const auto& a : proto.artifacts()) {
      auto artifact = mojom::ToolArtifact::New();
      artifact->type = a.type();
      artifact->content_json = a.content_json();
      artifacts.push_back(std::move(artifact));
    }
    tool_use->artifacts = std::move(artifacts);
  }
  return tool_use;
}

mojom::ConversationEntryEventPtr ProtoToEntryEvent(
    const sync_pb::AIChatEntryEventProto& proto) {
  if (proto.has_completion_text()) {
    auto completion = mojom::CompletionEvent::New();
    completion->completion = proto.completion_text();
    return mojom::ConversationEntryEvent::NewCompletionEvent(
        std::move(completion));
  }
  if (proto.has_search_queries()) {
    auto event = mojom::SearchQueriesEvent::New();
    for (const auto& q : proto.search_queries().queries()) {
      event->search_queries.push_back(q);
    }
    return mojom::ConversationEntryEvent::NewSearchQueriesEvent(
        std::move(event));
  }
  if (proto.has_web_sources()) {
    auto event = mojom::WebSourcesEvent::New();
    for (const auto& source : proto.web_sources().sources()) {
      event->sources.push_back(ProtoToWebSource(source));
    }
    for (const auto& rr : proto.web_sources().rich_results()) {
      event->rich_results.push_back(rr);
    }
    return mojom::ConversationEntryEvent::NewSourcesEvent(std::move(event));
  }
  if (proto.has_inline_search()) {
    auto event = mojom::InlineSearchEvent::New();
    event->query = proto.inline_search().query();
    event->results_json = proto.inline_search().results_json();
    return mojom::ConversationEntryEvent::NewInlineSearchEvent(
        std::move(event));
  }
  if (proto.has_tool_use()) {
    return mojom::ConversationEntryEvent::NewToolUseEvent(
        ProtoToToolUse(proto.tool_use()));
  }
  return nullptr;
}

mojom::AssociatedContentPtr ProtoToAssociatedContent(
    const sync_pb::AIChatAssociatedContentProto& proto,
    const std::string& entry_uuid) {
  auto content = mojom::AssociatedContent::New();
  content->uuid = proto.uuid();
  content->title = proto.title();
  if (proto.has_url()) {
    content->url = GURL(proto.url());
  }
  content->content_type = static_cast<mojom::ContentType>(proto.content_type());
  content->content_used_percentage = proto.content_used_percentage();
  content->conversation_turn_uuid = entry_uuid;
  return content;
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

mojom::ConversationPtr SpecificsToConversationMetadata(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  if (!specifics.has_conversation()) {
    return nullptr;
  }
  const auto& meta = specifics.conversation();
  auto conversation = mojom::Conversation::New();
  conversation->uuid = meta.uuid();
  conversation->title = meta.title();
  if (meta.has_model_key()) {
    conversation->model_key = meta.model_key();
  }
  conversation->total_tokens = meta.total_tokens();
  conversation->trimmed_tokens = meta.trimmed_tokens();
  conversation->updated_time = base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(meta.last_modified_time_unix_epoch_micros()));
  conversation->has_content = true;
  return conversation;
}

mojom::ConversationTurnPtr SpecificsToEntry(
    const sync_pb::AIChatConversationSpecifics& specifics,
    std::vector<mojom::AssociatedContentPtr>* associated_content) {
  if (!specifics.has_entry()) {
    return nullptr;
  }
  const auto& proto = specifics.entry();
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = proto.uuid();
  entry->created_time = base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(proto.date_unix_epoch_micros()));
  entry->text = proto.entry_text();
  if (proto.has_prompt()) {
    entry->prompt = proto.prompt();
  }
  entry->character_type =
      static_cast<mojom::CharacterType>(proto.character_type());
  entry->action_type = static_cast<mojom::ActionType>(proto.action_type());
  if (proto.has_selected_text()) {
    entry->selected_text = proto.selected_text();
  }
  if (proto.has_model_key()) {
    entry->model_key = proto.model_key();
  }

  if (proto.events_size() > 0) {
    std::vector<mojom::ConversationEntryEventPtr> events;
    events.reserve(proto.events_size());
    for (const auto& event_proto : proto.events()) {
      auto event = ProtoToEntryEvent(event_proto);
      if (event) {
        events.push_back(std::move(event));
      }
    }
    entry->events = std::move(events);
  }

  if (associated_content && proto.associated_content_size() > 0) {
    for (const auto& content_proto : proto.associated_content()) {
      associated_content->push_back(
          ProtoToAssociatedContent(content_proto, proto.uuid()));
    }
  }
  return entry;
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
