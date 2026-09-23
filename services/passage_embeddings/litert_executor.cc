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
  // The process-global runner cached in MaybeCreateLitertExecutor(), which is
  // only replaced while no executor holds it, so a ref is safe.
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
                          int pad_id,
                          int num_threads) {
  // Upstream rebuilds its executor on every priority change, and LiteRT bakes
  // the thread count into the compiled model, so cache one runner and let
  // executors borrow it, replacing it when the count changes. Replacing cannot
  // strand a live borrow, because PassageEmbedderImpl drops its executor before
  // calling this. Only the service's task runner sequence gets here, so the
  // cache needs no locking.
  struct CachedRunner {
    std::unique_ptr<LitertModelRunner> runner;
    int num_threads = 0;
  };
  static base::NoDestructor<CachedRunner> cached;
  if (!cached->runner || cached->num_threads != num_threads) {
    // A compiled model holds XNNPACK's repacked copy of the weights. Assigning
    // would keep the old one alive while the replacement compiles, so free it
    // first.
    cached->runner.reset();
    cached->runner =
        LitertModelRunner::CreateFromFile(embeddings_model_file, num_threads);
    cached->num_threads = num_threads;
  }
  if (!cached->runner) {
    return nullptr;
  }
  return std::make_unique<LitertExecutor>(*cached->runner, bos_id, eos_id,
                                          pad_id);
}

}  // namespace brave_history_embeddings
