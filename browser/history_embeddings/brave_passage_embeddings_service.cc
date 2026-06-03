// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service.h"

#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "components/history_embeddings/core/history_embeddings_features.h"

namespace passage_embeddings {

BravePassageEmbeddingsService::BravePassageEmbeddingsService(
    BackgroundWebContentsFactory background_web_contents_factory)
    : background_web_contents_factory_(
          std::move(background_web_contents_factory)) {
  CHECK(base::FeatureList::IsEnabled(history_embeddings::kHistoryEmbeddings));
}

BravePassageEmbeddingsService::~BravePassageEmbeddingsService() = default;

void BravePassageEmbeddingsService::BindLocalAIReceiver(
    mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver) {
  if (batch_embedder_) {
    batch_embedder_->BindLocalAIReceiver(std::move(receiver));
  }
}

void BravePassageEmbeddingsService::BindPassageEmbedder(
    mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
    local_ai::mojom::ModelFilesPtr model_files,
    base::OnceCallback<void(bool)> callback) {
  CHECK(model_files);
  // Upstream's controller binds one embedder at a time — the base class
  // gates on `!embedder_remote_`. If we see a second Bind while a load
  // is in flight or an embedder is already active, fail the extra
  // request defensively.
  if (batch_embedder_) {
    DVLOG(1) << "BindPassageEmbedder called while a BatchEmbedder is already "
                "bound; failing";
    std::move(callback).Run(false);
    return;
  }

  batch_embedder_ = std::make_unique<BraveBatchPassageEmbedder>(
      std::move(receiver), background_web_contents_factory_,
      std::move(model_files), std::move(callback),
      base::BindOnce(
          &BravePassageEmbeddingsService::OnBatchEmbedderDisconnected,
          weak_ptr_factory_.GetWeakPtr()));
}

void BravePassageEmbeddingsService::LoadModels(
    mojom::PassageEmbeddingsLoadModelsParamsPtr model_params,
    mojom::PassageEmbedderParamsPtr params,
    mojo::PendingReceiver<mojom::PassageEmbedder> model,
    LoadModelsCallback callback) {
  // The upstream mojom is shaped for tflite + sentencepiece; Brave's
  // model files are not carried in this struct. The controller calls
  // BindPassageEmbedder directly and never binds service_remote_, so
  // this override is unreachable in practice. Fail defensively.
  DVLOG(1) << "LoadModels called on BravePassageEmbeddingsService; "
              "this path is unused. Failing.";
  std::move(callback).Run(false);
}

void BravePassageEmbeddingsService::OnBatchEmbedderDisconnected() {
  DVLOG(3) << "BraveBatchPassageEmbedder disconnected; tearing down";
  batch_embedder_.reset();
}

}  // namespace passage_embeddings
