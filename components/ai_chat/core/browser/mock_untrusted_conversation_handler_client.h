// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MOCK_UNTRUSTED_CONVERSATION_HANDLER_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MOCK_UNTRUSTED_CONVERSATION_HANDLER_CLIENT_H_

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ai_chat {

class MockUntrustedConversationHandlerClient
    : public mojom::UntrustedConversationUI {
 public:
  explicit MockUntrustedConversationHandlerClient(ConversationHandler* driver);
  ~MockUntrustedConversationHandlerClient() override;

  void Disconnect();

  MOCK_METHOD(void,
              OnConversationHistoryUpdate,
              (mojom::ConversationTurnPtr),
              (override));
  MOCK_METHOD(void,
              OnToolUseEventOutput,
              (const std::string&, mojom::ToolUseEventPtr),
              (override));
  MOCK_METHOD(void,
              OnEntriesUIStateChanged,
              (mojom::ConversationEntriesStatePtr),
              (override));
  MOCK_METHOD(void,
              AssociatedContentChanged,
              (std::vector<mojom::AssociatedContentPtr>),
              (override));

 private:
  mojo::Receiver<mojom::UntrustedConversationUI> conversation_ui_receiver_{
      this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MOCK_UNTRUSTED_CONVERSATION_HANDLER_CLIENT_H_
