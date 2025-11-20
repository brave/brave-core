// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/core/local_ai_service.h"

#include <utility>

#include "base/barrier_closure.h"
#include "base/check.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace local_ai {

namespace {

// Reads a file directly into BigBuffer's internal storage. For
// files >64KB BigBuffer uses shared memory — the file data goes
// straight into that allocation, avoiding a separate heap copy.
std::optional<mojo_base::BigBuffer> ReadFileToBigBuffer(
    const base::FilePath& path) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    DVLOG(0) << "Failed to open: " << path;
    return std::nullopt;
  }

  int64_t size = file.GetLength();
  if (size <= 0) {
    DVLOG(0) << "Empty or unreadable: " << path;
    return std::nullopt;
  }

  mojo_base::BigBuffer buffer(static_cast<size_t>(size));
  if (!file.ReadAndCheck(0, base::span<uint8_t>(buffer))) {
    DVLOG(0) << "Failed to read: " << path;
    return std::nullopt;
  }

  DVLOG(1) << "Loaded " << path.BaseName() << ", size: " << size;
  return buffer;
}

mojom::ModelFilesPtr LoadLocalModelFilesFromDisk(
    const base::FilePath& weights_path,
    const base::FilePath& weights_dense1_path,
    const base::FilePath& weights_dense2_path,
    const base::FilePath& tokenizer_path,
    const base::FilePath& config_path) {
  auto weights = ReadFileToBigBuffer(weights_path);
  if (!weights) {
    return nullptr;
  }

  auto weights_dense1 = ReadFileToBigBuffer(weights_dense1_path);
  if (!weights_dense1) {
    return nullptr;
  }

  auto weights_dense2 = ReadFileToBigBuffer(weights_dense2_path);
  if (!weights_dense2) {
    return nullptr;
  }

  auto tokenizer = ReadFileToBigBuffer(tokenizer_path);
  if (!tokenizer) {
    return nullptr;
  }

  auto config = ReadFileToBigBuffer(config_path);
  if (!config) {
    return nullptr;
  }

  auto model_files = mojom::ModelFiles::New();
  model_files->weights = std::move(*weights);
  model_files->weights_dense1 = std::move(*weights_dense1);
  model_files->weights_dense2 = std::move(*weights_dense2);
  model_files->tokenizer = std::move(*tokenizer);
  model_files->config = std::move(*config);

  return model_files;
}

}  // namespace

LocalAIService::LocalAIService(BackgroundWebContentsFactory factory,
                               LocalModelsUpdaterState* updater_state)
    : background_web_contents_factory_(std::move(factory)),
      updater_state_(updater_state) {
  CHECK(base::FeatureList::IsEnabled(features::kLocalAIModels));
  CHECK(updater_state_);
  DVLOG(3) << "LocalAIService created";

  MaybeWaitForLocalModelFilesReady();
  updater_state_->AddObserver(this);
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

  DVLOG(3) << "LocalAIService: WASM factory registered";
  if (model_load_barrier_) {
    model_load_barrier_.Run();
  }
}

void LocalAIService::GetPassageEmbedder(GetPassageEmbedderCallback callback) {
  MaybeCreateBackgroundContents();
  if (model_ready_ && factory_.is_bound()) {
    BindPassageEmbedder(std::move(callback));
  } else {
    pending_embedder_callbacks_.push_back(std::move(callback));
  }
}

void LocalAIService::NotifyPassageEmbedderIdle() {
  DVLOG(3) << "LocalAIService: All PassageEmbedder receivers disconnected";
  CloseBackgroundContents();
}

void LocalAIService::OnLocalModelsReady(const base::FilePath& install_dir) {
  DVLOG(3) << "LocalAIService: Local models ready at: " << install_dir;
  if (model_load_barrier_) {
    model_load_barrier_.Run();
  }
}

void LocalAIService::MaybeWaitForLocalModelFilesReady() {
  model_load_barrier_ = base::BarrierClosure(
      2, base::BindOnce(&LocalAIService::LoadLocalModelFiles,
                        weak_ptr_factory_.GetWeakPtr()));

  // If models are already installed, signal the barrier immediately
  // so only the factory registration is needed to trigger loading.
  if (!updater_state_->GetInstallDir().empty()) {
    model_load_barrier_.Run();
  }
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
  updater_state_->RemoveObserver(this);
  CloseBackgroundContents();
}

void LocalAIService::MaybeCreateBackgroundContents() {
  if (background_web_contents_) {
    return;
  }

  DVLOG(3) << "LocalAIService: Creating background contents";
  background_web_contents_ = background_web_contents_factory_.Run(this);
}

void LocalAIService::LoadLocalModelFiles() {
  // Get model file paths from LocalModelsUpdaterState
  base::FilePath weights_path = updater_state_->GetEmbeddingGemmaModel();
  base::FilePath weights_dense1_path =
      updater_state_->GetEmbeddingGemmaDense1();
  base::FilePath weights_dense2_path =
      updater_state_->GetEmbeddingGemmaDense2();
  base::FilePath tokenizer_path = updater_state_->GetEmbeddingGemmaTokenizer();
  base::FilePath config_path = updater_state_->GetEmbeddingGemmaConfig();

  const base::FilePath& model_dir = updater_state_->GetEmbeddingGemmaModelDir();

  if (model_dir.empty()) {
    DVLOG(0) << "LocalAIService: Model directory not set "
                "in updater state";
    OnPassageEmbedderReady(false);
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
      base::BindOnce(&LoadLocalModelFilesFromDisk, weights_path,
                     weights_dense1_path, weights_dense2_path, tokenizer_path,
                     config_path),
      base::BindOnce(&LocalAIService::OnLocalModelFilesLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void LocalAIService::OnLocalModelFilesLoaded(mojom::ModelFilesPtr model_files) {
  DVLOG(3) << "LocalAIService::OnLocalModelFilesLoaded called";

  if (!model_files) {
    DVLOG(0) << "Failed to load model files from disk";
    OnPassageEmbedderReady(false);
    return;
  }

  if (!factory_.is_bound()) {
    DVLOG(0) << "Factory gone before model files loaded";
    OnPassageEmbedderReady(false);
    return;
  }

  DVLOG(3) << "Sending model files to factory via Init...";
  factory_->Init(std::move(model_files),
                 base::BindOnce(&LocalAIService::OnPassageEmbedderReady,
                                weak_ptr_factory_.GetWeakPtr()));
}

void LocalAIService::OnPassageEmbedderReady(bool success) {
  DVLOG(3) << "LocalAIService::OnPassageEmbedderReady called "
              "with success="
           << success;

  if (success) {
    DVLOG(3) << "LocalAIService: Model loaded successfully!";
    model_ready_ = true;
    ProcessPendingCallbacks();
  } else {
    DVLOG(0) << "LocalAIService: Failed to load model. "
                "History embeddings will not work.";
    CloseBackgroundContents();
  }
}

void LocalAIService::CloseBackgroundContents() {
  DVLOG(3) << "LocalAIService: Closing background contents to free memory";
  factory_.reset();
  CancelPendingCallbacks();
  background_web_contents_.reset();
  model_ready_ = false;
  MaybeWaitForLocalModelFilesReady();
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
  model_ready_ = false;
  CancelPendingCallbacks();
  MaybeWaitForLocalModelFilesReady();
}

void LocalAIService::CancelPendingCallbacks() {
  auto callbacks = std::move(pending_embedder_callbacks_);
  for (auto& cb : callbacks) {
    std::move(cb).Run(mojo::PendingRemote<mojom::PassageEmbedder>());
  }
}

}  // namespace local_ai
