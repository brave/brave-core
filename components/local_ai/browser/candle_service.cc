// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/candle_service.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "ui/base/page_transition_types.h"

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

CandleService::CandleService(Profile* profile) : profile_(profile) {
  LOG(INFO) << "CandleService created for profile";

  if (!profile_) {
    LOG(ERROR) << "CandleService: No profile available";
    return;
  }

  // Observe the profile for shutdown
  profile_->AddObserver(this);

  // Create a hidden WebContents to load the WASM
  content::WebContents::CreateParams create_params(profile_);
  create_params.is_never_composited = true;
  wasm_web_contents_ = content::WebContents::Create(create_params);

  // Observe the WebContents
  Observe(wasm_web_contents_.get());

  // Navigate to the WASM page - this will trigger BindEmbeddingGemma
  // automatically
  GURL wasm_url(kUntrustedCandleEmbeddingGemmaWasmURL);
  LOG(INFO) << "CandleService: Loading WASM from " << wasm_url;
  wasm_web_contents_->GetController().LoadURL(
      wasm_url, content::Referrer(), ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
      std::string());
}

CandleService::~CandleService() {
  CloseWasmWebContents();
  if (profile_) {
    profile_->RemoveObserver(this);
  }
}

void CandleService::BindReceiver(
    mojo::PendingReceiver<mojom::CandleService> receiver) {
  receivers_.Add(this, std::move(receiver));
  LOG(INFO) << "BindReceiver called";
}

void CandleService::BindEmbeddingGemma(
    mojo::PendingRemote<mojom::EmbeddingGemmaInterface> pending_remote) {
  // Bind the single embedder remote from our WASM WebContents
  if (embedding_gemma_remote_.is_bound()) {
    LOG(WARNING) << "EmbeddingGemma already bound, resetting";
    embedding_gemma_remote_.reset();
  }
  embedding_gemma_remote_.Bind(std::move(pending_remote));
  LOG(INFO) << "BindEmbeddingGemma: Bound embedder remote";
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
  LOG(INFO) << "CandleService::Embed called with text length=" << text.length();

  if (!embedding_gemma_remote_) {
    LOG(WARNING) << "Embedding Gemma remote not bound - model not loaded/initialized";
    std::move(callback).Run({});
    return;
  }

  LOG(INFO) << "Forwarding Embed request to EmbeddingGemma";
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

void CandleService::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                   const GURL& validated_url) {
  LOG(INFO) << "CandleService: WASM page loaded: " << validated_url;

  // The WASM has now automatically called BindEmbeddingGemma
  // Now we can load the model files
  LoadWasmModel();
}

void CandleService::OnProfileWillBeDestroyed(Profile* profile) {
  LOG(INFO) << "CandleService: Profile is being destroyed, closing WebContents";
  CloseWasmWebContents();
  profile_->RemoveObserver(this);
  profile_ = nullptr;
}

void CandleService::Shutdown() {
  LOG(INFO) << "CandleService: Shutting down";
  CloseWasmWebContents();
}

void CandleService::CloseWasmWebContents() {
  if (wasm_web_contents_) {
    Observe(nullptr);
    wasm_web_contents_->Close();
    wasm_web_contents_.reset();
  }
}

void CandleService::LoadWasmModel() {
  LOG(INFO) << "CandleService: Loading EmbeddingGemma model files...";

  // Get default model path and load model files
  GetDefaultModelPath(base::BindOnce(&CandleService::OnGotDefaultModelPath,
                                     weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnGotDefaultModelPath(
    const std::optional<base::FilePath>& model_path) {
  if (!model_path.has_value()) {
    LOG(ERROR) << "CandleService: No default model path provided";
    return;
  }

  LOG(INFO) << "CandleService: Default model path: " << model_path.value();

  // Store model path for potential retries
  pending_model_path_ = model_path.value();

  // Build paths for model files
  base::FilePath weights_path =
      pending_model_path_.Append(FILE_PATH_LITERAL("model.safetensors"));
  base::FilePath tokenizer_path =
      pending_model_path_.Append(FILE_PATH_LITERAL("tokenizer.json"));
  base::FilePath config_path =
      pending_model_path_.Append(FILE_PATH_LITERAL("config.json"));

  LOG(INFO) << "CandleService: Loading model files (attempt "
            << (model_load_retry_count_ + 1) << "/" << kMaxModelLoadRetries
            << "):";
  LOG(INFO) << "  Weights: " << weights_path;
  LOG(INFO) << "  Tokenizer: " << tokenizer_path;
  LOG(INFO) << "  Config: " << config_path;

  // Load the model files
  LoadModelFiles(weights_path, tokenizer_path, config_path,
                 base::BindOnce(&CandleService::OnModelFilesLoaded,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnModelFilesLoaded(bool success) {
  if (success) {
    LOG(INFO) << "CandleService: EmbeddingGemma model loaded successfully! "
              << "History embeddings are now ready.";
    model_load_retry_count_ = 0;
  } else {
    // Failed - this could be because binding isn't ready yet or file not found
    model_load_retry_count_++;

    if (model_load_retry_count_ < kMaxModelLoadRetries) {
      LOG(WARNING) << "CandleService: Failed to load model (attempt "
                   << model_load_retry_count_ << "/" << kMaxModelLoadRetries
                   << "). Retrying in 100ms...";
      RetryLoadWasmModel();
    } else {
      LOG(ERROR) << "CandleService: Failed to load EmbeddingGemma model after "
                 << kMaxModelLoadRetries << " attempts. "
                 << "History embeddings will not work. "
                 << "Make sure model files exist in "
                    "~/Downloads/embeddinggemma-300m/";
      model_load_retry_count_ = 0;
    }
  }
}

void CandleService::RetryLoadWasmModel() {
  // Post a delayed task to retry after 100ms
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&CandleService::OnGotDefaultModelPath,
                     weak_ptr_factory_.GetWeakPtr(), pending_model_path_),
      base::Milliseconds(100));
}

}  // namespace local_ai
