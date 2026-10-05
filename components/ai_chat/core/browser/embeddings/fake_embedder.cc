// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/embeddings/fake_embedder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"

namespace ai_chat {

namespace {

constexpr std::array<std::string_view, 3> kAnimals = {"cat", "dog", "bird"};

passage_embeddings::Embedding EmbedPassage(std::string_view passage) {
  const std::string text = base::ToLowerASCII(passage);
  std::vector<float> data;
  for (std::string_view animal : kAnimals) {
    data.push_back(text.find(animal) != std::string::npos ? 1.0f : 0.0f);
  }
  // Keeps the vector from being zero for a passage that mentions no animal.
  data.push_back(0.1f);
  float magnitude = 0.0f;
  for (float value : data) {
    magnitude += value * value;
  }
  magnitude = std::sqrt(magnitude);
  for (float& value : data) {
    value /= magnitude;
  }
  return passage_embeddings::Embedding(std::move(data));
}

}  // namespace

FakeEmbedder::PendingJob::PendingJob() = default;
FakeEmbedder::PendingJob::PendingJob(PendingJob&&) = default;
FakeEmbedder::PendingJob& FakeEmbedder::PendingJob::operator=(PendingJob&&) =
    default;
FakeEmbedder::PendingJob::~PendingJob() = default;

FakeEmbedder::FakeEmbedder() = default;
FakeEmbedder::~FakeEmbedder() = default;

bool FakeEmbedder::HasEmbedded(std::string_view text) const {
  return std::ranges::any_of(embedded_passages_, [&](const auto& passage) {
    return passage.first.find(text) != std::string::npos;
  });
}

passage_embeddings::Embedder::Job FakeEmbedder::ComputePassagesEmbeddings(
    passage_embeddings::PassagePriority priority,
    std::vector<std::string> passages,
    ComputePassagesEmbeddingsCallback callback) {
  const uint64_t job_id = next_job_id_++;
  PendingJob job;
  job.priority = priority;
  job.passages = std::move(passages);
  job.callback = std::move(callback);
  pending_jobs_.emplace(job_id, std::move(job));
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&FakeEmbedder::RunJob,
                                weak_ptr_factory_.GetWeakPtr(), job_id));
  return Job(weak_ptr_factory_.GetWeakPtr(), job_id);
}

base::WeakPtr<passage_embeddings::Embedder> FakeEmbedder::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void FakeEmbedder::ReprioritizeJobs(
    passage_embeddings::PassagePriority priority,
    const std::set<uint64_t>& job_ids) {}

bool FakeEmbedder::TryCancel(uint64_t job_id) {
  auto node = pending_jobs_.extract(job_id);
  if (!node) {
    return false;
  }
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(node.mapped().callback),
                     std::move(node.mapped().passages),
                     std::vector<passage_embeddings::Embedding>(), job_id,
                     passage_embeddings::ComputeEmbeddingsStatus::kCanceled));
  return true;
}

void FakeEmbedder::RunJob(uint64_t job_id) {
  auto node = pending_jobs_.extract(job_id);
  if (!node) {
    return;
  }
  PendingJob& job = node.mapped();
  std::vector<passage_embeddings::Embedding> embeddings =
      base::ToVector(job.passages, &EmbedPassage);
  for (const std::string& passage : job.passages) {
    embedded_passages_.emplace_back(passage, job.priority);
  }
  std::move(job.callback)
      .Run(std::move(job.passages), std::move(embeddings), job_id,
           passage_embeddings::ComputeEmbeddingsStatus::kSuccess);
}

FakeEmbedderMetadataProvider::FakeEmbedderMetadataProvider()
    : metadata_(/*model_version=*/1,
                /*output_size=*/4,
                /*search_score_threshold=*/0.5) {}

FakeEmbedderMetadataProvider::~FakeEmbedderMetadataProvider() = default;

void FakeEmbedderMetadataProvider::SetMetadata(
    passage_embeddings::EmbedderMetadata metadata) {
  metadata_ = metadata;
  for (auto& observer : observers_) {
    observer.EmbedderMetadataUpdated(metadata_);
  }
}

void FakeEmbedderMetadataProvider::AddObserver(
    passage_embeddings::EmbedderMetadataObserver* observer) {
  observers_.AddObserver(observer);
  observer->EmbedderMetadataUpdated(metadata_);
}

void FakeEmbedderMetadataProvider::RemoveObserver(
    passage_embeddings::EmbedderMetadataObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace ai_chat
