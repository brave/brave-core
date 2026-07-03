// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_PROVIDER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_PROVIDER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/remote_models_disk_cache.h"
#include "brave/components/ai_chat/core/browser/remote_models_fetcher.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}

namespace ai_chat {

// Combines RemoteModelsDiskCache and RemoteModelsFetcher into a single async
// interface. Callers invoke GetModels() and receive the model list from
// whichever source is available: the disk cache is tried first, and a network
// fetch is performed only when the cache is absent or expired.
//
// Multiple concurrent GetModels() calls are coalesced: if a fetch is already
// in flight the new callback is queued and invoked when the fetch completes.
class RemoteModelsProvider {
 public:
  using GetModelsCallback =
      base::OnceCallback<void(std::vector<mojom::ModelPtr>)>;

  RemoteModelsProvider(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* pref_service,
      base::FilePath cache_path,
      base::TimeDelta cache_ttl,
      std::string endpoint_url);
  ~RemoteModelsProvider();

  RemoteModelsProvider(const RemoteModelsProvider&) = delete;
  RemoteModelsProvider& operator=(const RemoteModelsProvider&) = delete;

  // Returns models from the disk cache if available and within TTL, otherwise
  // fetches from the remote endpoint and caches the result. On fetch failure
  // the callback is invoked with an empty vector. The callback is always
  // invoked asynchronously.
  void GetModels(GetModelsCallback callback);

 private:
  void OnCacheLoaded(std::optional<std::vector<mojom::ModelPtr>> cached_models);
  void OnFetchComplete(std::vector<mojom::ModelPtr> models);
  void DeliverResult(std::vector<mojom::ModelPtr> models);

  RemoteModelsDiskCache cache_;
  RemoteModelsFetcher fetcher_;
  std::string endpoint_url_;

  // Pending callbacks. Non-empty while a cache load or network fetch is in
  // flight; all callbacks are delivered together when the operation completes.
  std::vector<GetModelsCallback> pending_callbacks_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<RemoteModelsProvider> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_PROVIDER_H_
