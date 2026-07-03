// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_provider.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

RemoteModelsProvider::RemoteModelsProvider(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* pref_service,
    base::FilePath cache_path,
    base::TimeDelta cache_ttl,
    std::string endpoint_url)
    : cache_(std::move(cache_path), cache_ttl, pref_service),
      fetcher_(url_loader_factory),
      endpoint_url_(std::move(endpoint_url)) {}

RemoteModelsProvider::~RemoteModelsProvider() = default;

void RemoteModelsProvider::GetModels(GetModelsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool in_progress = !pending_callbacks_.empty();
  pending_callbacks_.push_back(std::move(callback));

  if (in_progress) {
    // A cache load or network fetch is already in flight; this callback will
    // be delivered when it completes.
    return;
  }

  cache_.Load(base::BindOnce(&RemoteModelsProvider::OnCacheLoaded,
                             weak_ptr_factory_.GetWeakPtr()));
}

void RemoteModelsProvider::OnCacheLoaded(
    std::optional<std::vector<mojom::ModelPtr>> cached_models) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (cached_models.has_value()) {
    DeliverResult(std::move(*cached_models));
    return;
  }

  fetcher_.FetchModels(endpoint_url_,
                       base::BindOnce(&RemoteModelsProvider::OnFetchComplete,
                                      weak_ptr_factory_.GetWeakPtr()));
}

void RemoteModelsProvider::OnFetchComplete(
    std::vector<mojom::ModelPtr> models) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!models.empty()) {
    // Clone for cache; deliver originals to callers.
    std::vector<mojom::ModelPtr> to_cache;
    to_cache.reserve(models.size());
    for (const auto& m : models) {
      to_cache.push_back(m.Clone());
    }
    cache_.Save(std::move(to_cache), base::DoNothing());
  }

  DeliverResult(std::move(models));
}

void RemoteModelsProvider::DeliverResult(std::vector<mojom::ModelPtr> models) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::vector<GetModelsCallback> callbacks;
  callbacks.swap(pending_callbacks_);

  // All callbacks except the last receive a clone; the last takes ownership.
  for (size_t i = 0; i + 1 < callbacks.size(); ++i) {
    std::vector<mojom::ModelPtr> copy;
    copy.reserve(models.size());
    for (const auto& m : models) {
      copy.push_back(m.Clone());
    }
    std::move(callbacks[i]).Run(std::move(copy));
  }
  if (!callbacks.empty()) {
    std::move(callbacks.back()).Run(std::move(models));
  }
}

}  // namespace ai_chat
