// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/mock_conversation_handler_observer.h"

namespace ai_chat {

MockConversationHandlerObserver::MockConversationHandlerObserver() = default;
MockConversationHandlerObserver::~MockConversationHandlerObserver() = default;

void MockConversationHandlerObserver::Observe(
    ConversationHandler* conversation) {
  conversation_observations_.AddObservation(conversation);
}

}  // namespace ai_chat
