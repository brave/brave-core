/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

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

void ConvertWebSourceToProto(const mojom::WebSource& source,
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

void ConvertToolUseToProto(const mojom::ToolUseEvent& tool_use,
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
          ConvertWebSourceToProto(*source, ws->add_sources());
        }
        for (const auto& rr : sources_event->rich_results) {
          ws->add_rich_results(rr);
        }
      } else if (event->is_inline_search_event()) {
        auto* is = event_proto->mutable_inline_search();
        is->set_query(event->get_inline_search_event()->query);
        is->set_results_json(event->get_inline_search_event()->results_json);
      } else if (event->is_tool_use_event()) {
        ConvertToolUseToProto(*event->get_tool_use_event(),
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
    for (const auto& s : proto.extra_snippets()) {
      snippets.push_back(s);
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
    std::vector<mojom::ContentBlockPtr> output;
    for (const auto& block : proto.output()) {
      if (block.has_text_content_block()) {
        auto text = mojom::TextContentBlock::New();
        text->text = block.text_content_block().text();
        output.push_back(
            mojom::ContentBlock::NewTextContentBlock(std::move(text)));
      } else if (block.has_image_content_block()) {
        auto image = mojom::ImageContentBlock::New();
        image->image_url = GURL(block.image_content_block().image_url());
        output.push_back(
            mojom::ContentBlock::NewImageContentBlock(std::move(image)));
      }
    }
    tool_use->output = std::move(output);
  }
  if (proto.artifacts_size() > 0) {
    std::vector<mojom::ToolArtifactPtr> artifacts;
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

  for (const auto& entry : archive.entries) {
    ConvertEntryToProto(*entry, specifics.add_entries());
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

mojom::ConversationPtr SpecificsToConversation(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  auto conversation = mojom::Conversation::New();
  conversation->uuid = specifics.uuid();
  conversation->title = specifics.title();
  if (specifics.has_model_key()) {
    conversation->model_key = specifics.model_key();
  }
  conversation->total_tokens = specifics.total_tokens();
  conversation->trimmed_tokens = specifics.trimmed_tokens();

  for (const auto& content_proto : specifics.associated_content()) {
    auto content = mojom::AssociatedContent::New();
    content->uuid = content_proto.uuid();
    content->title = content_proto.title();
    if (content_proto.has_url()) {
      content->url = GURL(content_proto.url());
    }
    content->content_type =
        static_cast<mojom::ContentType>(content_proto.content_type());
    content->content_used_percentage = content_proto.content_used_percentage();
    if (content_proto.has_conversation_entry_uuid()) {
      content->conversation_turn_uuid = content_proto.conversation_entry_uuid();
    }
    conversation->associated_content.push_back(std::move(content));
  }

  return conversation;
}

mojom::ConversationArchivePtr SpecificsToArchive(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  auto archive = mojom::ConversationArchive::New();

  for (const auto& entry_proto : specifics.entries()) {
    auto entry = mojom::ConversationTurn::New();
    if (entry_proto.has_uuid()) {
      entry->uuid = entry_proto.uuid();
    }
    if (entry_proto.has_date_unix_epoch_micros()) {
      entry->created_time = base::Time::FromDeltaSinceWindowsEpoch(
          base::Microseconds(entry_proto.date_unix_epoch_micros()));
    }
    entry->text = entry_proto.entry_text();
    if (entry_proto.has_prompt()) {
      entry->prompt = entry_proto.prompt();
    }
    entry->character_type =
        static_cast<mojom::CharacterType>(entry_proto.character_type());
    entry->action_type =
        static_cast<mojom::ActionType>(entry_proto.action_type());
    if (entry_proto.has_selected_text()) {
      entry->selected_text = entry_proto.selected_text();
    }
    if (entry_proto.has_model_key()) {
      entry->model_key = entry_proto.model_key();
    }

    std::vector<mojom::ConversationEntryEventPtr> events;
    for (const auto& event_proto : entry_proto.events()) {
      if (event_proto.has_completion_text()) {
        auto completion = mojom::CompletionEvent::New();
        completion->completion = event_proto.completion_text();
        events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
            std::move(completion)));
      } else if (event_proto.has_search_queries()) {
        auto search = mojom::SearchQueriesEvent::New();
        for (const auto& q : event_proto.search_queries().queries()) {
          search->search_queries.push_back(q);
        }
        events.push_back(mojom::ConversationEntryEvent::NewSearchQueriesEvent(
            std::move(search)));
      } else if (event_proto.has_web_sources()) {
        auto sources_event = mojom::WebSourcesEvent::New();
        for (const auto& src : event_proto.web_sources().sources()) {
          sources_event->sources.push_back(ProtoToWebSource(src));
        }
        for (const auto& rr : event_proto.web_sources().rich_results()) {
          sources_event->rich_results.push_back(rr);
        }
        events.push_back(mojom::ConversationEntryEvent::NewSourcesEvent(
            std::move(sources_event)));
      } else if (event_proto.has_inline_search()) {
        auto inline_search = mojom::InlineSearchEvent::New();
        inline_search->query = event_proto.inline_search().query();
        inline_search->results_json =
            event_proto.inline_search().results_json();
        events.push_back(mojom::ConversationEntryEvent::NewInlineSearchEvent(
            std::move(inline_search)));
      } else if (event_proto.has_tool_use()) {
        events.push_back(mojom::ConversationEntryEvent::NewToolUseEvent(
            ProtoToToolUse(event_proto.tool_use())));
      }
    }
    if (!events.empty()) {
      entry->events = std::move(events);
    }

    archive->entries.push_back(std::move(entry));
  }

  return archive;
}

}  // namespace ai_chat
