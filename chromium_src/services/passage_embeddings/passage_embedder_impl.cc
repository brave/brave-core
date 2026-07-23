/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

namespace base {
class File;
}  // namespace base

namespace passage_embeddings {
class PassageEmbedderExecutor;
}  // namespace passage_embeddings

namespace brave_history_embeddings {

// Implemented in //brave/services/passage_embeddings:chromium_impl. Returns
// null when the LiteRT model is unavailable, or when local AI is disabled.
std::unique_ptr<passage_embeddings::PassageEmbedderExecutor>
MaybeCreateLitertExecutor(base::File& embeddings_model_file,
                          int bos_id,
                          int eos_id,
                          int pad_id);

}  // namespace brave_history_embeddings

// Injected at the top of PassageEmbedderImpl::BuildExecutionTask so
// EmbeddingGemma runs through LiteRT's CompiledModel, falling through to the
// upstream tflite executor otherwise.
#define BRAVE_PASSAGE_EMBEDDER_IMPL_BUILD_EXECUTION_TASK           \
  if (std::unique_ptr<PassageEmbedderExecutor> executor =          \
          brave_history_embeddings::MaybeCreateLitertExecutor(     \
              embeddings_model_file_, sp_processor_->bos_id(),     \
              sp_processor_->eos_id(), sp_processor_->pad_id())) { \
    executor_ = std::move(executor);                               \
    return true;                                                   \
  }

#include <services/passage_embeddings/passage_embedder_impl.cc>

#undef BRAVE_PASSAGE_EMBEDDER_IMPL_BUILD_EXECUTION_TASK
