// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/candle_service.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "components/grit/brave_components_resources.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "ui/base/resource/resource_bundle.h"

namespace local_ai {

namespace {

// TODO(darkdh): Use component updater to deliver model files
mojom::ModelFilesPtr LoadModelFilesFromResources() {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();

  scoped_refptr<base::RefCountedMemory> weights_data =
      resource_bundle.LoadDataResourceBytes(
          IDR_LOCAL_AI_BERT_MODEL_SAFETENSORS);
  scoped_refptr<base::RefCountedMemory> tokenizer_data =
      resource_bundle.LoadDataResourceBytes(IDR_LOCAL_AI_BERT_TOKENIZER_JSON);
  scoped_refptr<base::RefCountedMemory> config_data =
      resource_bundle.LoadDataResourceBytes(IDR_LOCAL_AI_BERT_CONFIG_JSON);

  if (!weights_data || !tokenizer_data || !config_data) {
    LOG(ERROR) << "Failed to load BERT model files from resources";
    return nullptr;
  }

  auto model_files = mojom::ModelFiles::New();
  model_files->weights.assign(weights_data->begin(), weights_data->end());
  model_files->tokenizer.assign(tokenizer_data->begin(), tokenizer_data->end());
  model_files->config.assign(config_data->begin(), config_data->end());

  return model_files;
}

mojom::LargeModelFilesPtr LoadEmbeddingGemmaModelFilesFromDisk() {
  // TODO(darkdh): Make this configurable or use component updater
  base::FilePath home_dir;
  if (!base::PathService::Get(base::DIR_HOME, &home_dir)) {
    LOG(ERROR) << "Failed to get home directory";
    return nullptr;
  }

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
  auto model_files = mojom::LargeModelFiles::New();
  model_files->weights = mojo_base::BigBuffer(std::move(*weights_opt));
  model_files->tokenizer = mojo_base::BigBuffer(std::move(*tokenizer_opt));
  model_files->config = mojo_base::BigBuffer(std::move(*config_opt));

  return model_files;
}

mojom::LargeModelFilesPtr LoadPhiModelFilesFromDisk() {
  // TODO(darkdh): Make this configurable or use component updater
  base::FilePath home_dir;
  if (!base::PathService::Get(base::DIR_HOME, &home_dir)) {
    LOG(ERROR) << "Failed to get home directory";
    return nullptr;
  }

  base::FilePath model_dir =
      home_dir.Append("Downloads").Append("candle-quantized-phi");

  base::FilePath weights_path = model_dir.Append("model-q4k.gguf");
  base::FilePath tokenizer_path = model_dir.Append("tokenizer.json");
  base::FilePath config_path = model_dir.Append("phi-1_5.json");

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

  LOG(ERROR) << "Loaded Phi weights, size: " << weights_opt->size();
  LOG(ERROR) << "Loaded Phi tokenizer, size: " << tokenizer_opt->size();
  LOG(ERROR) << "Loaded Phi config, size: " << config_opt->size();

  // Create BigBuffer directly - it will automatically use shared memory for
  // large data (> 64KB)
  auto model_files = mojom::LargeModelFiles::New();
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

void CandleService::BindBert(
    mojo::PendingRemote<mojom::BertInterface> pending_remote) {
  // Reset if already bound (e.g., on page reload)
  if (bert_remote_.is_bound()) {
    bert_remote_.reset();
  }
  bert_remote_.Bind(std::move(pending_remote));
  // Run immediately when bound for easy POC purpose only
  RunBertExample();
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

void CandleService::BindPhi(
    mojo::PendingRemote<mojom::PhiInterface> pending_remote) {
  // Reset if already bound (e.g., on page reload)
  if (phi_remote_.is_bound()) {
    phi_remote_.reset();
  }
  phi_remote_.Bind(std::move(pending_remote));
  // Initialize immediately when bound
  RunPhiInit();
}

void CandleService::Embed(const std::string& text, EmbedCallback callback) {
  if (!embedding_gemma_remote_) {
    LOG(ERROR) << "Embedding Gemma not initialized";
    std::move(callback).Run({});
    return;
  }

  embedding_gemma_remote_->Embed(text, std::move(callback));
}

void CandleService::RunBertExample() {
  auto model_files = LoadModelFilesFromResources();
  if (!model_files) {
    LOG(ERROR) << "Failed to load model files";
    return;
  }

  bert_remote_->RunExample(std::move(model_files),
                           base::BindOnce(&CandleService::OnRunBertExample,
                                          weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnRunBertExample(const std::string& result) {
  LOG(ERROR) << __PRETTY_FUNCTION__ << "\n" << result;
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
    mojom::LargeModelFilesPtr model_files) {
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
  } else {
    LOG(ERROR) << "Failed to initialize Embedding Gemma model";
  }
}

void CandleService::RunPhiInit() {
  LOG(ERROR) << "Loading Phi model files from disk...";

  // Load model files on a background thread to avoid blocking
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&LoadPhiModelFilesFromDisk),
      base::BindOnce(&CandleService::OnPhiModelFilesLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnPhiModelFilesLoaded(
    mojom::LargeModelFilesPtr model_files) {
  if (!model_files) {
    LOG(ERROR) << "Failed to load Phi model files from disk";
    return;
  }

  LOG(ERROR) << "Phi model files loaded, sending Init request...";
  // Note: quantized=true for q4k GGUF model
  phi_remote_->Init(std::move(model_files), true,
                    base::BindOnce(&CandleService::OnPhiInit,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnPhiInit(bool success) {
  if (success) {
    LOG(ERROR) << "Phi model initialized successfully!";
  } else {
    LOG(ERROR) << "Failed to initialize Phi model";
  }
}

}  // namespace local_ai
