// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"

#include "brave/browser/history_embeddings/brave_embedder.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace passage_embeddings {

// BravePassageEmbedderImpl implementation

BravePassageEmbeddingsServiceController::BravePassageEmbedderImpl::
    BravePassageEmbedderImpl(brave::BraveEmbedder* embedder)
    : embedder_(embedder) {}

BravePassageEmbeddingsServiceController::BravePassageEmbedderImpl::
    ~BravePassageEmbedderImpl() = default;

void BravePassageEmbeddingsServiceController::BravePassageEmbedderImpl::
    GenerateEmbeddings(const std::vector<std::string>& passages,
                       mojom::PassagePriority priority,
                       GenerateEmbeddingsCallback callback) {
  LOG(INFO) << "BravePassageEmbedderImpl::GenerateEmbeddings called with "
            << passages.size() << " passages";

  if (!embedder_) {
    LOG(WARNING) << "Embedder is null!";
    // Return empty results on failure
    std::move(callback).Run({});
    return;
  }

  // Convert mojom priority to passage_embeddings priority
  PassagePriority embedder_priority;
  switch (priority) {
    case mojom::PassagePriority::kUrgent:
      embedder_priority = PassagePriority::kUrgent;
      break;
    case mojom::PassagePriority::kUserInitiated:
      embedder_priority = PassagePriority::kUserInitiated;
      break;
    case mojom::PassagePriority::kPassive:
      embedder_priority = PassagePriority::kPassive;
      break;
    default:
      embedder_priority = PassagePriority::kUserInitiated;
      break;
  }

  // Forward to BraveEmbedder
  std::vector<std::string> passages_copy = passages;
  embedder_->ComputePassagesEmbeddings(
      embedder_priority, std::move(passages_copy),
      base::BindOnce(&BravePassageEmbedderImpl::OnEmbeddingsGenerated,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void BravePassageEmbeddingsServiceController::BravePassageEmbedderImpl::
    OnEmbeddingsGenerated(GenerateEmbeddingsCallback callback,
                          std::vector<std::string> passages,
                          std::vector<Embedding> embeddings,
                          Embedder::TaskId task_id,
                          ComputeEmbeddingsStatus status) {
  if (status != ComputeEmbeddingsStatus::kSuccess || embeddings.empty()) {
    // Return empty results on failure
    std::move(callback).Run({});
    return;
  }

  // Convert embeddings to mojom format
  std::vector<mojom::PassageEmbeddingsResultPtr> results;
  results.reserve(embeddings.size());

  for (const auto& embedding : embeddings) {
    auto result = mojom::PassageEmbeddingsResult::New();
    result->embeddings = embedding.GetData();
    results.push_back(std::move(result));
  }

  std::move(callback).Run(std::move(results));
}

// BravePassageEmbeddingsServiceImpl implementation

BravePassageEmbeddingsServiceController::BravePassageEmbeddingsServiceImpl::
    BravePassageEmbeddingsServiceImpl(brave::BraveEmbedder* embedder)
    : embedder_(embedder) {}

BravePassageEmbeddingsServiceController::BravePassageEmbeddingsServiceImpl::
    ~BravePassageEmbeddingsServiceImpl() = default;

void BravePassageEmbeddingsServiceController::BravePassageEmbeddingsServiceImpl::
    LoadModels(mojom::PassageEmbeddingsLoadModelsParamsPtr model_params,
               mojom::PassageEmbedderParamsPtr params,
               mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
               LoadModelsCallback callback) {
  LOG(INFO) << "BravePassageEmbeddingsServiceImpl::LoadModels called";

  // We don't actually load models from disk - BraveEmbedder uses CandleService
  // Just bind the embedder receiver and return success

  if (!embedder_impl_) {
    embedder_impl_ = std::make_unique<BravePassageEmbedderImpl>(embedder_);
  }

  embedder_receiver_ = std::make_unique<mojo::Receiver<mojom::PassageEmbedder>>(
      embedder_impl_.get(), std::move(receiver));

  LOG(INFO) << "LoadModels completed successfully";

  // Report success
  std::move(callback).Run(true);
}

// BravePassageEmbeddingsServiceController implementation

// static
BravePassageEmbeddingsServiceController*
BravePassageEmbeddingsServiceController::Get() {
  static base::NoDestructor<BravePassageEmbeddingsServiceController> instance;
  return instance.get();
}

BravePassageEmbeddingsServiceController::
    BravePassageEmbeddingsServiceController()
    : PassageEmbeddingsServiceController() {
  LOG(INFO) << "BravePassageEmbeddingsServiceController created";

  // Create BraveEmbedder with the last used profile
  // Note: This is a singleton controller, so it will use the last used profile
  Profile* profile = ProfileManager::GetLastUsedProfile();
  if (profile) {
    brave_embedder_ = std::make_unique<brave::BraveEmbedder>(profile);
  } else {
    LOG(ERROR) << "BravePassageEmbeddingsServiceController: No profile "
                  "available";
  }
}

BravePassageEmbeddingsServiceController::
    ~BravePassageEmbeddingsServiceController() = default;

Embedder* BravePassageEmbeddingsServiceController::GetBraveEmbedder() {
  LOG(INFO) << "BravePassageEmbeddingsServiceController::GetBraveEmbedder called";
  return brave_embedder_.get();
}

void BravePassageEmbeddingsServiceController::MaybeLaunchService() {
  LOG(INFO) << "BravePassageEmbeddingsServiceController::MaybeLaunchService called";

  // Check if service is already bound
  if (service_remote_.is_bound()) {
    LOG(INFO) << "Service already bound, returning";
    return;
  }

  // Create the service implementation
  if (!service_impl_) {
    service_impl_ = std::make_unique<BravePassageEmbeddingsServiceImpl>(
        brave_embedder_.get());
  }

  // Bind service_remote_ to our in-process service
  mojo::PendingRemote<mojom::PassageEmbeddingsService> pending_remote;
  auto pending_receiver = pending_remote.InitWithNewPipeAndPassReceiver();

  service_receiver_ = std::make_unique<mojo::Receiver<mojom::PassageEmbeddingsService>>(
      service_impl_.get(), std::move(pending_receiver));

  service_remote_.Bind(std::move(pending_remote));

  LOG(INFO) << "Service bound successfully";
}

void BravePassageEmbeddingsServiceController::ResetServiceRemote() {
  // Reset the Mojo connections
  if (service_remote_.is_bound()) {
    service_remote_.reset();
  }
  if (service_receiver_ && service_receiver_->is_bound()) {
    service_receiver_->reset();
  }
}

bool BravePassageEmbeddingsServiceController::EmbedderReady() {
  // We're always ready since we use CandleService instead of model files
  LOG(INFO) << "EmbedderReady called, returning true";
  return true;
}

EmbedderMetadata BravePassageEmbeddingsServiceController::GetEmbedderMetadata() {
  // Return metadata for EmbeddingGemma model
  // Version 1, 768-dimensional output, 0.75 search threshold
  LOG(INFO) << "GetEmbedderMetadata called";
  return EmbedderMetadata(
      /*model_version=*/1,
      /*output_size=*/768,
      /*search_score_threshold=*/0.65);
}

void BravePassageEmbeddingsServiceController::GetEmbeddings(
    std::vector<std::string> passages,
    PassagePriority priority,
    GetEmbeddingsResultCallback callback) {
  LOG(INFO) << "BravePassageEmbeddingsServiceController::GetEmbeddings called with "
            << passages.size() << " passages";

  if (passages.empty()) {
    LOG(INFO) << "No passages, returning success";
    std::move(callback).Run({}, ComputeEmbeddingsStatus::kSuccess);
    return;
  }

  if (!brave_embedder_) {
    LOG(WARNING) << "BraveEmbedder not initialized!";
    std::move(callback).Run({}, ComputeEmbeddingsStatus::kExecutionFailure);
    return;
  }

  LOG(INFO) << "Calling BraveEmbedder::ComputePassagesEmbeddings";

  // Directly use BraveEmbedder to compute embeddings
  brave_embedder_->ComputePassagesEmbeddings(
      priority, std::move(passages),
      base::BindOnce(
          [](GetEmbeddingsResultCallback callback,
             std::vector<std::string> passages,
             std::vector<Embedding> embeddings,
             Embedder::TaskId task_id,
             ComputeEmbeddingsStatus status) {
            LOG(INFO) << "BraveEmbedder callback: status="
                      << static_cast<int>(status)
                      << ", embeddings.size()=" << embeddings.size();

            if (status != ComputeEmbeddingsStatus::kSuccess || embeddings.empty()) {
              std::move(callback).Run({}, status);
              return;
            }

            // Convert embeddings to mojom format
            std::vector<mojom::PassageEmbeddingsResultPtr> results;
            results.reserve(embeddings.size());

            for (const auto& embedding : embeddings) {
              auto result = mojom::PassageEmbeddingsResult::New();
              result->embeddings = embedding.GetData();
              results.push_back(std::move(result));
            }

            std::move(callback).Run(std::move(results), status);
          },
          std::move(callback)));
}

}  // namespace passage_embeddings
