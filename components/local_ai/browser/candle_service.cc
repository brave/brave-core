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

mojom::ModelFilesPtr LoadEmbeddingGemmaModelFilesFromDisk() {
  auto home_dir = base::PathService::CheckedGet(base::DIR_HOME);

  base::FilePath model_dir =
      home_dir.Append("Downloads").Append("embeddinggemma-300m");

  base::FilePath weights_path = model_dir.Append("model.safetensors");
  base::FilePath tokenizer_path = model_dir.Append("tokenizer.json");
  base::FilePath config_path = model_dir.Append("config.json");

  auto weights_opt = base::ReadFileToBytes(weights_path);
  if (!weights_opt) {
    LOG(ERROR) << "Failed to read model weights from: " << weights_path;
    return nullptr;
  }

  auto tokenizer_opt = base::ReadFileToBytes(tokenizer_path);
  if (!tokenizer_opt) {
    LOG(ERROR) << "Failed to read tokenizer from: " << tokenizer_path;
    return nullptr;
  }

  auto config_opt = base::ReadFileToBytes(config_path);
  if (!config_opt) {
    LOG(ERROR) << "Failed to read config from: " << config_path;
    return nullptr;
  }

  LOG(ERROR) << "Loaded weights, size: " << weights_opt->size();
  LOG(ERROR) << "Loaded tokenizer, size: " << tokenizer_opt->size();
  LOG(ERROR) << "Loaded config, size: " << config_opt->size();

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
  // Initialize immediately when bound
  RunEmbeddingGemmaInit();
}

void CandleService::RunEmbeddingGemmaInit() {
  LOG(ERROR) << "Loading Embedding Gemma model files from disk...";

  // Load model files on a background thread to avoid blocking
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&LoadEmbeddingGemmaModelFilesFromDisk),
      base::BindOnce(&CandleService::OnEmbeddingGemmaModelFilesLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnEmbeddingGemmaModelFilesLoaded(
    mojom::ModelFilesPtr model_files) {
  if (!model_files) {
    LOG(ERROR) << "Failed to load embedding gemma model files from disk";
    return;
  }

  LOG(ERROR) << "Model files loaded, sending Init request...";
  embedding_gemma_remote_->Init(
      std::move(model_files),
      base::BindOnce(&CandleService::OnEmbeddingGemmaInit,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnEmbeddingGemmaInit(bool success) {
  if (success) {
    LOG(ERROR) << "Embedding Gemma model initialized successfully!";
    // Run example embeddings after successful initialization
    RunEmbeddingGemmaExamples();
  } else {
    LOG(ERROR) << "Failed to initialize Embedding Gemma model";
  }
}

void CandleService::RunEmbeddingGemmaExamples() {
  // Test with a few example strings
  std::vector<std::string> test_strings = {
      "The cat sits outside",
      "A man is playing guitar",
      "I love pasta",
      "The new movie is awesome",
  };

  LOG(ERROR) << "Running embedding examples...";
  for (const auto& text : test_strings) {
    embedding_gemma_remote_->Embed(
        text, base::BindOnce(&CandleService::OnEmbeddingGemmaEmbed,
                             weak_ptr_factory_.GetWeakPtr(), text));
  }
}

void CandleService::OnEmbeddingGemmaEmbed(
    const std::string& text,
    const std::vector<double>& embedding) {
  LOG(ERROR) << "Embedding for \"" << text
             << "\": dimension=" << embedding.size() << ", first 5 values: [";
  for (size_t i = 0; i < std::min(size_t(5), embedding.size()); ++i) {
    LOG(ERROR) << "  " << embedding[i] << (i < 4 ? "," : "");
  }
  LOG(ERROR) << "]";
}

}  // namespace local_ai
