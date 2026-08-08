/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/services/passage_embeddings/litert_executor-forward.inc"

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
