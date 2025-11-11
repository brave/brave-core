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

namespace local_ai {

namespace {

mojom::ModelFilesPtr LoadEmbeddingGemmaModelFilesFromDisk() {
  auto home_dir = base::PathService::CheckedGet(base::DIR_HOME);

  base::FilePath model_dir =
      home_dir.Append("Downloads").Append("embeddinggemma-300m");

  base::FilePath weights_path = model_dir.Append("model.safetensors");
  base::FilePath tokenizer_path = model_dir.Append("tokenizer.json");
  base::FilePath config_path = model_dir.Append("config.json");

  std::string weights_data;
  std::string tokenizer_data;
  std::string config_data;

  if (!base::ReadFileToString(weights_path, &weights_data)) {
    LOG(ERROR) << "Failed to read model weights from: " << weights_path;
    return nullptr;
  }

  if (!base::ReadFileToString(tokenizer_path, &tokenizer_data)) {
    LOG(ERROR) << "Failed to read tokenizer from: " << tokenizer_path;
    return nullptr;
  }

  if (!base::ReadFileToString(config_path, &config_data)) {
    LOG(ERROR) << "Failed to read config from: " << config_path;
    return nullptr;
  }

  auto model_files = mojom::ModelFiles::New();
  model_files->weights.assign(weights_data.begin(), weights_data.end());
  model_files->tokenizer.assign(tokenizer_data.begin(), tokenizer_data.end());
  model_files->config.assign(config_data.begin(), config_data.end());

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

  LOG(ERROR) << "Model files loaded, initializing Embedding Gemma model...";
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
