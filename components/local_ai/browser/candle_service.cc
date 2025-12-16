// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/candle_service.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/local_ai/browser/local_models_updater.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "ui/base/page_transition_types.h"

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

CandleService::CandleService(content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  DVLOG(3) << "CandleService created for browser context";

  if (!browser_context_) {
    DVLOG(0) << "CandleService: No browser context available";
    return;
  }

  // Observe the component updater for model readiness
  LocalModelsUpdaterState::GetInstance()->AddObserver(this);

  // Create a hidden WebContents to load the WASM
  content::WebContents::CreateParams create_params(browser_context_);
  create_params.is_never_composited = true;
  wasm_web_contents_ = content::WebContents::Create(create_params);

  // Observe the WebContents
  Observe(wasm_web_contents_.get());

  // Navigate to the WASM page - this will trigger BindEmbeddingGemma
  // automatically
  GURL wasm_url(kUntrustedCandleEmbeddingGemmaWasmURL);
  DVLOG(3) << "CandleService: Loading WASM from " << wasm_url;
  wasm_web_contents_->GetController().LoadURL(wasm_url, content::Referrer(),
                                              ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                              std::string());
}

CandleService::~CandleService() {
  LocalModelsUpdaterState::GetInstance()->RemoveObserver(this);
  CloseWasmWebContents();
}

void CandleService::BindReceiver(
    mojo::PendingReceiver<mojom::CandleService> receiver) {
  receivers_.Add(this, std::move(receiver));
  DVLOG(3) << "BindReceiver called";
}

void CandleService::BindEmbeddingGemma(
    mojo::PendingRemote<mojom::EmbeddingGemmaInterface> pending_remote) {
  // Bind the single embedder remote from our WASM WebContents
  if (embedding_gemma_remote_.is_bound()) {
    DVLOG(1) << "EmbeddingGemma already bound, resetting";
    embedding_gemma_remote_.reset();
    model_initialized_ = false;
  }
  embedding_gemma_remote_.Bind(std::move(pending_remote));

  // Set up disconnect handler
  embedding_gemma_remote_.set_disconnect_handler(base::BindOnce(
      [](CandleService* service) {
        DVLOG(1) << "EmbeddingGemma remote disconnected";
        service->model_initialized_ = false;
        // Clear pending requests on disconnect
        for (auto& request : service->pending_embed_requests_) {
          std::move(request.callback).Run({});
        }
        service->pending_embed_requests_.clear();
      },
      base::Unretained(this)));

  DVLOG(3) << "BindEmbeddingGemma: Bound embedder remote";

  // Try to load model now that remote is bound
  TryLoadModel();
}

void CandleService::GetDefaultModelPath(GetDefaultModelPathCallback callback) {
  // Use the local_models_updater to get the model path
  const base::FilePath& model_dir =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaModelDir();

  if (model_dir.empty()) {
    DVLOG(1) << "CandleService: Model directory not set in updater state";
    std::move(callback).Run(std::nullopt);
    return;
  }

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
    DVLOG(0) << "Embedding Gemma remote not bound";
    std::move(callback).Run({});
    return;
  }

  // If model is not initialized yet, queue the request
  if (!model_initialized_) {
    DVLOG(3) << "Model not initialized yet, queuing embed request";
    pending_embed_requests_.emplace_back(text, std::move(callback));
    return;
  }

  embedding_gemma_remote_->Embed(text, std::move(callback));
}

void CandleService::OnEmbeddingGemmaModelFilesLoaded(
    LoadModelFilesCallback callback,
    mojom::ModelFilesPtr model_files) {
  DVLOG(3) << "CandleService::OnEmbeddingGemmaModelFilesLoaded called";

  if (!model_files) {
    DVLOG(0) << "Failed to load embedding gemma model files from disk";
    std::move(callback).Run(false);
    return;
  }

  DVLOG(3) << "Calling embedding_gemma_remote_->Init()...";
  embedding_gemma_remote_->Init(std::move(model_files), std::move(callback));
}

void CandleService::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                  const GURL& validated_url) {
  DVLOG(3) << "CandleService: WASM page loaded: " << validated_url;
  wasm_page_loaded_ = true;

  // Try to load model if both conditions are met
  TryLoadModel();
}

void CandleService::OnComponentReady(const base::FilePath& install_dir) {
  DVLOG(3) << "CandleService: Component ready at: " << install_dir;
  component_ready_ = true;

  // Try to load model if both conditions are met
  TryLoadModel();
}

void CandleService::TryLoadModel() {
  DVLOG(3) << "CandleService::TryLoadModel - wasm_page_loaded_="
           << wasm_page_loaded_ << ", component_ready_=" << component_ready_
           << ", remote_bound=" << embedding_gemma_remote_.is_bound()
           << ", model_initialized_=" << model_initialized_;

  if (!wasm_page_loaded_) {
    DVLOG(3) << "CandleService: Waiting for WASM page to load...";
    return;
  }

  if (!component_ready_) {
    DVLOG(3) << "CandleService: Waiting for component to be ready...";
    return;
  }

  if (!embedding_gemma_remote_.is_bound()) {
    DVLOG(1) << "CandleService: WASM page loaded but remote not bound yet";
    return;
  }

  if (model_initialized_) {
    DVLOG(3) << "CandleService: Model already initialized";
    return;
  }

  DVLOG(3) << "CandleService: Both WASM and component ready, loading model...";
  LoadWasmModel();
}

void CandleService::Shutdown() {
  DVLOG(3) << "CandleService: Shutting down";

  // Clear any pending requests
  for (auto& request : pending_embed_requests_) {
    std::move(request.callback).Run({});
  }
  pending_embed_requests_.clear();

  CloseWasmWebContents();
}

void CandleService::ProcessPendingEmbedRequests() {
  if (!model_initialized_ || !embedding_gemma_remote_) {
    return;
  }

  DVLOG(3) << "Processing " << pending_embed_requests_.size()
           << " pending embed requests";

  // Process all queued requests
  for (auto& request : pending_embed_requests_) {
    embedding_gemma_remote_->Embed(request.text, std::move(request.callback));
  }
  pending_embed_requests_.clear();
}

void CandleService::CloseWasmWebContents() {
  if (wasm_web_contents_) {
    Observe(nullptr);
    wasm_web_contents_->Close();
    wasm_web_contents_.reset();
  }
}

void CandleService::LoadWasmModel() {
  DVLOG(3) << "CandleService: Loading EmbeddingGemma model files...";

  // Get default model path and load model files
  GetDefaultModelPath(base::BindOnce(&CandleService::OnGotDefaultModelPath,
                                     weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnGotDefaultModelPath(
    const std::optional<base::FilePath>& model_path) {
  if (!model_path.has_value()) {
    DVLOG(0) << "CandleService: No default model path provided";
    return;
  }

  DVLOG(3) << "CandleService: Default model path: " << model_path.value();

  // Store model path for potential retries
  pending_model_path_ = model_path.value();

  // Build paths for model files - use the files from local_models_updater
  base::FilePath weights_path =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaModel();
  base::FilePath tokenizer_path =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaTokenizer();
  base::FilePath config_path =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaConfig();

  DVLOG(3) << "CandleService: Loading model files (attempt "
           << (model_load_retry_count_ + 1) << "/" << kMaxModelLoadRetries
           << "):";
  DVLOG(3) << "  Weights: " << weights_path;
  DVLOG(3) << "  Tokenizer: " << tokenizer_path;
  DVLOG(3) << "  Config: " << config_path;

  // Load the model files
  LoadModelFiles(weights_path, tokenizer_path, config_path,
                 base::BindOnce(&CandleService::OnModelFilesLoaded,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnModelFilesLoaded(bool success) {
  DVLOG(3) << "CandleService::OnModelFilesLoaded called with success="
           << success;

  if (success) {
    DVLOG(3) << "CandleService: EmbeddingGemma model loaded successfully! "
             << "History embeddings are now ready.";
    model_load_retry_count_ = 0;
    model_initialized_ = true;

    DVLOG(3) << "Processing " << pending_embed_requests_.size()
             << " pending requests";
    // Process any queued embed requests
    ProcessPendingEmbedRequests();
  } else {
    // Failed - this could be because binding isn't ready yet or file not found
    model_load_retry_count_++;
    model_initialized_ = false;

    if (model_load_retry_count_ < kMaxModelLoadRetries) {
      DVLOG(1) << "CandleService: Failed to load model (attempt "
               << model_load_retry_count_ << "/" << kMaxModelLoadRetries
               << "). Retrying in 100ms...";
      RetryLoadWasmModel();
    } else {
      DVLOG(0) << "CandleService: Failed to load EmbeddingGemma model after "
               << kMaxModelLoadRetries << " attempts. "
               << "History embeddings will not work. "
               << "Make sure model files are downloaded via component "
               << "updater.";
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
