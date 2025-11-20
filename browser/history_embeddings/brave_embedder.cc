// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_embedder.h"

#include "base/barrier_callback.h"
#include "base/logging.h"
#include "brave/browser/local_ai/local_ai_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history_embeddings/history_embeddings_features.h"
#include "components/history_embeddings/history_embeddings_service.h"

namespace brave {

BraveEmbedder::PendingTask::PendingTask() = default;
BraveEmbedder::PendingTask::~PendingTask() = default;
BraveEmbedder::PendingTask::PendingTask(PendingTask&&) = default;
BraveEmbedder::PendingTask& BraveEmbedder::PendingTask::operator=(
    PendingTask&&) = default;

BraveEmbedder::BraveEmbedder(Profile* profile) {
  DVLOG(3) << "BraveEmbedder created";

  local_ai_service_.Bind(
      local_ai::LocalAIServiceFactory::GetForProfile(profile));
  if (!local_ai_service_.is_bound()) {
    DVLOG(2) << "BraveEmbedder: LocalAIService is not available";
  }
}

void BraveEmbedder::AcquirePassageEmbedder() {
  if (acquiring_embedder_) {
    return;
  }
  acquiring_embedder_ = true;
  local_ai_service_->GetPassageEmbedder(
      base::BindOnce(&BraveEmbedder::OnPassageEmbedderAcquired,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveEmbedder::OnPassageEmbedderAcquired(
    mojo::PendingRemote<local_ai::mojom::PassageEmbedder> remote) {
  acquiring_embedder_ = false;
  if (!remote.is_valid()) {
    DVLOG(2) << "BraveEmbedder: received invalid PassageEmbedder remote";
    FailAllPendingTasks();
    return;
  }
  passage_embedder_.Bind(std::move(remote));
  passage_embedder_.set_disconnect_handler(
      base::BindOnce(&BraveEmbedder::OnPassageEmbedderDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  // Process any tasks that were queued while waiting for the embedder.
  for (auto& [task_id, task] : pending_tasks_) {
    if (!task.in_flight && !task.cancelled) {
      ProcessAllPassagesInParallel(task_id);
    }
  }
}

void BraveEmbedder::OnPassageEmbedderDisconnected() {
  DVLOG(2) << "BraveEmbedder: PassageEmbedder disconnected";
  passage_embedder_.reset();
  acquiring_embedder_ = false;
  FailAllPendingTasks();
}

void BraveEmbedder::FailAllPendingTasks() {
  auto tasks = std::move(pending_tasks_);
  for (auto& [task_id, task] : tasks) {
    if (task.callback) {
      std::move(task.callback)
          .Run(std::move(task.passages), {}, task_id,
               passage_embeddings::ComputeEmbeddingsStatus::kExecutionFailure);
    }
  }
}

BraveEmbedder::~BraveEmbedder() = default;

// Note: The priority parameter is accepted to satisfy the Embedder interface
// contract, but it is not used for scheduling. All tasks are processed
// immediately in parallel regardless of priority, which is suitable for local
// LocalAIService execution.
BraveEmbedder::TaskId BraveEmbedder::ComputePassagesEmbeddings(
    passage_embeddings::PassagePriority priority,
    std::vector<std::string> passages,
    ComputePassagesEmbeddingsCallback callback) {
  TaskId task_id = next_task_id_++;

  DVLOG(3) << "BraveEmbedder::ComputePassagesEmbeddings called with task_id="
           << task_id << ", " << passages.size() << " passages";

  if (passages.empty()) {
    DVLOG(3) << "No passages to embed, returning success";
    // Return immediately with empty results
    std::move(callback).Run(
        std::move(passages), {}, task_id,
        passage_embeddings::ComputeEmbeddingsStatus::kSuccess);
    return task_id;
  }

  // Check if LocalAIService is available
  if (!local_ai_service_.is_bound() && !passage_embedder_.is_bound()) {
    DVLOG(2) << "LocalAIService not initialized, cannot compute embeddings";
    std::move(callback).Run(
        std::move(passages), {}, task_id,
        passage_embeddings::ComputeEmbeddingsStatus::kExecutionFailure);
    return task_id;
  }

  DVLOG(3) << "LocalAIService is available, creating task";

  // Create pending task
  PendingTask task;
  task.passages = passages;
  task.callback = std::move(callback);
  task.embeddings.resize(passages.size());

  pending_tasks_[task_id] = std::move(task);

  if (passage_embedder_.is_bound()) {
    ProcessAllPassagesInParallel(task_id);
  } else {
    AcquirePassageEmbedder();
  }

  return task_id;
}

void BraveEmbedder::ReprioritizeTasks(
    passage_embeddings::PassagePriority priority,
    const std::set<TaskId>& tasks) {
  // This is a no-op. BraveEmbedder processes all tasks immediately in
  // parallel when ComputePassagesEmbeddings is called, so by the time
  // ReprioritizeTasks could be invoked, the embedding requests are already
  // in-flight or completed. This differs from Chromium's SchedulingEmbedder,
  // which maintains a priority queue and can reorder pending tasks.
  //
  // The cancellation window for tasks is extremely small (only between task
  // creation and the immediate ProcessAllPassagesInParallel call), making
  // reprioritization effectively meaningless for this implementation.

  // Log if someone tries to reprioritize active tasks (for debugging)
  if (VLOG_IS_ON(3) && !tasks.empty()) {
    size_t found_tasks = 0;
    for (TaskId task_id : tasks) {
      if (pending_tasks_.find(task_id) != pending_tasks_.end()) {
        ++found_tasks;
      }
    }
    if (found_tasks > 0) {
      VLOG(3) << "ReprioritizeTasks called for " << found_tasks
              << " active tasks, but this is a no-op (tasks already "
              << "processing in parallel)";
    }
  }
}

// Note: The cancellation window is very small because tasks are processed
// immediately in parallel when created. Cancellation only succeeds if the
// task hasn't been marked in_flight yet.
bool BraveEmbedder::TryCancel(TaskId task_id) {
  auto it = pending_tasks_.find(task_id);
  if (it == pending_tasks_.end()) {
    return false;
  }

  // Only cancel if requests haven't been issued yet
  if (!it->second.in_flight) {
    it->second.cancelled = true;
    std::vector<std::string> passages = std::move(it->second.passages);
    ComputePassagesEmbeddingsCallback callback = std::move(it->second.callback);
    pending_tasks_.erase(it);

    // Invoke callback with cancelled status
    std::move(callback).Run(
        std::move(passages), {}, task_id,
        passage_embeddings::ComputeEmbeddingsStatus::kCanceled);
    return true;
  }

  return false;
}

void BraveEmbedder::ProcessAllPassagesInParallel(TaskId task_id) {
  auto it = pending_tasks_.find(task_id);
  if (it == pending_tasks_.end() || it->second.cancelled) {
    DVLOG(3) << "Task " << task_id << " not found or cancelled";
    return;
  }

  PendingTask& task = it->second;

  DVLOG(3) << "ProcessAllPassagesInParallel: task_id=" << task_id
           << ", passages=" << task.passages.size();

  // If passage_embedder_ is not yet bound, wait for it.
  if (!passage_embedder_.is_bound()) {
    DVLOG(3) << "PassageEmbedder not yet bound, deferring task " << task_id;
    return;
  }

  // Mark task as in-flight
  task.in_flight = true;

  // Create barrier callback to collect all results
  size_t num_passages = task.passages.size();
  auto barrier_callback =
      base::BarrierCallback<std::pair<size_t, std::vector<double>>>(
          num_passages,
          base::BindOnce(&BraveEmbedder::OnAllEmbeddingsComplete,
                         weak_ptr_factory_.GetWeakPtr(), task_id));

  // Issue all embedding requests in parallel
  for (size_t i = 0; i < num_passages; ++i) {
    const std::string& passage = task.passages[i];

    DVLOG(3) << "Calling LocalAIService::GenerateEmbeddings for passage " << i
             << ", length=" << passage.length();

    passage_embedder_->GenerateEmbeddings(
        passage,
        base::BindOnce(&BraveEmbedder::OnSingleEmbeddingResult,
                       weak_ptr_factory_.GetWeakPtr(), barrier_callback, i));
  }
}

void BraveEmbedder::OnSingleEmbeddingResult(
    base::RepeatingCallback<void(std::pair<size_t, std::vector<double>>)>
        barrier_callback,
    size_t passage_index,
    const std::vector<double>& embedding) {
  DVLOG(3) << "OnSingleEmbeddingResult: passage_index=" << passage_index
           << ", embedding_size=" << embedding.size();

  // Forward the result to the barrier callback
  barrier_callback.Run(std::make_pair(passage_index, embedding));
}

void BraveEmbedder::OnAllEmbeddingsComplete(
    TaskId task_id,
    const std::vector<std::pair<size_t, std::vector<double>>>& results) {
  DVLOG(3) << "OnAllEmbeddingsComplete: task_id=" << task_id
           << ", results=" << results.size();

  auto it = pending_tasks_.find(task_id);
  if (it == pending_tasks_.end() || it->second.cancelled) {
    DVLOG(2) << "Task " << task_id << " not found or cancelled";
    return;
  }

  PendingTask& task = it->second;

  // Check if any embedding failed (empty result)
  for (const auto& result : results) {
    if (result.second.empty()) {
      DVLOG(2) << "Embedding failed for passage " << result.first
               << " - empty embedding returned";
      std::vector<std::string> passages = std::move(task.passages);
      ComputePassagesEmbeddingsCallback callback = std::move(task.callback);
      pending_tasks_.erase(it);

      std::move(callback).Run(
          std::move(passages), {}, task_id,
          passage_embeddings::ComputeEmbeddingsStatus::kExecutionFailure);
      MaybeReleasePassageEmbedder();
      return;
    }
  }

  DVLOG(3) << "All embeddings successful, converting to float";

  // Convert all embeddings and store them
  for (const auto& result : results) {
    size_t passage_index = result.first;
    const std::vector<double>& embedding = result.second;

    // Convert double vector to float vector
    std::vector<float> float_embedding;
    float_embedding.reserve(embedding.size());
    for (double val : embedding) {
      float_embedding.push_back(static_cast<float>(val));
    }

    // Count words in the passage for filtering
    size_t word_count =
        history_embeddings::CountWords(task.passages[passage_index]);

    // Store the embedding
    task.embeddings[passage_index] =
        passage_embeddings::Embedding(std::move(float_embedding), word_count);
  }

  // All passages processed successfully
  DVLOG(3) << "All passages processed for task " << task_id
           << ", invoking callback with " << task.embeddings.size()
           << " embeddings";

  std::vector<std::string> passages = std::move(task.passages);
  std::vector<passage_embeddings::Embedding> embeddings =
      std::move(task.embeddings);
  ComputePassagesEmbeddingsCallback callback = std::move(task.callback);
  pending_tasks_.erase(it);

  std::move(callback).Run(
      std::move(passages), std::move(embeddings), task_id,
      passage_embeddings::ComputeEmbeddingsStatus::kSuccess);

  MaybeReleasePassageEmbedder();
}

void BraveEmbedder::MaybeReleasePassageEmbedder() {
  if (pending_tasks_.empty() && passage_embedder_.is_bound()) {
    DVLOG(3) << "BraveEmbedder: All tasks done, releasing PassageEmbedder";
    passage_embedder_.reset();
  }
}

}  // namespace brave
