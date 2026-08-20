/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"

namespace base {
class File;
}  // namespace base

namespace passage_embeddings {
class PassageEmbedderExecutor;
}  // namespace passage_embeddings

namespace brave_history_embeddings {

namespace {

// Resolves the thread count the way upstream's BuildExecutionTask does, so the
// LiteRT runner only ever sees a count rather than a passage priority.
int NumThreadsForPriority(passage_embeddings::mojom::PassagePriority priority,
                          int user_initiated_num_threads,
                          int urgent_num_threads,
                          int passive_num_threads) {
  switch (priority) {
    case passage_embeddings::mojom::PassagePriority::kUserInitiated:
      return user_initiated_num_threads;
    case passage_embeddings::mojom::PassagePriority::kUrgent:
      return urgent_num_threads;
    case passage_embeddings::mojom::PassagePriority::kPassive:
      return passive_num_threads;
    case passage_embeddings::mojom::PassagePriority::kUnknown:
      // BuildExecutionTask CHECKs against this; the case keeps the switch
      // exhaustive.
      return 1;
  }
}

}  // namespace

// Implemented in //brave/services/passage_embeddings:chromium_impl. Returns
// null when the LiteRT model is unavailable, or when local AI is disabled.
std::unique_ptr<passage_embeddings::PassageEmbedderExecutor>
MaybeCreateLitertExecutor(base::File& embeddings_model_file,
                          int bos_id,
                          int eos_id,
                          int pad_id,
                          int num_threads);

}  // namespace brave_history_embeddings

// Injected at the top of PassageEmbedderImpl::BuildExecutionTask so
// EmbeddingGemma runs through LiteRT's CompiledModel, falling through to the
// upstream tflite executor otherwise. Returning here skips the thread count
// upstream applies via InitInterpreter below, so it is resolved here and passed
// to the runner, which compiles the model with it.
//
// The patch must keep this after executor_.reset(): the executor being replaced
// borrows the cached runner that MaybeCreateLitertExecutor may free.
#define BRAVE_PASSAGE_EMBEDDER_IMPL_BUILD_EXECUTION_TASK                   \
  if (std::unique_ptr<PassageEmbedderExecutor> executor =                  \
          brave_history_embeddings::MaybeCreateLitertExecutor(             \
              embeddings_model_file_, sp_processor_->bos_id(),             \
              sp_processor_->eos_id(), sp_processor_->pad_id(),            \
              brave_history_embeddings::NumThreadsForPriority(             \
                  current_priority_, user_initiated_priority_num_threads_, \
                  urgent_priority_num_threads_,                            \
                  passive_priority_num_threads_))) {                       \
    executor_ = std::move(executor);                                       \
    return true;                                                           \
  }

#include <services/passage_embeddings/passage_embedder_impl.cc>

#undef BRAVE_PASSAGE_EMBEDDER_IMPL_BUILD_EXECUTION_TASK
