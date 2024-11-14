// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"

#include <optional>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

EngineConsumer::EngineConsumer() = default;
EngineConsumer::~EngineConsumer() = default;

bool EngineConsumer::SupportsDeltaTextResponses() const {
  return false;
}

bool EngineConsumer::CanPerformCompletionRequest(
    const ConversationHistory& conversation_history) const {
  if (conversation_history.empty()) {
    return false;
  }

  const auto& last_turn = conversation_history.back();
  if (last_turn->character_type != mojom::CharacterType::HUMAN) {
    return false;
  }

  return true;
}

}  // namespace ai_chat
