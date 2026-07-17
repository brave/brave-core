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
  DCHECK(!pending_callback_) << "GetModels called while a request is in flight";

  pending_callback_ = std::move(callback);

  cache_.Load(base::BindOnce(&RemoteModelsProvider::OnCacheLoaded,
                             weak_ptr_factory_.GetWeakPtr()));
}

void RemoteModelsProvider::OnCacheLoaded(
    std::optional<std::vector<mojom::ModelPtr>> cached_models) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (cached_models.has_value()) {
    std::move(pending_callback_).Run(std::move(*cached_models));
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
    std::vector<mojom::ModelPtr> to_cache;
    to_cache.reserve(models.size());
    for (const auto& m : models) {
      to_cache.push_back(m.Clone());
    }
    cache_.Save(std::move(to_cache), base::DoNothing());
  }

  std::move(pending_callback_).Run(std::move(models));
}

}  // namespace ai_chat
