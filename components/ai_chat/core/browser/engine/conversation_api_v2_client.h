// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_V2_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_V2_CLIENT_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"

namespace api_request_helper {
class APIRequestHelper;
}  // namespace api_request_helper

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

class AIChatCredentialManager;
class ModelService;

// Performs remote request to the remote HTTP Brave Conversation API.
class ConversationAPIV2Client {
 public:
  ConversationAPIV2Client(
      const std::string& model_name,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager,
      ModelService* model_service);
  ConversationAPIV2Client(const ConversationAPIV2Client&) = delete;
  ConversationAPIV2Client& operator=(const ConversationAPIV2Client&) = delete;
  virtual ~ConversationAPIV2Client();

  void ClearAllQueries();

 private:
  const std::string model_name_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  raw_ptr<AIChatCredentialManager> credential_manager_;
  raw_ptr<ModelService> model_service_;

  base::WeakPtrFactory<ConversationAPIV2Client> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_V2_CLIENT_H_
