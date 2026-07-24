/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/no_destructor.h"
#include "brave/services/passage_embeddings/litert_model_runner.h"
#include "services/passage_embeddings/passage_embedder_executor.h"

namespace brave_history_embeddings {

namespace {

// Wraps the LiteRT GPU runner in upstream's PassageEmbedderExecutor interface.
// Defined here (compiled into the same TU as the upstream .cc via the include
// below) so it can subclass the upstream interface while the runner target
// stays free of any dependency on //services/passage_embeddings.
class LitertExecutor : public passage_embeddings::PassageEmbedderExecutor {
 public:
  explicit LitertExecutor(LitertModelRunner& runner) : runner_(runner) {}
  ~LitertExecutor() override = default;

  std::optional<passage_embeddings::EmbedderExecutionResult> Execute(
      const std::vector<int>& raw_tokens) override {
    std::optional<std::vector<float>> embedding = runner_->Run(raw_tokens);
    if (!embedding) {
      return std::nullopt;
    }
    passage_embeddings::EmbedderExecutionResult result;
    result.embeddings = std::move(*embedding);
    result.signature_length = static_cast<uint32_t>(runner_->window());
    return result;
  }

 private:
  // Process-global (see below); outlives this executor, so a ref is safe.
  const raw_ref<LitertModelRunner> runner_;
};

}  // namespace

std::unique_ptr<passage_embeddings::PassageEmbedderExecutor>
MaybeCreateLitertExecutor(base::File& embeddings_model_file,
                          int bos_id,
                          int eos_id,
                          int pad_id) {
  // The runner owns the CompiledModel + Metal/WebGPU environment, which are
  // expensive to build (compile + accelerator dlopen). PassageEmbedderImpl
  // rebuilds its executor on every priority change, and a fresh instance is
  // created whenever the embedder is torn down and reloaded -- so build the
  // runner once per process and hand out lightweight, non-owning executors that
  // reuse it, instead of recompiling the model on every query. All calls arrive
  // on the service's single task-runner sequence, so the lazy init needs no
  // extra locking; one model per process means a single cached runner.
  static base::NoDestructor<std::unique_ptr<LitertModelRunner>> runner(
      MaybeCreateLitertModelRunner(embeddings_model_file, bos_id, eos_id,
                                   pad_id));
  if (!runner->get()) {
    return nullptr;
  }
  return std::make_unique<LitertExecutor>(*runner->get());
}

}  // namespace brave_history_embeddings

// Injected at the top of PassageEmbedderImpl::BuildExecutionTask so
// EmbeddingGemma runs on the GPU via LiteRT when the accelerator is present,
// falling through to the upstream tflite executor otherwise.
#define BRAVE_PASSAGE_EMBEDDER_IMPL_BUILD_EXECUTION_TASK           \
  if (std::unique_ptr<PassageEmbedderExecutor> executor =          \
          brave_history_embeddings::MaybeCreateLitertExecutor(     \
              embeddings_model_file_, sp_processor_->bos_id(),     \
              sp_processor_->eos_id(), sp_processor_->pad_id())) { \
    executor_ = std::move(executor);                               \
    return true;                                                   \
  }

#else  // BUILDFLAG(ENABLE_LOCAL_AI)

#define BRAVE_PASSAGE_EMBEDDER_IMPL_BUILD_EXECUTION_TASK

#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

#include <services/passage_embeddings/passage_embedder_impl.cc>

#undef BRAVE_PASSAGE_EMBEDDER_IMPL_BUILD_EXECUTION_TASK
