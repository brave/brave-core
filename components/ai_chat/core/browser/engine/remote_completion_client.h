/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_REMOTE_COMPLETION_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_REMOTE_COMPLETION_CLIENT_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace base {
class Value;
}  // namespace base
template <class T>
class scoped_refptr;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {
class AIChatCredentialManager;
struct CredentialCacheEntry;

using api_request_helper::APIRequestResult;

class RemoteCompletionClient {
 public:
  using GenerationResult = base::expected<std::string, mojom::APIError>;
  using GenerationDataCallback =
      base::RepeatingCallback<void(mojom::ConversationEntryEventPtr)>;
  using GenerationCompletedCallback =
      base::OnceCallback<void(GenerationResult)>;

  RemoteCompletionClient(
      const std::string& model_name,
      base::flat_set<std::string_view> stop_sequences,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager);

  RemoteCompletionClient(const RemoteCompletionClient&) = delete;
  RemoteCompletionClient& operator=(const RemoteCompletionClient&) = delete;
  virtual ~RemoteCompletionClient();

  // This function queries both types of APIs: SSE and non-SSE.
  // In non-SSE cases, only the data_completed_callback will be triggered.
  virtual void QueryPrompt(
      const std::string& prompt,
      std::vector<std::string> stop_sequences,
      GenerationCompletedCallback data_completed_callback,
      GenerationDataCallback data_received_callback = base::NullCallback());
  // Clears all in-progress requests
  void ClearAllQueries();

 private:
  void OnQueryDataReceived(GenerationDataCallback callback,
                           base::expected<base::Value, std::string> result);
  void OnQueryCompleted(std::optional<CredentialCacheEntry> credential,
                        GenerationCompletedCallback callback,
                        APIRequestResult result);

  void OnFetchPremiumCredential(
      const std::string& prompt,
      const std::vector<std::string>& extra_stop_sequences,
      GenerationCompletedCallback data_completed_callback,
      GenerationDataCallback data_received_callback,
      std::optional<CredentialCacheEntry> credential);

  const std::string model_name_;
  const base::flat_set<std::string_view> stop_sequences_;
  api_request_helper::APIRequestHelper api_request_helper_;
  raw_ptr<AIChatCredentialManager> credential_manager_;

  base::WeakPtrFactory<RemoteCompletionClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_REMOTE_COMPLETION_CLIENT_H_
