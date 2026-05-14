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
    BackgroundWebContentsFactory background_web_contents_factory,
    local_ai::mojom::ModelFilesPtr model_files,
    base::OnceCallback<void(bool)> load_callback,
    base::OnceClosure on_disconnect)
    : background_web_contents_factory_(
          std::move(background_web_contents_factory)),
      pending_receiver_(std::move(receiver)),
      model_files_(std::move(model_files)),
      load_callback_(std::move(load_callback)),
      on_disconnect_(std::move(on_disconnect)) {
  CHECK(model_files_);
  background_web_contents_factory_.Run(
      this,
      base::BindOnce(&BraveBatchPassageEmbedder::OnBackgroundContentsCreated,
                     weak_ptr_factory_.GetWeakPtr()));
}

BraveBatchPassageEmbedder::~BraveBatchPassageEmbedder() {
  // Owner is destroying us — fail any still-pending load before we go
  // away. Don't fire on_disconnect_; the owner already knows.
  if (load_callback_) {
    std::move(load_callback_).Run(false);
  }
}

void BraveBatchPassageEmbedder::BindLocalAIReceiver(
    mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver) {
  local_ai_receivers_.Add(this, std::move(receiver));
}

base::WeakPtr<BraveBatchPassageEmbedder>
BraveBatchPassageEmbedder::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

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

void BraveBatchPassageEmbedder::RegisterPassageEmbedderFactory(
    mojo::PendingRemote<local_ai::mojom::PassageEmbedderFactory> factory) {
  // A buggy or compromised renderer could call this twice without first
  // triggering a disconnect, or call it after the renderer-side
  // PassageEmbedder pipe is already up. mojo::Remote::Bind CHECKs when
  // already bound; the phase guard rejects any call past
  // kAwaitingFactory so duplicates can't crash the browser.
  if (phase_ != LoadPhase::kAwaitingFactory || factory_.is_bound()) {
    DVLOG(1) << "Ignoring RegisterPassageEmbedderFactory in phase="
             << static_cast<int>(phase_);
    return;
  }
  factory_.Bind(std::move(factory));
  factory_.set_disconnect_handler(
      base::BindOnce(&BraveBatchPassageEmbedder::OnDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  MaybeStartInit();
}

void BraveBatchPassageEmbedder::OnBackgroundContentsDestroyed(
    local_ai::BackgroundWebContents::DestroyReason reason) {
  DVLOG(1) << "Background contents destroyed unexpectedly, reason="
           << static_cast<int>(reason);
  background_web_contents_.reset();
  OnDisconnected();
}

void BraveBatchPassageEmbedder::OnBackgroundContentsCreated(
    std::unique_ptr<local_ai::BackgroundWebContents> contents) {
  if (phase_ != LoadPhase::kCreatingContents) {
    DVLOG(1) << "OnBackgroundContentsCreated ignored in phase="
             << static_cast<int>(phase_);
    return;
  }
  if (!contents) {
    DVLOG(1) << "Background contents creation failed";
    OnDisconnected();
    return;
  }
  background_web_contents_ = std::move(contents);
  phase_ = LoadPhase::kAwaitingFactory;
  MaybeStartInit();
}

void BraveBatchPassageEmbedder::MaybeStartInit() {
  if (phase_ != LoadPhase::kAwaitingFactory || !factory_.is_bound()) {
    return;
  }
  phase_ = LoadPhase::kInitializing;
  factory_->Init(std::move(model_files_),
                 base::BindOnce(&BraveBatchPassageEmbedder::OnInitDone,
                                weak_ptr_factory_.GetWeakPtr()));
}

void BraveBatchPassageEmbedder::OnInitDone(bool success) {
  if (phase_ != LoadPhase::kInitializing) {
    DVLOG(1) << "OnInitDone ignored in phase=" << static_cast<int>(phase_);
    return;
  }
  if (!success) {
    DVLOG(1) << "Factory Init failed";
    OnDisconnected();
    return;
  }
  factory_->Bind(renderer_embedder_.BindNewPipeAndPassReceiver());
  renderer_embedder_.set_disconnect_handler(
      base::BindOnce(&BraveBatchPassageEmbedder::OnDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  // Now that the renderer-side pipe is up, bind the caller-side
  // receiver. Any GenerateEmbeddings calls already on the wire drain
  // here.
  receiver_.Bind(std::move(pending_receiver_));
  receiver_.set_disconnect_handler(
      base::BindOnce(&BraveBatchPassageEmbedder::OnDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  phase_ = LoadPhase::kReady;
  DVLOG(3) << "Factory Init succeeded; signaling load success";
  std::move(load_callback_).Run(true);
}

void BraveBatchPassageEmbedder::ProcessNext() {
  CHECK(!current_batch_);
  if (pending_batches_.empty()) {
    return;
  }
  current_batch_.emplace(std::move(pending_batches_.front()));
  pending_batches_.pop_front();

  // GenerateEmbeddings can only be delivered after OnInitDone bound
  // receiver_, which also binds renderer_embedder_.
  CHECK(renderer_embedder_.is_bound());
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
  // If Init hasn't replied yet, fail the outstanding load.
  if (load_callback_) {
    std::move(load_callback_).Run(false);
  }

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
