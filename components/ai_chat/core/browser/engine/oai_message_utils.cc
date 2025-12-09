// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"

#include "base/containers/adapters.h"

namespace ai_chat {

namespace {

ExtendedContentBlock GetContentBlockFromAssociatedContent(
    const PageContent& content,
    uint32_t remaining_length,
    base::FunctionRef<void(std::string&)> sanitize_input) {
  std::string truncated = content.content.substr(0, remaining_length);
  sanitize_input(truncated);
  return ExtendedContentBlock{content.is_video
                                  ? ExtendedContentBlockType::kVideoTranscript
                                  : ExtendedContentBlockType::kPageText,
                              TextContent{std::move(truncated)}};
}

}  // namespace

OAIMessage::OAIMessage() = default;

OAIMessage::OAIMessage(OAIMessage&&) = default;

OAIMessage& OAIMessage::operator=(OAIMessage&&) = default;

OAIMessage::~OAIMessage() = default;

std::vector<OAIMessage> BuildOAIMessages(
    PageContentsMap&& page_contents,
    const EngineConsumer::ConversationHistory& conversation_history,
    uint32_t remaining_length,
    base::FunctionRef<void(std::string&)> sanitize_input) {
  std::vector<OAIMessage> oai_messages;

  // Key is conversation entry uuid, value is a list of content blocks for that
  // entry.
  // We use this so we can look up all the page content blocks for a given
  // conversation entry.
  base::flat_map<std::string, std::vector<ExtendedContentBlock>>
      page_contents_blocks;

  // Step 1:
  //   - generate content blocks for the page contents which we're going to
  //   keep.
  for (size_t message_index = conversation_history.size(); message_index > 0;
       --message_index) {
    const auto& message = conversation_history[message_index - 1];
    DCHECK(message->uuid) << "Tried to send a turn without a uuid";
    if (!message->uuid) {
      continue;
    }

    // If we have page contents for this turn, generate a content block for
    // each.
    auto page_content_it = page_contents.find(message->uuid.value());
    if (page_content_it != page_contents.end() && remaining_length != 0) {
      auto& blocks = page_contents_blocks[message->uuid.value()];
      for (const auto& content : base::Reversed(page_content_it->second)) {
        if (remaining_length == 0) {
          break;
        }

        blocks.emplace_back(GetContentBlockFromAssociatedContent(
            content, remaining_length, sanitize_input));

        if (content.get().content.size() > remaining_length) {
          remaining_length = 0;
        } else {
          remaining_length -= content.get().content.size();
        }
      }
    }
  }

  // Step 2: Main pass - build conversation in chronological order
  for (size_t message_index = 0; message_index < conversation_history.size();
       ++message_index) {
    const auto& message = conversation_history[message_index];

    OAIMessage oai_message;
    oai_message.role = message->character_type == mojom::CharacterType::HUMAN
                           ? "user"
                           : "assistant";

    // Append associated content for the message (if any).
    // Note: We don't create the blocks here because we want to keep the newest
    // page contents until we run out of context, so they need to be built in
    // reverse chronological order.
    auto page_content_it = page_contents_blocks.find(message->uuid.value());
    if (page_content_it != page_contents_blocks.end()) {
      for (auto& block : page_content_it->second) {
        oai_message.content.emplace_back(std::move(block));
      }
    }

    if (message->selected_text.has_value() &&
        !message->selected_text->empty()) {
      oai_message.content.emplace_back(ExtendedContentBlockType::kPageExcerpt,
                                       TextContent{*message->selected_text});
    }

    // Build the main content block
    if (message->action_type == mojom::ActionType::SUMMARIZE_PAGE) {
      oai_message.content.emplace_back(
          ExtendedContentBlockType::kRequestSummary, TextContent{""});
    } else {
      oai_message.content.emplace_back(
          ExtendedContentBlockType::kText,
          TextContent{EngineConsumer::GetPromptForEntry(message)});
    }

    oai_messages.emplace_back(std::move(oai_message));
  }

  return oai_messages;
}

std::optional<std::vector<OAIMessage>> BuildOAIRewriteSuggestionMessages(
    const std::string& text,
    mojom::ActionType action_type) {
  std::vector<OAIMessage> messages;
  OAIMessage msg;
  msg.role = "user";

  msg.content.emplace_back(ExtendedContentBlockType::kPageExcerpt,
                           TextContent{text});

  switch (action_type) {
    case mojom::ActionType::PARAPHRASE:
      msg.content.emplace_back(ExtendedContentBlockType::kParaphrase,
                               TextContent{""});
      break;
    case mojom::ActionType::IMPROVE:
      msg.content.emplace_back(ExtendedContentBlockType::kImprove,
                               TextContent{""});
      break;
    case mojom::ActionType::ACADEMICIZE:
      msg.content.emplace_back(ExtendedContentBlockType::kChangeTone,
                               ChangeToneContent{"academic"});
      break;
    case mojom::ActionType::PROFESSIONALIZE:
      msg.content.emplace_back(ExtendedContentBlockType::kChangeTone,
                               ChangeToneContent{"professional"});
      break;
    case mojom::ActionType::PERSUASIVE_TONE:
      msg.content.emplace_back(ExtendedContentBlockType::kChangeTone,
                               ChangeToneContent{"persuasive"});
      break;
    case mojom::ActionType::CASUALIZE:
      msg.content.emplace_back(ExtendedContentBlockType::kChangeTone,
                               ChangeToneContent{"casual"});
      break;
    case mojom::ActionType::FUNNY_TONE:
      msg.content.emplace_back(ExtendedContentBlockType::kChangeTone,
                               ChangeToneContent{"funny"});
      break;
    case mojom::ActionType::SHORTEN:
      msg.content.emplace_back(ExtendedContentBlockType::kShorten,
                               TextContent{""});
      break;
    case mojom::ActionType::EXPAND:
      msg.content.emplace_back(ExtendedContentBlockType::kExpand,
                               TextContent{""});
      break;
    default:
      return std::nullopt;
  }

  messages.push_back(std::move(msg));
  return messages;
}

OAIMessage BuildOAISeedMessage(const std::string& text) {
  OAIMessage message;
  message.role = "assistant";
  message.content.emplace_back(ExtendedContentBlockType::kText,
                               TextContent{text});
  return message;
}

}  // namespace ai_chat
