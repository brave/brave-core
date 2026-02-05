// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/core/local_ai_service.h"

#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace local_ai {

namespace {

mojom::ModelFilesPtr LoadModelFilesFromDisk(
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

  auto model_files = mojom::ModelFiles::New();
  model_files->weights = mojo_base::BigBuffer(std::move(*weights_opt));
  model_files->weights_dense1 =
      mojo_base::BigBuffer(std::move(*weights_dense1_opt));
  model_files->weights_dense2 =
      mojo_base::BigBuffer(std::move(*weights_dense2_opt));
  model_files->tokenizer = mojo_base::BigBuffer(std::move(*tokenizer_opt));
  model_files->config = mojo_base::BigBuffer(std::move(*config_opt));

  return model_files;
}

}  // namespace

LocalAIService::LocalAIService(BackgroundWebContentsFactory factory)
    : background_web_contents_factory_(std::move(factory)) {
  CHECK(base::FeatureList::IsEnabled(features::kLocalAIModels));
  DVLOG(3) << "LocalAIService created";

  // Observe the component updater for model readiness
  LocalModelsUpdaterState::GetInstance()->AddObserver(this);
}

LocalAIService::~LocalAIService() {
  CloseBackgroundContents();
}

mojo::PendingRemote<mojom::LocalAIService> LocalAIService::MakeRemote() {
  mojo::PendingRemote<mojom::LocalAIService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void LocalAIService::Bind(
    mojo::PendingReceiver<mojom::LocalAIService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void LocalAIService::RegisterPassageEmbedderFactory(
    mojo::PendingRemote<mojom::PassageEmbedderFactory> factory) {
  if (!background_web_contents_) {
    DVLOG(1) << "RegisterPassageEmbedderFactory: No background contents";
    return;
  }
  factory_.Bind(std::move(factory));
  factory_.set_disconnect_handler(base::BindOnce(
      &LocalAIService::OnFactoryDisconnected, weak_ptr_factory_.GetWeakPtr()));

  // The WASM page has loaded and registered its factory
  wasm_page_loaded_ = true;
  TryLoadModel();
}

void LocalAIService::GetPassageEmbedder(GetPassageEmbedderCallback callback) {
  MaybeCreateBackgroundContents();
  if (model_initialized_ && factory_.is_bound()) {
    BindPassageEmbedder(std::move(callback));
  } else {
    pending_embedder_callbacks_.push_back(std::move(callback));
  }
}

void LocalAIService::NotifyPassageEmbedderIdle() {
  DVLOG(3) << "LocalAIService: PassageEmbedder idle";
  CloseBackgroundContents();
}

void LocalAIService::OnLocalModelsReady(const base::FilePath& install_dir) {
  DVLOG(3) << "LocalAIService: Local models ready at: " << install_dir;
  models_ready_ = true;
  TryLoadModel();
}

void LocalAIService::TryLoadModel() {
  DVLOG(3) << "LocalAIService::TryLoadModel"
           << " - wasm_page_loaded_=" << wasm_page_loaded_
           << ", models_ready_=" << models_ready_
           << ", model_initialized_=" << model_initialized_;

  if (!wasm_page_loaded_) {
    DVLOG(3) << "LocalAIService: Waiting for WASM page to load...";
    return;
  }

  if (!models_ready_) {
    DVLOG(3) << "LocalAIService: Waiting for models to be ready...";
    return;
  }

  if (model_initialized_) {
    DVLOG(3) << "LocalAIService: Model already initialized";
    return;
  }

  if (!factory_.is_bound()) {
    DVLOG(1) << "LocalAIService: No factory for model loading";
    return;
  }

  DVLOG(3) << "LocalAIService: Both WASM and component ready, "
              "loading model...";
  LoadModelFiles();
}

void LocalAIService::OnBackgroundContentsDestroyed(
    BackgroundWebContents::DestroyReason reason) {
  DVLOG(1) << "LocalAIService: Background contents destroyed";
  CloseBackgroundContents();
}

void LocalAIService::Shutdown() {
  DVLOG(3) << "LocalAIService: Shutting down";
  receivers_.Clear();
  weak_ptr_factory_.InvalidateWeakPtrs();
  LocalModelsUpdaterState::GetInstance()->RemoveObserver(this);
  CloseBackgroundContents();
}

void LocalAIService::MaybeCreateBackgroundContents() {
  if (background_web_contents_) {
    return;
  }

  DVLOG(3) << "LocalAIService: Creating background contents";
  background_web_contents_ = background_web_contents_factory_.Run(this);
}

void LocalAIService::LoadModelFiles() {
  // Get model file paths from LocalModelsUpdaterState
  auto* state = LocalModelsUpdaterState::GetInstance();
  base::FilePath weights_path = state->GetEmbeddingGemmaModel();
  base::FilePath weights_dense1_path = state->GetEmbeddingGemmaDense1();
  base::FilePath weights_dense2_path = state->GetEmbeddingGemmaDense2();
  base::FilePath tokenizer_path = state->GetEmbeddingGemmaTokenizer();
  base::FilePath config_path = state->GetEmbeddingGemmaConfig();

  const base::FilePath& model_dir = state->GetEmbeddingGemmaModelDir();

  if (model_dir.empty()) {
    DVLOG(0) << "LocalAIService: Model directory not set "
                "in updater state";
    OnModelInitialized(false);
    return;
  }

  DVLOG(1) << "Loading model files:";
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
      base::BindOnce(&LoadModelFilesFromDisk, weights_path, weights_dense1_path,
                     weights_dense2_path, tokenizer_path, config_path),
      base::BindOnce(&LocalAIService::OnModelFilesLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void LocalAIService::OnModelFilesLoaded(mojom::ModelFilesPtr model_files) {
  DVLOG(3) << "LocalAIService::OnModelFilesLoaded called";

  if (!model_files) {
    DVLOG(0) << "Failed to load model files from disk";
    OnModelInitialized(false);
    return;
  }

  if (!factory_.is_bound()) {
    DVLOG(0) << "Factory gone before model files loaded";
    OnModelInitialized(false);
    return;
  }

  DVLOG(3) << "Sending model files to factory via Init...";
  factory_->Init(std::move(model_files),
                 base::BindOnce(&LocalAIService::OnModelInitialized,
                                weak_ptr_factory_.GetWeakPtr()));
}

void LocalAIService::OnModelInitialized(bool success) {
  DVLOG(3) << "LocalAIService::OnModelInitialized called "
              "with success="
           << success;

  if (success) {
    DVLOG(3) << "LocalAIService: Model loaded successfully!";
    model_initialized_ = true;
    ProcessPendingCallbacks();
  } else {
    DVLOG(0) << "LocalAIService: Failed to load model. "
                "History embeddings will not work.";
    model_initialized_ = false;
    CancelPendingCallbacks();
  }
}

void LocalAIService::CloseBackgroundContents() {
  DVLOG(3) << "LocalAIService: Closing background contents to free memory";
  factory_.reset();
  CancelPendingCallbacks();
  background_web_contents_.reset();
  wasm_page_loaded_ = false;
  model_initialized_ = false;
}

void LocalAIService::BindPassageEmbedder(GetPassageEmbedderCallback callback) {
  mojo::PendingRemote<mojom::PassageEmbedder> remote;
  factory_->Bind(remote.InitWithNewPipeAndPassReceiver());
  std::move(callback).Run(std::move(remote));
}

void LocalAIService::ProcessPendingCallbacks() {
  auto callbacks = std::move(pending_embedder_callbacks_);
  for (auto& cb : callbacks) {
    BindPassageEmbedder(std::move(cb));
  }
}

void LocalAIService::OnFactoryDisconnected() {
  factory_.reset();
  wasm_page_loaded_ = false;
  model_initialized_ = false;
  CancelPendingCallbacks();
}

void LocalAIService::CancelPendingCallbacks() {
  auto callbacks = std::move(pending_embedder_callbacks_);
  for (auto& cb : callbacks) {
    std::move(cb).Run(mojo::PendingRemote<mojom::PassageEmbedder>());
  }
}

}  // namespace local_ai
