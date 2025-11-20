// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_embedder.h"

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/local_ai/browser/candle_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/history_embeddings/history_embeddings_features.h"
#include "components/history_embeddings/history_embeddings_service.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"

namespace brave {

BraveEmbedder::PendingTask::PendingTask() = default;
BraveEmbedder::PendingTask::~PendingTask() = default;
BraveEmbedder::PendingTask::PendingTask(PendingTask&&) = default;
BraveEmbedder::PendingTask& BraveEmbedder::PendingTask::operator=(
    PendingTask&&) = default;

BraveEmbedder::BraveEmbedder()
    : candle_service_(local_ai::CandleService::GetInstance()) {
  if (!candle_service_) {
    LOG(WARNING) << "BraveEmbedder: CandleService is not available.";
    return;
  }

  // Create a hidden WebContents to load the WASM
  profile_ = ProfileManager::GetLastUsedProfile();
  if (!profile_) {
    LOG(ERROR) << "BraveEmbedder: No profile available";
    return;
  }

  // Observe the profile for shutdown
  profile_->AddObserver(this);

  content::WebContents::CreateParams create_params(profile_);
  create_params.initially_hidden = true;
  wasm_web_contents_ = content::WebContents::Create(create_params);

  // Observe the WebContents
  Observe(wasm_web_contents_.get());

  // Navigate to the WASM page - this will trigger BindEmbeddingGemma automatically
  GURL wasm_url(kUntrustedCandleEmbeddingGemmaWasmURL);
  LOG(INFO) << "BraveEmbedder: Loading WASM from " << wasm_url;
  wasm_web_contents_->GetController().LoadURL(
      wasm_url, content::Referrer(), ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
      std::string());
}

BraveEmbedder::~BraveEmbedder() {
  CloseWasmWebContents();
  if (profile_) {
    profile_->RemoveObserver(this);
  }
}

BraveEmbedder::TaskId BraveEmbedder::ComputePassagesEmbeddings(
    passage_embeddings::PassagePriority priority,
    std::vector<std::string> passages,
    ComputePassagesEmbeddingsCallback callback) {
  TaskId task_id = next_task_id_++;

  LOG(INFO) << "BraveEmbedder::ComputePassagesEmbeddings called with task_id="
            << task_id << ", " << passages.size() << " passages";

  if (passages.empty()) {
    LOG(INFO) << "No passages to embed, returning success";
    // Return immediately with empty results
    std::move(callback).Run(
        std::move(passages), {},
        task_id,
        passage_embeddings::ComputeEmbeddingsStatus::kSuccess);
    return task_id;
  }

  // Check if CandleService is available
  if (!candle_service_) {
    LOG(WARNING) << "CandleService not initialized, cannot compute embeddings";
    std::move(callback).Run(
        std::move(passages), {},
        task_id,
        passage_embeddings::ComputeEmbeddingsStatus::kExecutionFailure);
    return task_id;
  }

  LOG(INFO) << "CandleService is available, creating task";

  // Create pending task
  PendingTask task;
  task.priority = priority;
  task.passages = passages;
  task.callback = std::move(callback);
  task.embeddings.resize(passages.size());

  pending_tasks_[task_id] = std::move(task);

  // Start processing the first passage
  ProcessNextPassage(task_id);

  return task_id;
}

void BraveEmbedder::ReprioritizeTasks(
    passage_embeddings::PassagePriority priority,
    const std::set<TaskId>& tasks) {
  // Update priority for specified tasks
  for (TaskId task_id : tasks) {
    auto it = pending_tasks_.find(task_id);
    if (it != pending_tasks_.end()) {
      it->second.priority = priority;
    }
  }
  // Note: We don't reorder execution here, but could be implemented
  // if needed for more sophisticated scheduling
}

bool BraveEmbedder::TryCancel(TaskId task_id) {
  auto it = pending_tasks_.find(task_id);
  if (it == pending_tasks_.end()) {
    return false;
  }

  // Only cancel if no passages have been processed yet
  if (it->second.completed_count == 0) {
    it->second.cancelled = true;
    std::vector<std::string> passages = std::move(it->second.passages);
    ComputePassagesEmbeddingsCallback callback =
        std::move(it->second.callback);
    pending_tasks_.erase(it);

    // Invoke callback with cancelled status
    std::move(callback).Run(
        std::move(passages), {},
        task_id,
        passage_embeddings::ComputeEmbeddingsStatus::kCanceled);
    return true;
  }

  return false;
}

void BraveEmbedder::ProcessNextPassage(TaskId task_id) {
  auto it = pending_tasks_.find(task_id);
  if (it == pending_tasks_.end() || it->second.cancelled) {
    LOG(INFO) << "Task " << task_id << " not found or cancelled";
    return;
  }

  PendingTask& task = it->second;
  size_t passage_index = task.completed_count;

  LOG(INFO) << "ProcessNextPassage: task_id=" << task_id
            << ", passage_index=" << passage_index
            << "/" << task.passages.size();

  if (passage_index >= task.passages.size()) {
    // All passages processed, invoke callback
    LOG(INFO) << "All passages processed for task " << task_id
              << ", invoking callback with " << task.embeddings.size()
              << " embeddings";
    std::vector<std::string> passages = std::move(task.passages);
    std::vector<passage_embeddings::Embedding> embeddings =
        std::move(task.embeddings);
    ComputePassagesEmbeddingsCallback callback = std::move(task.callback);
    pending_tasks_.erase(it);

    std::move(callback).Run(
        std::move(passages),
        std::move(embeddings),
        task_id,
        passage_embeddings::ComputeEmbeddingsStatus::kSuccess);
    return;
  }

  // Check if CandleService is available
  if (!candle_service_) {
    LOG(WARNING) << "CandleService not available at passage " << passage_index;
    std::vector<std::string> passages = std::move(task.passages);
    ComputePassagesEmbeddingsCallback callback = std::move(task.callback);
    pending_tasks_.erase(it);

    std::move(callback).Run(
        std::move(passages), {},
        task_id,
        passage_embeddings::ComputeEmbeddingsStatus::kExecutionFailure);
    return;
  }

  // Embed the current passage
  const std::string& passage = task.passages[passage_index];

  LOG(INFO) << "Calling CandleService::Embed for passage " << passage_index
            << ", length=" << passage.length();

  candle_service_->Embed(
      passage,
      base::BindOnce(&BraveEmbedder::OnEmbeddingResult,
                     weak_ptr_factory_.GetWeakPtr(),
                     task_id,
                     passage_index));
}

void BraveEmbedder::OnEmbeddingResult(
    TaskId task_id,
    size_t passage_index,
    const std::vector<double>& embedding) {
  LOG(INFO) << "OnEmbeddingResult: task_id=" << task_id
            << ", passage_index=" << passage_index
            << ", embedding_size=" << embedding.size();

  auto it = pending_tasks_.find(task_id);
  if (it == pending_tasks_.end() || it->second.cancelled) {
    LOG(WARNING) << "Task " << task_id << " not found or cancelled in callback";
    return;
  }

  PendingTask& task = it->second;

  if (embedding.empty()) {
    // Embedding failed, return error for all passages
    LOG(WARNING) << "Embedding failed for passage " << passage_index
                 << " - empty embedding returned";
    std::vector<std::string> passages = std::move(task.passages);
    ComputePassagesEmbeddingsCallback callback = std::move(task.callback);
    pending_tasks_.erase(it);

    std::move(callback).Run(
        std::move(passages), {},
        task_id,
        passage_embeddings::ComputeEmbeddingsStatus::kExecutionFailure);
    return;
  }

  LOG(INFO) << "Successfully received embedding with " << embedding.size()
            << " dimensions";

  // Convert double vector to float vector
  std::vector<float> float_embedding;
  float_embedding.reserve(embedding.size());
  for (double val : embedding) {
    float_embedding.push_back(static_cast<float>(val));
  }

  // Count words in the passage for filtering
  size_t word_count = history_embeddings::CountWords(task.passages[passage_index]);

  // Store the embedding
  task.embeddings[passage_index] =
      passage_embeddings::Embedding(std::move(float_embedding), word_count);
  task.completed_count++;

  // Process next passage
  ProcessNextPassage(task_id);
}

void BraveEmbedder::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                   const GURL& validated_url) {
  LOG(INFO) << "BraveEmbedder: WASM page loaded: " << validated_url;

  // The WASM has now automatically called BindEmbeddingGemma
  // Now we can load the model files
  LoadWasmModel();
}

void BraveEmbedder::OnProfileWillBeDestroyed(Profile* profile) {
  LOG(INFO) << "BraveEmbedder: Profile is being destroyed, closing WebContents";
  CloseWasmWebContents();
  profile_->RemoveObserver(this);
  profile_ = nullptr;
}

void BraveEmbedder::CloseWasmWebContents() {
  if (wasm_web_contents_) {
    Observe(nullptr);
    wasm_web_contents_->Close();
    wasm_web_contents_.reset();
  }
}

void BraveEmbedder::LoadWasmModel() {
  LOG(INFO) << "BraveEmbedder: Loading EmbeddingGemma model files...";

  // Get default model path and load model files
  candle_service_->GetDefaultModelPath(base::BindOnce(
      &BraveEmbedder::OnGotDefaultModelPath,
      weak_ptr_factory_.GetWeakPtr()));
}

void BraveEmbedder::OnGotDefaultModelPath(const std::optional<base::FilePath>& model_path) {
  if (!model_path.has_value()) {
    LOG(ERROR) << "BraveEmbedder: No default model path provided";
    return;
  }

  LOG(INFO) << "BraveEmbedder: Default model path: " << model_path.value();

  // Store model path for potential retries
  pending_model_path_ = model_path.value();

  // Build paths for model files
  base::FilePath weights_path =
      pending_model_path_.Append(FILE_PATH_LITERAL("model.safetensors"));
  base::FilePath tokenizer_path =
      pending_model_path_.Append(FILE_PATH_LITERAL("tokenizer.json"));
  base::FilePath config_path =
      pending_model_path_.Append(FILE_PATH_LITERAL("config.json"));

  LOG(INFO) << "BraveEmbedder: Loading model files (attempt "
            << (model_load_retry_count_ + 1) << "/" << kMaxModelLoadRetries << "):";
  LOG(INFO) << "  Weights: " << weights_path;
  LOG(INFO) << "  Tokenizer: " << tokenizer_path;
  LOG(INFO) << "  Config: " << config_path;

  // Load the model files
  candle_service_->LoadModelFiles(
      weights_path, tokenizer_path, config_path,
      base::BindOnce(&BraveEmbedder::OnModelFilesLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveEmbedder::OnModelFilesLoaded(bool success) {
  if (success) {
    LOG(INFO) << "BraveEmbedder: EmbeddingGemma model loaded successfully! "
              << "History embeddings are now ready.";
    model_load_retry_count_ = 0;
  } else {
    // Failed - this could be because binding isn't ready yet or file not found
    model_load_retry_count_++;

    if (model_load_retry_count_ < kMaxModelLoadRetries) {
      LOG(WARNING) << "BraveEmbedder: Failed to load model (attempt "
                   << model_load_retry_count_ << "/" << kMaxModelLoadRetries
                   << "). Retrying in 100ms...";
      RetryLoadWasmModel();
    } else {
      LOG(ERROR) << "BraveEmbedder: Failed to load EmbeddingGemma model after "
                 << kMaxModelLoadRetries << " attempts. "
                 << "History embeddings will not work. "
                 << "Make sure model files exist in ~/Downloads/embeddinggemma-300m/";
      model_load_retry_count_ = 0;
    }
  }
}

void BraveEmbedder::RetryLoadWasmModel() {
  // Post a delayed task to retry after 100ms
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BraveEmbedder::OnGotDefaultModelPath,
                     weak_ptr_factory_.GetWeakPtr(),
                     pending_model_path_),
      base::Milliseconds(100));
}

}  // namespace brave
