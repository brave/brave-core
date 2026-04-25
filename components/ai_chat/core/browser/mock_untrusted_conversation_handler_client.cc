// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/mock_untrusted_conversation_handler_client.h"

#include "base/functional/callback_helpers.h"

namespace ai_chat {

MockUntrustedConversationHandlerClient::MockUntrustedConversationHandlerClient(
    ConversationHandler* driver) {
  driver->BindUntrustedConversationUI(
      conversation_ui_receiver_.BindNewPipeAndPassRemote(), base::DoNothing());
}

MockUntrustedConversationHandlerClient::
    ~MockUntrustedConversationHandlerClient() = default;

void MockUntrustedConversationHandlerClient::Disconnect() {
  conversation_ui_receiver_.reset();
}

}  // namespace ai_chat
