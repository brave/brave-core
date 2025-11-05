// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MOCK_CONVERSATION_HANDLER_OBSERVER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MOCK_CONVERSATION_HANDLER_OBSERVER_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/scoped_multi_source_observation.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ai_chat {

class MockConversationHandlerObserver : public ConversationHandler::Observer {
 public:
  MockConversationHandlerObserver();
  ~MockConversationHandlerObserver() override;

  void Observe(ConversationHandler* conversation);

  MOCK_METHOD(void,
              OnRequestInProgressChanged,
              (ConversationHandler*, bool),
              (override));

  MOCK_METHOD(void,
              OnConversationEntryAdded,
              (ConversationHandler*,
               mojom::ConversationTurnPtr&,
               std::optional<PageContents>),
              (override));

  MOCK_METHOD(void,
              OnConversationEntryRemoved,
              (ConversationHandler*, std::string),
              (override));

  MOCK_METHOD(void,
              OnToolUseEventOutput,
              (ConversationHandler*,
               std::string_view entry_uuid,
               size_t event_order,
               mojom::ToolUseEventPtr tool_use),
              (override));

  MOCK_METHOD(void,
              OnClientConnectionChanged,
              (ConversationHandler*),
              (override));

  MOCK_METHOD(void,
              OnConversationTitleChanged,
              (const std::string&, const std::string&),
              (override));

  MOCK_METHOD(void,
              OnConversationTokenInfoChanged,
              (const std::string&, uint64_t, uint64_t),
              (override));

 private:
  base::ScopedMultiSourceObservation<ConversationHandler,
                                     ConversationHandler::Observer>
      conversation_observations_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MOCK_CONVERSATION_HANDLER_OBSERVER_H_
