// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_embedder.h"

#include "base/logging.h"
#include "brave/browser/local_ai/candle_service_factory.h"
#include "brave/components/local_ai/browser/candle_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history_embeddings/history_embeddings_features.h"
#include "components/history_embeddings/history_embeddings_service.h"

namespace brave {

BraveEmbedder::PendingTask::PendingTask() = default;
BraveEmbedder::PendingTask::~PendingTask() = default;
BraveEmbedder::PendingTask::PendingTask(PendingTask&&) = default;
BraveEmbedder::PendingTask& BraveEmbedder::PendingTask::operator=(
    PendingTask&&) = default;

BraveEmbedder::BraveEmbedder(Profile* profile)
    : candle_service_(
          local_ai::CandleServiceFactory::GetForBrowserContext(profile)) {
  LOG(INFO) << "BraveEmbedder created";

  if (!candle_service_) {
    LOG(WARNING) << "BraveEmbedder: CandleService is not available";
  }
}

BraveEmbedder::~BraveEmbedder() = default;

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

}  // namespace brave
