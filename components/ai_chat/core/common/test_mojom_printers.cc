// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/test_mojom_printers.h"

#include <utility>

#include "base/base64.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {
namespace mojom {

void PrintTo(const AssociatedContent& content, std::ostream* os) {
  *os << "--AssociatedContent--\n";
  *os << "  uuid: " << content.uuid << "\n";
  *os << "  title: " << content.title << "\n";
  *os << "  content_id: " << content.content_id << "\n";
  *os << "  url: " << content.url.possibly_invalid_spec() << "\n";
  *os << "  content_used_percentage: " << content.content_used_percentage
      << "\n";
  *os << "  content_type: " << static_cast<int>(content.content_type) << "\n";
  *os << "  conversation_turn_uuid: "
      << content.conversation_turn_uuid.value_or("<nullopt>") << "\n";
}

void PrintTo(const Conversation& conversation, std::ostream* os) {
  *os << "--Conversation--\n";
  *os << "uuid: " << conversation.uuid << "\n";
  *os << "title: " << conversation.title << "\n";
  *os << "updated_time: "
      << base::TimeFormatFriendlyDateAndTime(conversation.updated_time) << "\n";
  *os << "has_content: " << conversation.has_content << "\n";
  if (conversation.model_key) {
    *os << "model_key: " << *conversation.model_key << "\n";
  }
  *os << "total_tokens: " << conversation.total_tokens << "\n";
  *os << "trimmed_tokens: " << conversation.trimmed_tokens << "\n";
  *os << "temporary: " << conversation.temporary << "\n";
  *os << "associated_content:\n";
  for (const auto& content : conversation.associated_content) {
    *os << "  - ";
    PrintTo(*content, os);
  }
}

void PrintTo(const mojom::ToolUseEvent& event, std::ostream* os) {
  *os << "--ToolUseEvent--\n";
  *os << "tool_name: " << event.tool_name << "\n";
  *os << "id: " << event.id << "\n";
  *os << "arguments_json: " << event.arguments_json << "\n";
  *os << "output:\n"
      << (!event.output.has_value()
              ? "[nullopt]"
              : " array with " + base::NumberToString(event.output->size()) +
                    " elements")
      << "\n";
  if (event.output) {
    for (const auto& block : event.output.value()) {
      *os << "  - ";
      if (block) {
        switch (block->which()) {
          case mojom::ContentBlock::Tag::kImageContentBlock: {
            const auto& img = block->get_image_content_block();
            *os << "image_url: " << img->image_url.possibly_invalid_spec();
            break;
          }
          case mojom::ContentBlock::Tag::kTextContentBlock: {
            const auto& txt = block->get_text_content_block();
            *os << "text: " << txt->text;
            break;
          }
          default: {
            NOTREACHED() << "Implement PrintTo for new types of content blocks";
          }
        }
      } else {
        *os << "[null]";
      }
      *os << "\n";
    }
  }

  *os << "permission_challenge:\n";
  if (event.permission_challenge) {
    *os << "  assessment: "
        << event.permission_challenge->assessment.value_or("<nullopt>") << "\n";
    *os << "  plan: " << event.permission_challenge->plan.value_or("<nullopt>")
        << "\n";
  } else {
    *os << "[nullopt]\n";
  }
}

void PrintTo(const ConversationEntryEvent& event, std::ostream* os) {
  *os << "--ConversationEntryEvent--\n";
  using Tag = ConversationEntryEvent::Tag;
  *os << "conversation_entry_event_type: " << static_cast<int>(event.which())
      << "\n";

  switch (event.which()) {
    case Tag::kCompletionEvent:
      *os << "completion: " << event.get_completion_event()->completion << "\n";
      break;
    case Tag::kSearchQueriesEvent: {
      *os << "search_queries:\n";
      for (const auto& q : event.get_search_queries_event()->search_queries) {
        *os << "  - " << q << "\n";
      }
      break;
    }
    case Tag::kSearchStatusEvent:
      *os << "is_searching: " << event.get_search_status_event()->is_searching
          << "\n";
      break;
    case Tag::kSourcesEvent: {
      *os << "sources:\n";
      for (const auto& s : event.get_sources_event()->sources) {
        *os << "  - title: " << s->title << "\n";
        *os << "    url: " << s->url.possibly_invalid_spec() << "\n";
        *os << "    favicon_url: " << s->favicon_url.possibly_invalid_spec()
            << "\n";
      }
      break;
    }
    case Tag::kToolUseEvent: {
      *os << "tool_use_event:\n";
      PrintTo(*event.get_tool_use_event(), os);
      break;
    }
    default:
      *os << "event: unknown\n";
  }
}

void PrintTo(const ConversationTurn& turn, std::ostream* os) {
  *os << "--ConversationTurn--\n";
  if (turn.uuid != std::nullopt) {
    *os << "  uuid: " << *turn.uuid << "\n";
  }
  *os << "  character_type: " << static_cast<int>(turn.character_type) << "\n";
  *os << "  action_type: " << static_cast<int>(turn.action_type) << "\n";
  *os << "  text: " << turn.text << "\n";
  if (turn.prompt != std::nullopt) {
    *os << "  prompt: " << *turn.prompt << "\n";
  }
  if (turn.selected_text != std::nullopt) {
    *os << "  selected_text: " << *turn.selected_text << "\n";
  }
  *os << "  created_time: "
      << base::TimeFormatFriendlyDateAndTime(turn.created_time) << "\n";
  *os << "  from_brave_search_SERP: " << turn.from_brave_search_SERP << "\n";
  if (turn.model_key != std::nullopt) {
    *os << "  model_key: " << *turn.model_key << "\n";
  }
  if (turn.uploaded_files) {
    *os << "  uploaded_files:\n";
    for (const auto& file : *turn.uploaded_files) {
      *os << "    - size: " << file->filesize << "\n";
      *os << "      name: " << file->filename << "\n";
      *os << "      type: " << static_cast<int>(file->type) << "\n";
      *os << "      data: " << base::Base64Encode(file->data) << "\n";
    }
  }
  if (turn.events) {
    *os << "  events:\n";
    for (const auto& ev : *turn.events) {
      *os << "    - ";
      PrintTo(*ev, os);
    }
  }
  if (turn.edits) {
    *os << "  edits:\n";
    for (const auto& edit : *turn.edits) {
      *os << "    - ";
      PrintTo(*edit, os);
    }
  }
}

void PrintTo(const ContentBlock& block, std::ostream* os) {
  *os << "ContentBlock with";
  switch (block.which()) {
    case ContentBlock::Tag::kImageContentBlock:
      *os << " image_url: "
          << block.get_image_content_block()->image_url.possibly_invalid_spec()
          << "\n";
      break;
    case ContentBlock::Tag::kTextContentBlock:
      *os << " text: \"" << block.get_text_content_block()->text << "\"\n";
      break;
    default:
      *os << " type: unknown\n";
  }
}

}  // namespace mojom
}  // namespace ai_chat
