// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MOCK_CONVERSATION_HANDLER_OBSERVER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MOCK_CONVERSATION_HANDLER_OBSERVER_H_

#include <string>
#include <string_view>

#include "base/scoped_multi_source_observation.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ai_chat {

class MockConversationHandlerObserver : public ConversationHandler::Observer {
 public:
  MockConversationHandlerObserver();
  ~MockConversationHandlerObserver() override;

  void Observe(ConversationHandler* conversation);

  MOCK_METHOD(void,
              OnRequestInProgressChanged,
              (ConversationHandler * handler, bool in_progress),
              (override));

  MOCK_METHOD(void,
              OnConversationEntryAdded,
              (ConversationHandler * handler,
               mojom::ConversationTurnPtr& entry,
               std::optional<std::string_view> associated_content_value),
              (override));

  MOCK_METHOD(void,
              OnConversationEntryRemoved,
              (ConversationHandler * handler, std::string turn_uuid),
              (override));

  MOCK_METHOD(void,
              OnConversationEntryUpdated,
              (ConversationHandler * handler, mojom::ConversationTurnPtr entry),
              (override));

  MOCK_METHOD(void,
              OnClientConnectionChanged,
              (ConversationHandler*),
              (override));

  MOCK_METHOD(void,
              OnConversationTitleChanged,
              (ConversationHandler * handler, std::string title),
              (override));

 private:
  base::ScopedMultiSourceObservation<ConversationHandler,
                                     ConversationHandler::Observer>
      conversation_observations_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MOCK_CONVERSATION_HANDLER_OBSERVER_H_
