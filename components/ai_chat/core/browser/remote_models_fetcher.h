// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_FETCHER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_FETCHER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace base {
class Value;
}  // namespace base

namespace network {
class SharedURLLoaderFactory;
}

namespace api_request_helper {
class APIRequestHelper;
class APIRequestResult;
}  // namespace api_request_helper

namespace ai_chat {

// Fetches AI chat models from a remote URL endpoint and parses the response.
class RemoteModelsFetcher {
 public:
  using FetchModelsCallback =
      base::OnceCallback<void(std::vector<mojom::ModelPtr>)>;

  explicit RemoteModelsFetcher(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~RemoteModelsFetcher();

  RemoteModelsFetcher(const RemoteModelsFetcher&) = delete;
  RemoteModelsFetcher& operator=(const RemoteModelsFetcher&) = delete;

  // Calls callback with parsed models on success, or empty vector on failure.
  void FetchModels(const std::string& url, FetchModelsCallback callback);

 private:
  std::vector<mojom::ModelPtr> ParseModelsFromJSON(const base::Value& json);

  void OnFetchComplete(const std::string& original_url,
                       FetchModelsCallback callback,
                       api_request_helper::APIRequestResult result);

  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  base::WeakPtrFactory<RemoteModelsFetcher> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_FETCHER_H_
