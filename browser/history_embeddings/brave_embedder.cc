// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_embedder.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"
#include "base/numerics/safe_math.h"
#include "components/history_embeddings/content/history_embeddings_service.h"

namespace passage_embeddings {

BraveEmbedder::Job::Job() = default;
BraveEmbedder::Job::~Job() = default;
BraveEmbedder::Job::Job(Job&&) noexcept = default;
BraveEmbedder::Job& BraveEmbedder::Job::operator=(Job&&) noexcept = default;

BraveEmbedder::BraveEmbedder(
    mojo::PendingRemote<local_ai::mojom::LocalAIService> service) {
  if (service.is_valid()) {
    local_ai_service_.Bind(std::move(service));
  }
}

BraveEmbedder::~BraveEmbedder() = default;

void BraveEmbedder::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveEmbedder::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BraveEmbedder::NotifyServiceIdle() {
  if (local_ai_service_.is_bound()) {
    local_ai_service_->NotifyPassageEmbedderIdle();
  }
}

void BraveEmbedder::AcquirePassageEmbedder() {
  if (acquiring_embedder_) {
    return;
  }
  acquiring_embedder_ = true;
  auto reset_flag = base::ScopedClosureRunner(base::BindOnce(
      [](base::WeakPtr<BraveEmbedder> self) {
        if (self) {
          self->acquiring_embedder_ = false;
        }
      },
      weak_ptr_factory_.GetWeakPtr()));
  local_ai_service_->GetPassageEmbedder(
      base::BindOnce(&BraveEmbedder::OnPassageEmbedderAcquired,
                     weak_ptr_factory_.GetWeakPtr(), std::move(reset_flag)));
}

void BraveEmbedder::OnPassageEmbedderAcquired(
    base::ScopedClosureRunner reset_flag,
    mojo::PendingRemote<local_ai::mojom::PassageEmbedder> remote) {
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
  passage_embedder_.reset();
  acquiring_embedder_ = false;
  FailAllPendingTasks();
}

void BraveEmbedder::FailJob(Job job) {
  if (job.callback) {
    std::move(job.callback)
        .Run(std::move(job.passages), {}, job.task_id,
             ComputeEmbeddingsStatus::kExecutionFailure);
  }
}

void BraveEmbedder::CompleteJob(Job job) {
  std::move(job.callback)
      .Run(std::move(job.passages), std::move(job.embeddings), job.task_id,
           ComputeEmbeddingsStatus::kSuccess);
}

void BraveEmbedder::FailAllPendingTasks() {
  std::deque<Job> jobs = std::move(jobs_);
  if (auto job = std::exchange(current_job_, std::nullopt)) {
    jobs.push_front(std::move(*job));
  }
  for (auto& job : jobs) {
    FailJob(std::move(job));
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
                                      const std::set<TaskId>& task_ids) {
  for (auto& job : jobs_) {
    if (task_ids.count(job.task_id)) {
      job.priority = priority;
    }
  }
}

bool BraveEmbedder::TryCancel(TaskId task_id) {
  if (current_job_ && current_job_->task_id == task_id) {
    auto job = std::exchange(current_job_, std::nullopt);
    std::move(job->callback)
        .Run(std::move(job->passages), {}, job->task_id,
             ComputeEmbeddingsStatus::kCanceled);
    ProcessNextPassage();
    return true;
  }
  for (auto it = jobs_.begin(); it != jobs_.end(); ++it) {
    if (it->task_id == task_id) {
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
  // If the embedder disconnected, OnPassageEmbedderDisconnected will
  // call FailAllPendingTasks to drain current_job_ and jobs_.
  if (current_job_ || !passage_embedder_.is_bound()) {
    return;
  }

  if (jobs_.empty()) {
    passage_embedder_.reset();
    for (auto& observer : observers_) {
      observer.OnEmbedderIdle();
    }
    return;
  }

  // Re-sort by priority so high-priority search queries go first.
  std::stable_sort(jobs_.begin(), jobs_.end(), [](const Job& a, const Job& b) {
    return a.priority < b.priority;
  });

  current_job_.emplace(std::move(jobs_.front()));
  jobs_.pop_front();

  size_t next_index = current_job_->embeddings.size();

  DVLOG(3) << "ProcessNextPassage: task=" << current_job_->task_id
           << " passage " << next_index << "/" << current_job_->passages.size();

  passage_embedder_->GenerateEmbeddings(
      current_job_->passages[next_index],
      base::BindOnce(&BraveEmbedder::OnPassageProcessed,
                     weak_ptr_factory_.GetWeakPtr()));
}

// static
std::optional<std::vector<float>> BraveEmbedder::ConvertEmbedding(
    const std::vector<double>& embedding) {
  std::vector<float> result;
  result.reserve(embedding.size());
  for (double val : embedding) {
    float f;
    if (!base::CheckedNumeric<float>(val).AssignIfValid(&f)) {
      return std::nullopt;
    }
    result.push_back(f);
  }
  return result;
}

void BraveEmbedder::OnPassageProcessed(const std::vector<double>& embedding) {
  if (!current_job_) {
    return;
  }

  if (embedding.empty()) {
    DVLOG(2) << "Embedding failed for passage "
             << current_job_->embeddings.size() << " of task "
             << current_job_->task_id;
    FailJob(std::move(*std::exchange(current_job_, std::nullopt)));
    ProcessNextPassage();
    return;
  }

  auto float_embedding = ConvertEmbedding(embedding);
  if (!float_embedding) {
    DVLOG(0) << "Embedding value out of float range for passage "
             << current_job_->embeddings.size() << " of task "
             << current_job_->task_id;
    FailJob(std::move(*std::exchange(current_job_, std::nullopt)));
    ProcessNextPassage();
    return;
  }

  size_t word_count = history_embeddings::CountWords(
      current_job_->passages[current_job_->embeddings.size()]);
  current_job_->embeddings.emplace_back(std::move(*float_embedding),
                                        word_count);

  // Check if this job is complete.
  if (current_job_->embeddings.size() == current_job_->passages.size()) {
    DVLOG(3) << "Job " << current_job_->task_id << " complete with "
             << current_job_->embeddings.size() << " embeddings";
    CompleteJob(std::move(*std::exchange(current_job_, std::nullopt)));
    ProcessNextPassage();
    return;
  }

  // More passages remain — put the job back so ProcessNextPassage can
  // re-sort by priority, allowing higher-priority jobs to preempt.
  jobs_.push_front(std::move(*std::exchange(current_job_, std::nullopt)));
  ProcessNextPassage();
}

}  // namespace passage_embeddings
