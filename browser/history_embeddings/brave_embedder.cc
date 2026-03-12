// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_embedder.h"

#include <algorithm>

#include "base/check_is_test.h"
#include "base/logging.h"
#include "brave/browser/local_ai/local_ai_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history_embeddings/history_embeddings_service.h"
#include "components/passage_embeddings/core/passage_embeddings_features.h"

namespace passage_embeddings {

BraveEmbedder::Job::Job() = default;
BraveEmbedder::Job::~Job() = default;
BraveEmbedder::Job::Job(Job&&) noexcept = default;
BraveEmbedder::Job& BraveEmbedder::Job::operator=(Job&&) noexcept = default;

BraveEmbedder::BraveEmbedder(Profile* profile) {
  DVLOG(3) << "BraveEmbedder created";

  local_ai_service_.Bind(
      local_ai::LocalAIServiceFactory::GetForProfile(profile));
  if (!local_ai_service_.is_bound()) {
    DVLOG(2) << "BraveEmbedder: LocalAIService is not available";
  }
}

BraveEmbedder::BraveEmbedder(
    mojo::PendingRemote<local_ai::mojom::PassageEmbedder> remote) {
  CHECK_IS_TEST();
  passage_embedder_.Bind(std::move(remote));
  passage_embedder_.set_disconnect_handler(
      base::BindOnce(&BraveEmbedder::OnPassageEmbedderDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
}

BraveEmbedder::~BraveEmbedder() = default;

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
  ProcessNextPassage();
}

void BraveEmbedder::OnPassageEmbedderDisconnected() {
  DVLOG(2) << "BraveEmbedder: PassageEmbedder disconnected";
  idle_timer_.Stop();
  passage_embedder_.reset();
  acquiring_embedder_ = false;
  FailAllPendingTasks();
}

void BraveEmbedder::OnIdleTimeout() {
  DVLOG(2) << "BraveEmbedder: Idle timeout, disconnecting PassageEmbedder";
  passage_embedder_.reset();
}

void BraveEmbedder::RestartIdleTimer() {
  idle_timer_.Start(FROM_HERE, kEmbedderTimeout.Get(),
                    base::BindOnce(&BraveEmbedder::OnIdleTimeout,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void BraveEmbedder::FailAllPendingTasks() {
  work_submitted_ = false;
  auto jobs = std::move(jobs_);
  for (auto& job : jobs) {
    if (job.callback) {
      std::move(job.callback)
          .Run(std::move(job.passages), {}, job.task_id,
               ComputeEmbeddingsStatus::kExecutionFailure);
    }
  }
}

BraveEmbedder::TaskId BraveEmbedder::ComputePassagesEmbeddings(
    PassagePriority priority,
    std::vector<std::string> passages,
    ComputePassagesEmbeddingsCallback callback) {
  TaskId task_id = next_task_id_++;

  if (passages.empty()) {
    std::move(callback).Run(std::move(passages), {}, task_id,
                            ComputeEmbeddingsStatus::kSuccess);
    return task_id;
  }

  if (!local_ai_service_.is_bound() && !passage_embedder_.is_bound()) {
    std::move(callback).Run(std::move(passages), {}, task_id,
                            ComputeEmbeddingsStatus::kExecutionFailure);
    return task_id;
  }

  idle_timer_.Stop();

  Job job;
  job.priority = priority;
  job.task_id = task_id;
  job.passages = std::move(passages);
  job.callback = std::move(callback);
  jobs_.push_back(std::move(job));

  if (passage_embedder_.is_bound()) {
    ProcessNextPassage();
  } else {
    AcquirePassageEmbedder();
  }

  return task_id;
}

void BraveEmbedder::ReprioritizeTasks(PassagePriority priority,
                                      const std::set<TaskId>& tasks) {
  for (auto& job : jobs_) {
    if (tasks.count(job.task_id)) {
      job.priority = priority;
    }
  }
}

bool BraveEmbedder::TryCancel(TaskId task_id) {
  for (auto it = jobs_.begin(); it != jobs_.end(); ++it) {
    if (it->task_id == task_id) {
      // Cannot cancel the front job if it has work in-flight.
      if (work_submitted_ && it == jobs_.begin()) {
        return false;
      }
      auto job = std::move(*it);
      jobs_.erase(it);
      std::move(job.callback)
          .Run(std::move(job.passages), {}, job.task_id,
               ComputeEmbeddingsStatus::kCanceled);
      return true;
    }
  }
  return false;
}

void BraveEmbedder::ProcessNextPassage() {
  if (work_submitted_ || jobs_.empty() || !passage_embedder_.is_bound()) {
    return;
  }

  // Re-sort by priority so high-priority search queries go first.
  std::stable_sort(jobs_.begin(), jobs_.end(), [](const Job& a, const Job& b) {
    return a.priority < b.priority;
  });

  Job& job = jobs_.front();
  size_t next_index = job.embeddings.size();

  DVLOG(3) << "ProcessNextPassage: task=" << job.task_id << " passage "
           << next_index << "/" << job.passages.size();

  work_submitted_ = true;
  passage_embedder_->GenerateEmbeddings(
      job.passages[next_index],
      base::BindOnce(&BraveEmbedder::OnSinglePassageComplete,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveEmbedder::OnSinglePassageComplete(
    const std::vector<double>& embedding) {
  work_submitted_ = false;

  if (jobs_.empty()) {
    return;
  }

  Job& job = jobs_.front();

  if (embedding.empty()) {
    DVLOG(2) << "Embedding failed for passage " << job.embeddings.size()
             << " of task " << job.task_id;
    auto failed_job = std::move(jobs_.front());
    jobs_.pop_front();
    std::move(failed_job.callback)
        .Run(std::move(failed_job.passages), {}, failed_job.task_id,
             ComputeEmbeddingsStatus::kExecutionFailure);
    ProcessNextPassage();
    return;
  }

  // Convert double to float and store.
  std::vector<float> float_embedding;
  float_embedding.reserve(embedding.size());
  for (double val : embedding) {
    float_embedding.push_back(static_cast<float>(val));
  }
  size_t word_count =
      history_embeddings::CountWords(job.passages[job.embeddings.size()]);
  job.embeddings.emplace_back(std::move(float_embedding), word_count);

  // Check if this job is complete.
  if (job.embeddings.size() == job.passages.size()) {
    DVLOG(3) << "Job " << job.task_id << " complete with "
             << job.embeddings.size() << " embeddings";
    auto completed_job = std::move(jobs_.front());
    jobs_.pop_front();
    std::move(completed_job.callback)
        .Run(std::move(completed_job.passages),
             std::move(completed_job.embeddings), completed_job.task_id,
             ComputeEmbeddingsStatus::kSuccess);
  }

  if (jobs_.empty() && passage_embedder_.is_bound()) {
    RestartIdleTimer();
  } else {
    ProcessNextPassage();
  }
}

}  // namespace passage_embeddings
