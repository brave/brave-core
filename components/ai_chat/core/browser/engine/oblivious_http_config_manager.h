// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OBLIVIOUS_HTTP_CONFIG_MANAGER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OBLIVIOUS_HTTP_CONFIG_MANAGER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "url/gurl.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

// Manages fetching and caching the OHTTP key config used for private AI Chat
// inference. Configs are cached per model name in a dict pref. Each fetch
// retrieves a key config and endpoint URL from
// /v1/models/{model_name}/ohttp_config. Concurrent requests for the same model
// are handled by APIRequestHelper independently.
class ObliviousHttpConfigManager {
 public:
  struct KeyConfigResult {
    std::string key_config;  // decoded raw HPKE key bytes
    GURL endpoint_url;       // inner-request resource URL from the server
  };

  // Callback receives the key config result on success, or std::nullopt on
  // failure.
  using KeyConfigCallback =
      base::OnceCallback<void(std::optional<KeyConfigResult>)>;

  static void DeleteExpiredKeyConfigs(PrefService* profile_prefs);

  ObliviousHttpConfigManager(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* profile_prefs);

  ObliviousHttpConfigManager(const ObliviousHttpConfigManager&) = delete;
  ObliviousHttpConfigManager& operator=(const ObliviousHttpConfigManager&) =
      delete;
  ~ObliviousHttpConfigManager();

  // Ensures a fresh key config is available for |model_name|, then calls
  // |callback|. If a valid cached config exists the callback is invoked
  // synchronously.
  void RequestKeyConfig(const std::string& model_name,
                        KeyConfigCallback callback);

  // Cancels any in-flight fetches and drops all queued callbacks.
  void CancelAll();

 private:
  std::optional<KeyConfigResult> GetCachedKeyConfig(
      const std::string& model_name) const;
  void FetchKeyConfig(const std::string& model_name,
                      KeyConfigCallback callback);
  void OnKeyConfigFetched(std::string model_name,
                          KeyConfigCallback callback,
                          api_request_helper::APIRequestResult result);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  raw_ptr<PrefService> profile_prefs_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  base::WeakPtrFactory<ObliviousHttpConfigManager> weak_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OBLIVIOUS_HTTP_CONFIG_MANAGER_H_
