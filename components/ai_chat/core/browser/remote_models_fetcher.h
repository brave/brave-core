// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_FETCHER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_FETCHER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}

namespace api_request_helper {
class APIRequestHelper;
class APIRequestResult;
}  // namespace api_request_helper

namespace ai_chat {

// Fetches AI chat models from a remote URL endpoint and caches them.
// Implements dual-layer caching: in-memory (with TTL) + PrefService
// (persistent). Falls back to static Leo models when remote fetch fails or is
// disabled.
class RemoteModelsFetcher {
 public:
  using FetchModelsCallback =
      base::OnceCallback<void(std::vector<mojom::ModelPtr>)>;

  RemoteModelsFetcher(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~RemoteModelsFetcher();

  RemoteModelsFetcher(const RemoteModelsFetcher&) = delete;
  RemoteModelsFetcher& operator=(const RemoteModelsFetcher&) = delete;

  // Fetch models from the given URL. Calls callback with models on success,
  // or empty vector on failure (caller should use static Leo models).
  void FetchModels(const std::string& url, FetchModelsCallback callback);

  // Check if in-memory cache is valid (exists and not expired).
  bool HasValidCache() const;

  // Check if cache exists but has expired (stale).
  bool IsStale() const;

  // Get cached models from in-memory cache (only if HasValidCache() is true).
  std::vector<mojom::ModelPtr> GetCachedModels() const;

  // Load models from PrefService on startup.
  // Returns models if cache exists in prefs, empty vector otherwise.
  std::vector<mojom::ModelPtr> LoadFromPrefs();

  // Clear in-memory cache (forces re-fetch on next request).
  void ClearCache();

 private:
  // Save fetched models to PrefService for persistence.
  void SaveToPrefs(const std::vector<mojom::ModelPtr>& models,
                   const std::string& endpoint_url);

  // Parse JSON response into models.
  std::vector<mojom::ModelPtr> ParseModelsFromJSON(const base::Value& json);

  // Validate a single model from JSON.
  bool ValidateModel(const base::DictValue& model_dict) const;

  // Handle network request completion.
  void OnFetchComplete(const std::string& original_url,
                       FetchModelsCallback callback,
                       api_request_helper::APIRequestResult result);

  raw_ptr<PrefService> pref_service_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  // In-memory cache
  std::vector<mojom::ModelPtr> cached_models_;
  base::Time cache_timestamp_;
  std::string cached_endpoint_url_;

  base::WeakPtrFactory<RemoteModelsFetcher> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_FETCHER_H_
