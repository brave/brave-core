/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_MOCK_REMOTE_COMPLETION_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_MOCK_REMOTE_COMPLETION_CLIENT_H_

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/browser/engine/remote_completion_client.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ai_chat {

class MockRemoteCompletionClient : public RemoteCompletionClient {
 public:
  explicit MockRemoteCompletionClient(const std::string& model_name);
  ~MockRemoteCompletionClient() override;

  MOCK_METHOD(void,
              QueryPrompt,
              (const std::string&,
               std::vector<std::string>,
               GenerationCompletedCallback,
               GenerationDataCallback),
              (override));
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_MOCK_REMOTE_COMPLETION_CLIENT_H_
