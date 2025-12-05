// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"

#include "base/containers/adapters.h"

namespace ai_chat {

OAIMessage::OAIMessage() = default;

OAIMessage::OAIMessage(OAIMessage&&) = default;

OAIMessage& OAIMessage::operator=(OAIMessage&&) = default;

OAIMessage::~OAIMessage() = default;

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

}  // namespace ai_chat
