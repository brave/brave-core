// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_batch_passage_embedder.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/numerics/safe_math.h"

namespace passage_embeddings {

BraveBatchPassageEmbedder::Batch::Batch() = default;
BraveBatchPassageEmbedder::Batch::~Batch() = default;
BraveBatchPassageEmbedder::Batch::Batch(Batch&&) noexcept = default;
BraveBatchPassageEmbedder::Batch& BraveBatchPassageEmbedder::Batch::operator=(
    Batch&&) noexcept = default;

BraveBatchPassageEmbedder::BraveBatchPassageEmbedder(
    mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
    mojo::PendingRemote<local_ai::mojom::PassageEmbedder> renderer_embedder,
    base::OnceClosure on_disconnect)
    : receiver_(this, std::move(receiver)),
      renderer_embedder_(std::move(renderer_embedder)),
      on_disconnect_(std::move(on_disconnect)) {
  receiver_.set_disconnect_handler(
      base::BindOnce(&BraveBatchPassageEmbedder::OnDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  renderer_embedder_.set_disconnect_handler(
      base::BindOnce(&BraveBatchPassageEmbedder::OnDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
}

BraveBatchPassageEmbedder::~BraveBatchPassageEmbedder() = default;

void BraveBatchPassageEmbedder::GenerateEmbeddings(
    const std::vector<std::string>& passages,
    mojom::PassagePriority priority,
    GenerateEmbeddingsCallback callback) {
  if (passages.empty()) {
    std::move(callback).Run({});
    return;
  }
  DVLOG(3) << "GenerateEmbeddings: queued batch of " << passages.size()
           << " passage(s), priority=" << static_cast<int>(priority);
  Batch batch;
  batch.passages = passages;
  batch.callback = std::move(callback);
  pending_batches_.push_back(std::move(batch));
  if (!current_batch_) {
    ProcessNext();
  }
}

void BraveBatchPassageEmbedder::ProcessNext() {
  CHECK(!current_batch_);
  if (pending_batches_.empty()) {
    return;
  }
  current_batch_.emplace(std::move(pending_batches_.front()));
  pending_batches_.pop_front();

  if (!renderer_embedder_.is_bound()) {
    DVLOG(1) << "Renderer embedder not bound; failing batch of "
             << current_batch_->passages.size() << " passage(s)";
    FailCurrentBatch();
    return;
  }
  DVLOG(3) << "ProcessNext: dispatching passage "
           << current_batch_->next_index + 1 << "/"
           << current_batch_->passages.size();
  const std::string& text =
      current_batch_->passages[current_batch_->next_index];
  renderer_embedder_->GenerateEmbeddings(
      text, base::BindOnce(&BraveBatchPassageEmbedder::OnOneEmbedding,
                           weak_ptr_factory_.GetWeakPtr()));
}

void BraveBatchPassageEmbedder::OnOneEmbedding(
    const std::vector<double>& embedding) {
  if (!current_batch_) {
    return;
  }
  if (embedding.empty()) {
    DVLOG(1) << "Renderer returned empty embedding for passage "
             << current_batch_->next_index + 1 << "/"
             << current_batch_->passages.size() << "; failing batch";
    FailCurrentBatch();
    return;
  }
  auto floats = ToFloatEmbedding(embedding);
  if (!floats) {
    DVLOG(1) << "Embedding value out of float range for passage "
             << current_batch_->next_index + 1 << "/"
             << current_batch_->passages.size() << "; failing batch";
    FailCurrentBatch();
    return;
  }
  auto result = mojom::PassageEmbeddingsResult::New();
  result->embeddings = std::move(*floats);
  current_batch_->results.push_back(std::move(result));
  current_batch_->next_index++;

  if (current_batch_->next_index == current_batch_->passages.size()) {
    DVLOG(3) << "Batch complete: " << current_batch_->results.size()
             << " embedding(s) returned to caller";
    Batch done = std::move(*current_batch_);
    current_batch_.reset();
    std::move(done.callback).Run(std::move(done.results));
    ProcessNext();
    return;
  }
  const std::string& text =
      current_batch_->passages[current_batch_->next_index];
  renderer_embedder_->GenerateEmbeddings(
      text, base::BindOnce(&BraveBatchPassageEmbedder::OnOneEmbedding,
                           weak_ptr_factory_.GetWeakPtr()));
}

void BraveBatchPassageEmbedder::FailCurrentBatch() {
  if (!current_batch_) {
    return;
  }
  Batch failed = std::move(*current_batch_);
  current_batch_.reset();
  std::move(failed.callback).Run({});
  ProcessNext();
}

void BraveBatchPassageEmbedder::OnDisconnected() {
  std::deque<Batch> to_fail = std::move(pending_batches_);
  if (current_batch_) {
    to_fail.push_front(std::move(*current_batch_));
    current_batch_.reset();
  }
  DVLOG_IF(1, !to_fail.empty())
      << "BatchEmbedder pipe disconnected with " << to_fail.size()
      << " in-flight/queued batch(es); failing all";
  for (auto& batch : to_fail) {
    if (batch.callback) {
      std::move(batch.callback).Run({});
    }
  }
  if (on_disconnect_) {
    std::move(on_disconnect_).Run();
  }
}

// static
std::optional<std::vector<float>> BraveBatchPassageEmbedder::ToFloatEmbedding(
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

}  // namespace passage_embeddings
