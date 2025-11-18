// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/candle_service.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace local_ai {

namespace {

mojom::ModelFilesPtr LoadEmbeddingGemmaModelFilesFromDisk(
    const base::FilePath& weights_path,
    const base::FilePath& tokenizer_path,
    const base::FilePath& config_path) {
  auto weights_opt = base::ReadFileToBytes(weights_path);
  if (!weights_opt) {
    DVLOG(0) << "Failed to read model weights from: " << weights_path;
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
  DVLOG(1) << "Loaded tokenizer, size: " << tokenizer_opt->size();
  DVLOG(1) << "Loaded config, size: " << config_opt->size();

  // Create BigBuffer directly - it will automatically use shared memory for
  // large data (> 64KB)
  auto model_files = mojom::ModelFiles::New();
  model_files->weights = mojo_base::BigBuffer(std::move(*weights_opt));
  model_files->tokenizer = mojo_base::BigBuffer(std::move(*tokenizer_opt));
  model_files->config = mojo_base::BigBuffer(std::move(*config_opt));

  return model_files;
}

}  // namespace

// static
CandleService* CandleService::GetInstance() {
  static base::NoDestructor<CandleService> instance;
  return instance.get();
}

CandleService::CandleService() = default;
CandleService::~CandleService() = default;

void CandleService::BindReceiver(
    mojo::PendingReceiver<mojom::CandleService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void CandleService::BindEmbeddingGemma(
    mojo::PendingRemote<mojom::EmbeddingGemmaInterface> pending_remote) {
  // Reset if already bound (e.g., on page reload)
  if (embedding_gemma_remote_.is_bound()) {
    embedding_gemma_remote_.reset();
  }
  embedding_gemma_remote_.Bind(std::move(pending_remote));
  // Don't auto-initialize - wait for user to provide file paths
}

void CandleService::GetDefaultModelPath(GetDefaultModelPathCallback callback) {
  auto home_dir = base::PathService::CheckedGet(base::DIR_HOME);
  base::FilePath model_dir =
      home_dir.Append("Downloads").Append("embeddinggemma-300m");
  std::move(callback).Run(model_dir);
}

void CandleService::LoadModelFiles(const base::FilePath& weights_path,
                                   const base::FilePath& tokenizer_path,
                                   const base::FilePath& config_path,
                                   LoadModelFilesCallback callback) {
  if (!embedding_gemma_remote_) {
    DVLOG(0) << "Embedding Gemma interface not bound";
    std::move(callback).Run(false);
    return;
  }

  DVLOG(1) << "Loading Embedding Gemma model files from specified paths...";
  DVLOG(1) << "Weights: " << weights_path;
  DVLOG(1) << "Tokenizer: " << tokenizer_path;
  DVLOG(1) << "Config: " << config_path;

  // Load model files on a background thread to avoid blocking
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&LoadEmbeddingGemmaModelFilesFromDisk, weights_path,
                     tokenizer_path, config_path),
      base::BindOnce(&CandleService::OnEmbeddingGemmaModelFilesLoaded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CandleService::Embed(const std::string& text, EmbedCallback callback) {
  if (!embedding_gemma_remote_) {
    DVLOG(0) << "Embedding Gemma not initialized";
    std::move(callback).Run({});
    return;
  }

  embedding_gemma_remote_->Embed(text, std::move(callback));
}

void CandleService::OnEmbeddingGemmaModelFilesLoaded(
    LoadModelFilesCallback callback,
    mojom::ModelFilesPtr model_files) {
  if (!model_files) {
    DVLOG(0) << "Failed to load embedding gemma model files from disk";
    std::move(callback).Run(false);
    return;
  }

  embedding_gemma_remote_->Init(std::move(model_files), std::move(callback));
}

}  // namespace local_ai
