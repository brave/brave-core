// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"

#include "base/logging.h"
#include "brave/browser/history_embeddings/brave_embedder.h"
#include "chrome/browser/profiles/profile.h"

namespace passage_embeddings {

// BravePassageEmbeddingsServiceController implementation

// static
BravePassageEmbeddingsServiceController*
BravePassageEmbeddingsServiceController::Get() {
  static base::NoDestructor<BravePassageEmbeddingsServiceController> instance;
  return instance.get();
}

BravePassageEmbeddingsServiceController::
    BravePassageEmbeddingsServiceController() {
  // Embedders are created lazily per-profile in GetBraveEmbedder()
}

BravePassageEmbeddingsServiceController::
    ~BravePassageEmbeddingsServiceController() = default;

Embedder* BravePassageEmbeddingsServiceController::GetBraveEmbedder(
    Profile* profile) {
  if (!profile) {
    DVLOG(1) << "GetBraveEmbedder called with null profile";
    return nullptr;
  }

  // Create embedder lazily for this profile if it doesn't exist
  auto it = profile_embedders_.find(profile);
  if (it == profile_embedders_.end()) {
    DVLOG(3) << "Creating BraveEmbedder for profile " << profile;
    auto embedder = std::make_unique<brave::BraveEmbedder>(profile);
    it = profile_embedders_.emplace(profile, std::move(embedder)).first;
  }

  return it->second.get();
}

void BravePassageEmbeddingsServiceController::MaybeLaunchService() {
  // No-op: BraveEmbedder instances are passed directly to
  // HistoryEmbeddingsService per-profile, so we don't launch a separate
  // service process. This method is required by the base class but not used
  // in our implementation.
  DVLOG(3) << "MaybeLaunchService called (no-op for BraveEmbedder)";
}

void BravePassageEmbeddingsServiceController::ResetServiceRemote() {
  // No-op: We don't use a separate service process. BraveEmbedder instances
  // are used directly per-profile.
  DVLOG(3) << "ResetServiceRemote called (no-op for BraveEmbedder)";
}

bool BravePassageEmbeddingsServiceController::EmbedderReady() {
  // We're always ready since we use CandleService instead of model files
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
  // implementation. BraveEmbedder instances are accessed directly per-profile
  // via GetBraveEmbedder(profile), and HistoryEmbeddingsService calls
  // methods on those embedders directly rather than going through this
  // controller method.
  DVLOG(1) << "GetEmbeddings called unexpectedly on BravePassage"
              "EmbeddingsServiceController";
  std::move(callback).Run({}, ComputeEmbeddingsStatus::kExecutionFailure);
}

}  // namespace passage_embeddings
