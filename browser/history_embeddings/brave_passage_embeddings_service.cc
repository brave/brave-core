// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/task/thread_pool.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace passage_embeddings {

namespace {

// Reads a file directly into BigBuffer storage. For files >64KB
// BigBuffer uses shared memory.
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
  return buffer;
}

using BindRegistry =
    base::flat_map<content::WebContents*,
                   BravePassageEmbeddingsService::BindCallback>;

BindRegistry& GetBindRegistry() {
  static base::NoDestructor<BindRegistry> registry;
  return *registry;
}

local_ai::mojom::ModelFilesPtr LoadLocalModelFilesFromDisk(
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
  auto model_files = local_ai::mojom::ModelFiles::New();
  model_files->weights = std::move(*weights);
  model_files->weights_dense1 = std::move(*weights_dense1);
  model_files->weights_dense2 = std::move(*weights_dense2);
  model_files->tokenizer = std::move(*tokenizer);
  model_files->config = std::move(*config);
  return model_files;
}

}  // namespace

BravePassageEmbeddingsService::BravePassageEmbeddingsService(
    BackgroundWebContentsFactory background_web_contents_factory,
    local_ai::LocalModelsUpdaterState* updater_state)
    : background_web_contents_factory_(
          std::move(background_web_contents_factory)),
      updater_state_(updater_state) {
  CHECK(base::FeatureList::IsEnabled(history_embeddings::kHistoryEmbeddings));
  CHECK(updater_state_);
  updater_state_->AddObserver(this);
}

BravePassageEmbeddingsService::~BravePassageEmbeddingsService() {
  updater_state_->RemoveObserver(this);
}

// static
void BravePassageEmbeddingsService::SetBindCallbackForWebContents(
    content::WebContents* web_contents,
    BindCallback callback) {
  GetBindRegistry()[web_contents] = std::move(callback);
}

// static
void BravePassageEmbeddingsService::RemoveBindCallbackForWebContents(
    content::WebContents* web_contents) {
  GetBindRegistry().erase(web_contents);
}

// static
void BravePassageEmbeddingsService::BindForWebContents(
    content::WebContents* web_contents,
    mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver) {
  auto& registry = GetBindRegistry();
  auto it = registry.find(web_contents);
  if (it != registry.end()) {
    it->second.Run(std::move(receiver));
  }
}

void BravePassageEmbeddingsService::BindPassageEmbedder(
    mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
    base::OnceCallback<void(bool)> callback) {
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
      std::move(callback),
      base::BindOnce(
          &BravePassageEmbeddingsService::OnBatchEmbedderDisconnected,
          weak_ptr_factory_.GetWeakPtr()));
  MaybeLoadLocalModelFiles();
}

void BravePassageEmbeddingsService::LoadModels(
    mojom::PassageEmbeddingsLoadModelsParamsPtr model_params,
    mojom::PassageEmbedderParamsPtr params,
    mojo::PendingReceiver<mojom::PassageEmbedder> model,
    LoadModelsCallback callback) {
  // Brave does not consume the upstream model file params — we use our
  // own EmbeddingGemma model hosted by a WASM renderer. The params
  // auto-destruct on return (closing any file handles); delegate the
  // receiver to the in-process path.
  BindPassageEmbedder(std::move(model), std::move(callback));
}

void BravePassageEmbeddingsService::OnLocalModelsReady(
    const base::FilePath& install_dir) {
  MaybeLoadLocalModelFiles();
}

void BravePassageEmbeddingsService::MaybeLoadLocalModelFiles() {
  if (file_load_in_flight_ || !batch_embedder_) {
    return;
  }
  if (updater_state_->GetEmbeddingGemmaModelDir().empty()) {
    return;
  }
  file_load_in_flight_ = true;
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&LoadLocalModelFilesFromDisk,
                     updater_state_->GetEmbeddingGemmaModel(),
                     updater_state_->GetEmbeddingGemmaDense1(),
                     updater_state_->GetEmbeddingGemmaDense2(),
                     updater_state_->GetEmbeddingGemmaTokenizer(),
                     updater_state_->GetEmbeddingGemmaConfig()),
      base::BindOnce(&BravePassageEmbeddingsService::OnLocalModelFilesLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BravePassageEmbeddingsService::OnLocalModelFilesLoaded(
    local_ai::mojom::ModelFilesPtr model_files) {
  file_load_in_flight_ = false;
  if (!batch_embedder_) {
    return;
  }
  if (!model_files) {
    DVLOG(1) << "Model files load failed; resetting BatchEmbedder";
    batch_embedder_.reset();
    return;
  }
  batch_embedder_->SetModelFiles(std::move(model_files));
}

void BravePassageEmbeddingsService::OnBatchEmbedderDisconnected() {
  DVLOG(3) << "BraveBatchPassageEmbedder disconnected; tearing down";
  batch_embedder_.reset();
  // Cancel any in-flight file load reply so it doesn't land after the
  // embedder is gone.
  weak_ptr_factory_.InvalidateWeakPtrs();
  file_load_in_flight_ = false;
}

}  // namespace passage_embeddings
