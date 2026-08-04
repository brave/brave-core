/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/file.h"
#include "base/memory/raw_ref.h"
#include "base/no_destructor.h"
#include "brave/services/passage_embeddings/litert_model_runner.h"
#include "services/passage_embeddings/passage_embedder_executor.h"

namespace brave_history_embeddings {

namespace {

// Adapts the LiteRT runner to upstream's PassageEmbedderExecutor interface.
class LitertExecutor : public passage_embeddings::PassageEmbedderExecutor {
 public:
  LitertExecutor(LitertModelRunner& runner, int bos_id, int eos_id, int pad_id)
      : runner_(runner), bos_id_(bos_id), eos_id_(eos_id), pad_id_(pad_id) {}
  ~LitertExecutor() override = default;

  std::optional<passage_embeddings::EmbedderExecutionResult> Execute(
      const std::vector<int>& raw_tokens) override {
    // Lay the SentencePiece content ids out for the model's input window using
    // upstream's Gemma formatter ([bos] + tokens + [eos] + pad).
    std::vector<int> tokens =
        passage_embeddings::GemmaModelExecutor::FormatInput(
            raw_tokens, runner_->window(), bos_id_, eos_id_, pad_id_);
    std::optional<std::vector<float>> embedding = runner_->Run(tokens);
    if (!embedding) {
      return std::nullopt;
    }
    passage_embeddings::EmbedderExecutionResult result;
    result.embeddings = std::move(*embedding);
    result.signature_length = static_cast<uint32_t>(runner_->window());
    return result;
  }

 private:
  // The process-global runner cached in MaybeCreateLitertExecutor(); outlives
  // this executor, so a ref is safe.
  const raw_ref<LitertModelRunner> runner_;
  const int bos_id_;
  const int eos_id_;
  const int pad_id_;
};

}  // namespace

std::unique_ptr<passage_embeddings::PassageEmbedderExecutor>
MaybeCreateLitertExecutor(base::File& embeddings_model_file,
                          int bos_id,
                          int eos_id,
                          int pad_id) {
  // Building a runner compiles the model, and PassageEmbedderImpl rebuilds its
  // executor on every priority change, so cache one runner per process and hand
  // out lightweight executors that borrow it. All calls arrive on the service's
  // single task-runner sequence, so the lazy init needs no locking.
  static base::NoDestructor<std::unique_ptr<LitertModelRunner>> runner(
      LitertModelRunner::CreateFromFile(embeddings_model_file));
  if (!runner->get()) {
    return nullptr;
  }
  return std::make_unique<LitertExecutor>(*runner->get(), bos_id, eos_id,
                                          pad_id);
}

}  // namespace brave_history_embeddings
