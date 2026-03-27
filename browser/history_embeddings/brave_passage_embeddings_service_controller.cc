// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"

#include <utility>

#include "base/logging.h"
#include "brave/browser/history_embeddings/brave_embedder.h"

namespace passage_embeddings {

// EmbedderProxy implementation

BravePassageEmbeddingsServiceController::EmbedderProxy::EmbedderProxy() =
    default;
BravePassageEmbeddingsServiceController::EmbedderProxy::~EmbedderProxy() =
    default;

void BravePassageEmbeddingsServiceController::EmbedderProxy::SetTarget(
    Embedder* target) {
  target_ = target;
}

Embedder::TaskId BravePassageEmbeddingsServiceController::EmbedderProxy::
    ComputePassagesEmbeddings(PassagePriority priority,
                              std::vector<std::string> passages,
                              ComputePassagesEmbeddingsCallback callback) {
  if (target_) {
    return target_->ComputePassagesEmbeddings(priority, std::move(passages),
                                              std::move(callback));
  }
  auto task_id = next_task_id_++;
  std::move(callback).Run(std::move(passages), {}, task_id,
                          ComputeEmbeddingsStatus::kModelUnavailable);
  return task_id;
}

void BravePassageEmbeddingsServiceController::EmbedderProxy::ReprioritizeTasks(
    PassagePriority priority,
    const std::set<TaskId>& tasks) {
  if (target_) {
    target_->ReprioritizeTasks(priority, tasks);
  }
}

bool BravePassageEmbeddingsServiceController::EmbedderProxy::TryCancel(
    TaskId task_id) {
  if (target_) {
    return target_->TryCancel(task_id);
  }
  return false;
}

// BravePassageEmbeddingsServiceController implementation

// static
BravePassageEmbeddingsServiceController*
BravePassageEmbeddingsServiceController::Get() {
  static base::NoDestructor<BravePassageEmbeddingsServiceController> instance;
  return instance.get();
}

BravePassageEmbeddingsServiceController::
    BravePassageEmbeddingsServiceController() = default;

BravePassageEmbeddingsServiceController::
    ~BravePassageEmbeddingsServiceController() = default;

Embedder* BravePassageEmbeddingsServiceController::GetBraveEmbedder() {
  // Always return the proxy — it's never null. The proxy delegates to the
  // real BraveEmbedder once SetLocalAIServiceRemote() has been called.
  return &embedder_proxy_;
}

void BravePassageEmbeddingsServiceController::SetLocalAIServiceRemote(
    mojo::PendingRemote<local_ai::mojom::LocalAIService> remote) {
  embedder_ = std::make_unique<BraveEmbedder>(std::move(remote));
  embedder_->AddObserver(this);
  embedder_proxy_.SetTarget(embedder_.get());
}

void BravePassageEmbeddingsServiceController::OnEmbedderIdle() {
  if (embedder_) {
    embedder_->NotifyServiceIdle();
  }
}

void BravePassageEmbeddingsServiceController::MaybeLaunchService() {
  // No-op: BraveEmbedder is created lazily in GetBraveEmbedder().
  DVLOG(3) << "MaybeLaunchService called (no-op for BraveEmbedder)";
}

void BravePassageEmbeddingsServiceController::ResetServiceRemote() {
  // No-op: We don't use a separate service remote.
  DVLOG(3) << "ResetServiceRemote called (no-op for BraveEmbedder)";
}

bool BravePassageEmbeddingsServiceController::EmbedderReady() {
  // We're always ready since we use LocalAIService instead of model files
  return true;
}

EmbedderMetadata
BravePassageEmbeddingsServiceController::GetEmbedderMetadata() {
  // Return metadata for EmbeddingGemma model
  // Version 1, 768-dimensional output, 0.45 search threshold
  return EmbedderMetadata(
      /*model_version=*/1,
      /*output_size=*/768,
      /*search_score_threshold=*/0.45);
}

void BravePassageEmbeddingsServiceController::GetEmbeddings(
    std::vector<std::string> passages,
    PassagePriority priority,
    GetEmbeddingsResultCallback callback) {
  // This method is part of the base class interface but not used in our
  // implementation. BraveEmbedder is accessed directly via GetBraveEmbedder()
  // and HistoryEmbeddingsService calls methods on the embedder directly.
  DVLOG(1) << "GetEmbeddings called unexpectedly on BravePassage"
              "EmbeddingsServiceController";
  std::move(callback).Run({}, ComputeEmbeddingsStatus::kExecutionFailure);
}

}  // namespace passage_embeddings
