// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/candle_service.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/local_ai/browser/local_models_updater.h"

namespace local_ai {

CandleService::PendingEmbedRequest::PendingEmbedRequest() = default;
CandleService::PendingEmbedRequest::PendingEmbedRequest(std::string text,
                                                        EmbedCallback callback)
    : text(std::move(text)), callback(std::move(callback)) {}
CandleService::PendingEmbedRequest::~PendingEmbedRequest() = default;
CandleService::PendingEmbedRequest::PendingEmbedRequest(PendingEmbedRequest&&) =
    default;
CandleService::PendingEmbedRequest&
CandleService::PendingEmbedRequest::operator=(PendingEmbedRequest&&) = default;

namespace {

struct ModelFiles {
  std::vector<uint8_t> weights;
  std::vector<uint8_t> weights_dense1;
  std::vector<uint8_t> weights_dense2;
  std::vector<uint8_t> tokenizer;
  std::vector<uint8_t> config;
};

std::unique_ptr<ModelFiles> LoadEmbeddingGemmaModelFilesFromDisk(
    const base::FilePath& weights_path,
    const base::FilePath& weights_dense1_path,
    const base::FilePath& weights_dense2_path,
    const base::FilePath& tokenizer_path,
    const base::FilePath& config_path) {
  auto weights_opt = base::ReadFileToBytes(weights_path);
  if (!weights_opt) {
    DVLOG(0) << "Failed to read model weights from: " << weights_path;
    return nullptr;
  }

  auto weights_dense1_opt = base::ReadFileToBytes(weights_dense1_path);
  if (!weights_dense1_opt) {
    DVLOG(0) << "Failed to read dense1 weights from: " << weights_dense1_path;
    return nullptr;
  }

  auto weights_dense2_opt = base::ReadFileToBytes(weights_dense2_path);
  if (!weights_dense2_opt) {
    DVLOG(0) << "Failed to read dense2 weights from: " << weights_dense2_path;
    return nullptr;
  }

  auto tokenizer_opt = base::ReadFileToBytes(tokenizer_path);
  if (!tokenizer_opt) {
    DVLOG(0) << "Failed to read tokenizer from: " << tokenizer_path;
    return nullptr;
  }

  auto config_opt = base::ReadFileToBytes(config_path);
  if (!config_opt) {
    DVLOG(0) << "Failed to read config from: " << config_path;
    return nullptr;
  }

  DVLOG(1) << "Loaded weights, size: " << weights_opt->size();
  DVLOG(1) << "Loaded weights_dense1, size: " << weights_dense1_opt->size();
  DVLOG(1) << "Loaded weights_dense2, size: " << weights_dense2_opt->size();
  DVLOG(1) << "Loaded tokenizer, size: " << tokenizer_opt->size();
  DVLOG(1) << "Loaded config, size: " << config_opt->size();

  auto model_files = std::make_unique<ModelFiles>();
  model_files->weights = std::move(*weights_opt);
  model_files->weights_dense1 = std::move(*weights_dense1_opt);
  model_files->weights_dense2 = std::move(*weights_dense2_opt);
  model_files->tokenizer = std::move(*tokenizer_opt);
  model_files->config = std::move(*config_opt);

  return model_files;
}

}  // namespace

CandleService::CandleService() {
  DVLOG(3) << "CandleService created";

  // Observe the component updater for model readiness
  LocalModelsUpdaterState::GetInstance()->AddObserver(this);

  // Check if model is already ready
  const base::FilePath& model_dir =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaModelDir();
  DVLOG(1) << "CandleService: model_dir at construction = " << model_dir;
  if (!model_dir.empty()) {
    DVLOG(1) << "CandleService: model_dir is set, calling OnComponentReady";
    OnComponentReady(model_dir);
  } else {
    DVLOG(1) << "CandleService: model_dir is empty, waiting for component";
  }
}

CandleService::~CandleService() {
  LocalModelsUpdaterState::GetInstance()->RemoveObserver(this);

  // Ensure embedder is deleted on thread pool to avoid blocking
  // (defensive - Shutdown() should have already done this)
  CandleEmbedder::DeleteOnThreadPool(std::move(embedder_));
}

void CandleService::LoadModelFiles() {
  if (model_initialized_ || embedder_ || initialization_in_progress_) {
    DVLOG(3) << "Model already initialized or loading";
    return;
  }

  initialization_in_progress_ = true;

  // Get model file paths from LocalModelsUpdaterState
  base::FilePath weights_path =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaModel();
  base::FilePath weights_dense1_path =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaDense1();
  base::FilePath weights_dense2_path =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaDense2();
  base::FilePath tokenizer_path =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaTokenizer();
  base::FilePath config_path =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaConfig();

  const base::FilePath& model_dir =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaModelDir();

  if (model_dir.empty()) {
    DVLOG(0) << "CandleService: Model directory not set in updater state";
    initialization_in_progress_ = false;
    return;
  }

  DVLOG(1) << "Loading Embedding Gemma model files (attempt "
           << (model_load_retry_count_ + 1) << "/" << kMaxModelLoadRetries
           << "):";
  DVLOG(1) << "Weights: " << weights_path;
  DVLOG(1) << "Weights Dense1: " << weights_dense1_path;
  DVLOG(1) << "Weights Dense2: " << weights_dense2_path;
  DVLOG(1) << "Tokenizer: " << tokenizer_path;
  DVLOG(1) << "Config: " << config_path;

  // Load model files on a background thread to avoid blocking
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&LoadEmbeddingGemmaModelFilesFromDisk, weights_path,
                     weights_dense1_path, weights_dense2_path, tokenizer_path,
                     config_path),
      base::BindOnce(
          [](base::WeakPtr<CandleService> service,
             std::unique_ptr<ModelFiles> model_files) {
            if (!service) {
              return;
            }

            if (!model_files) {
              DVLOG(0) << "Failed to load model files from disk";
              service->initialization_in_progress_ = false;
              service->model_load_retry_count_++;
              if (service->model_load_retry_count_ <
                  CandleService::kMaxModelLoadRetries) {
                DVLOG(1) << "Retrying model load in 1 second...";
                base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
                    FROM_HERE,
                    base::BindOnce(&CandleService::LoadModelFiles, service),
                    base::Seconds(1));
              }
              return;
            }

            DVLOG(3) << "Model files loaded, initializing embedder...";
            CandleEmbedder::Create(
                std::move(model_files->weights),
                std::move(model_files->weights_dense1),
                std::move(model_files->weights_dense2),
                std::move(model_files->tokenizer),
                std::move(model_files->config),
                base::BindOnce(
                    [](base::WeakPtr<CandleService> svc,
                       std::unique_ptr<CandleEmbedder> embedder,
                       const std::string& error_message) {
                      if (!svc) {
                        return;
                      }
                      svc->OnModelInitialized(std::move(embedder),
                                              error_message);
                    },
                    service));
          },
          weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::Embed(const std::string& text, EmbedCallback callback) {
  if (!embedder_ || !model_initialized_) {
    DVLOG(3) << "Model not initialized yet, queuing embed request";
    pending_embed_requests_.emplace_back(text, std::move(callback));
    return;
  }

  embedder_->Embed(text, std::move(callback));
}

void CandleService::OnModelInitialized(std::unique_ptr<CandleEmbedder> embedder,
                                       const std::string& error_message) {
  initialization_in_progress_ = false;

  if (embedder) {
    DVLOG(3) << "CandleService: EmbeddingGemma model loaded successfully! "
             << "History embeddings are now ready.";
    model_load_retry_count_ = 0;
    model_initialized_ = true;

    // Safely delete any existing embedder on thread pool (defensive)
    if (embedder_) {
      CandleEmbedder::DeleteOnThreadPool(std::move(embedder_));
    }
    embedder_ = std::move(embedder);

    // Process any queued embed requests
    ProcessPendingEmbedRequests();
  } else {
    DVLOG(0) << "CandleService: Model initialization failed: " << error_message;
    model_load_retry_count_++;

    if (model_load_retry_count_ < kMaxModelLoadRetries) {
      DVLOG(1) << "Retrying model load in 1 second...";
      base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&CandleService::LoadModelFiles,
                         weak_ptr_factory_.GetWeakPtr()),
          base::Seconds(1));
    } else {
      DVLOG(0) << "CandleService: Failed to load model after "
               << kMaxModelLoadRetries << " attempts. "
               << "History embeddings will not work.";
      // Clear pending requests
      for (auto& request : pending_embed_requests_) {
        std::vector<float> empty;
        std::move(request.callback).Run(empty);
      }
      pending_embed_requests_.clear();
    }
  }
}

void CandleService::OnComponentReady(const base::FilePath& install_dir) {
  DVLOG(3) << "CandleService: Component ready at: " << install_dir;
  component_ready_ = true;

  if (!model_initialized_ && !embedder_) {
    LoadModelFiles();
  }
}

void CandleService::Shutdown() {
  DVLOG(3) << "CandleService: Shutting down";

  // Clear any pending requests
  for (auto& request : pending_embed_requests_) {
    std::vector<float> empty;
    std::move(request.callback).Run(empty);
  }
  pending_embed_requests_.clear();

  // Delete embedder on thread pool to avoid blocking UI thread
  CandleEmbedder::DeleteOnThreadPool(std::move(embedder_));
  model_initialized_ = false;
}

void CandleService::ProcessPendingEmbedRequests() {
  if (!model_initialized_ || !embedder_) {
    return;
  }

  DVLOG(3) << "Processing " << pending_embed_requests_.size()
           << " pending embed requests";

  // Process all queued requests
  for (auto& request : pending_embed_requests_) {
    embedder_->Embed(request.text, std::move(request.callback));
  }
  pending_embed_requests_.clear();
}

}  // namespace local_ai
