// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_SERVICES_PASSAGE_EMBEDDINGS_LITERT_MODEL_RUNNER_H_
#define BRAVE_SERVICES_PASSAGE_EMBEDDINGS_LITERT_MODEL_RUNNER_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "third_party/litert/src/litert/cc/litert_compiled_model.h"
#include "third_party/litert/src/litert/cc/litert_environment.h"

namespace brave_history_embeddings {

// Runs an embeddings model through LiteRT's CompiledModel on the GPU (or CPU).
// This is the execution engine only -- it deliberately does NOT depend on
// //services/passage_embeddings, so upstream's PassageEmbedderImpl (which owns
// tokenization/caching) can wrap it in a thin PassageEmbedderExecutor adapter
// without a dependency cycle. Input `raw_tokens` are SentencePiece content ids;
// this applies the [bos] + tokens + [eos] + pad layout before running, matching
// GemmaModelExecutor.
class LitertModelRunner {
 public:
  // Builds from the in-memory `.tflite`. GPU is used only when `use_gpu` and
  // the accelerator is present in `gpu_runtime_lib_dir`; nullptr on failure.
  static std::unique_ptr<LitertModelRunner> Create(
      base::span<const uint8_t> tflite_model,
      bool use_gpu,
      const base::FilePath& gpu_runtime_lib_dir,
      int bos_id,
      int eos_id,
      int pad_id);

  ~LitertModelRunner();

  LitertModelRunner(const LitertModelRunner&) = delete;
  LitertModelRunner& operator=(const LitertModelRunner&) = delete;

  // Formats + runs one passage; returns the pooled Float32 embedding, or
  // nullopt on failure.
  std::optional<std::vector<float>> Run(const std::vector<int>& raw_tokens);

  // The model's input token window (for reporting).
  size_t window() const { return input_window_size_; }

 private:
  LitertModelRunner();

  bool Init(base::span<const uint8_t> tflite_model,
            bool use_gpu,
            const base::FilePath& gpu_runtime_lib_dir);

  std::vector<int32_t> FormatInput(const std::vector<int>& raw_tokens) const;

  std::vector<uint8_t> tflite_model_;
  std::optional<litert::Environment> environment_;
  std::optional<litert::CompiledModel> model_;
  size_t input_window_size_ = 0;
  int bos_id_ = 0;
  int eos_id_ = 0;
  int pad_id_ = 0;
};

// Factory for upstream PassageEmbedderImpl::BuildExecutionTask: returns a
// runner when the prebuilt accelerator is bundled next to the executable
// (reading the model from `embeddings_model_file` and taking the SentencePiece
// control-token ids), else nullptr so the caller uses its own CPU executor.
std::unique_ptr<LitertModelRunner> MaybeCreateLitertModelRunner(
    base::File& embeddings_model_file,
    int bos_id,
    int eos_id,
    int pad_id);

}  // namespace brave_history_embeddings

#endif  // BRAVE_SERVICES_PASSAGE_EMBEDDINGS_LITERT_MODEL_RUNNER_H_
