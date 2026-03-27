// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/core/local_ai_service.h"

#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace local_ai {

namespace {

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
    : LocalAIServiceBase(std::move(factory),
                         /*models_already_available=*/false),
      updater_state_(updater_state) {
  CHECK(base::FeatureList::IsEnabled(features::kBraveHistoryEmbeddings));
  CHECK(updater_state_);
  DVLOG(3) << "LocalAIService created";

  // If models are already installed, signal the barrier
  // so only factory registration is needed.
  if (!updater_state_->GetInstallDir().empty()) {
    SignalModelsAvailable();
  }
  updater_state_->AddObserver(this);
}

LocalAIService::~LocalAIService() = default;

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
  factory_.Bind(std::move(factory));
  HandleFactoryRegistered();
}

void LocalAIService::GetPassageEmbedder(GetPassageEmbedderCallback callback) {
  MaybeCreateBWC();
  if (ReadyToServe()) {
    BindPassageEmbedder(std::move(callback));
    return;
  }
  auto [ready_cb, cancel_cb] = base::SplitOnceCallback(std::move(callback));
  QueueConsumer(
      base::BindOnce(&LocalAIService::BindPassageEmbedder,
                     weak_ptr_factory_.GetWeakPtr(), std::move(ready_cb)),
      base::BindOnce(std::move(cancel_cb),
                     mojo::PendingRemote<mojom::PassageEmbedder>()));
}

void LocalAIService::NotifyPassageEmbedderIdle() {
  DVLOG(3) << "LocalAIService: "
              "All PassageEmbedder receivers disconnected";
  NotifyIdle();
}

void LocalAIService::OnLocalModelsReady(const base::FilePath& install_dir) {
  DVLOG(3) << "LocalAIService: Local models ready at: " << install_dir;
  SignalModelsAvailable();
}

void LocalAIService::LoadModelFiles(base::OnceCallback<void(bool)> on_done) {
  const auto& model_dir = updater_state_->GetEmbeddingGemmaModelDir();

  if (model_dir.empty()) {
    DVLOG(0) << "LocalAIService: "
                "Model directory not set";
    std::move(on_done).Run(false);
    return;
  }

  base::FilePath weights = updater_state_->GetEmbeddingGemmaModel();
  base::FilePath dense1 = updater_state_->GetEmbeddingGemmaDense1();
  base::FilePath dense2 = updater_state_->GetEmbeddingGemmaDense2();
  base::FilePath tokenizer = updater_state_->GetEmbeddingGemmaTokenizer();
  base::FilePath config = updater_state_->GetEmbeddingGemmaConfig();

  DVLOG(1) << "Loading model files:";
  DVLOG(1) << "Weights: " << weights;
  DVLOG(1) << "Weights Dense1: " << dense1;
  DVLOG(1) << "Weights Dense2: " << dense2;
  DVLOG(1) << "Tokenizer: " << tokenizer;
  DVLOG(1) << "Config: " << config;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&LoadLocalModelFilesFromDisk, weights, dense1, dense2,
                     tokenizer, config),
      base::BindOnce(&LocalAIService::OnFilesLoaded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(on_done)));
}

void LocalAIService::OnFilesLoaded(base::OnceCallback<void(bool)> on_done,
                                   mojom::ModelFilesPtr model_files) {
  DVLOG(3) << "LocalAIService::OnFilesLoaded";
  loaded_model_files_ = std::move(model_files);
  std::move(on_done).Run(!!loaded_model_files_);
}

void LocalAIService::InitModelViaFactory(
    base::OnceCallback<void(bool)> on_complete) {
  factory_->Init(std::move(loaded_model_files_), std::move(on_complete));
}

bool LocalAIService::IsFactoryBound() const {
  return factory_.is_bound();
}

void LocalAIService::ResetFactory() {
  factory_.reset();
}

void LocalAIService::SetFactoryDisconnectHandler(base::OnceClosure handler) {
  factory_.set_disconnect_handler(std::move(handler));
}

void LocalAIService::OnShutdownExtra() {
  receivers_.Clear();
  weak_ptr_factory_.InvalidateWeakPtrs();
  updater_state_->RemoveObserver(this);
}

void LocalAIService::BindPassageEmbedder(GetPassageEmbedderCallback callback) {
  mojo::PendingRemote<mojom::PassageEmbedder> remote;
  factory_->Bind(remote.InitWithNewPipeAndPassReceiver());
  std::move(callback).Run(std::move(remote));
}

}  // namespace local_ai
