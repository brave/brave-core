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
#include "base/files/memory_mapped_file.h"
#include "third_party/litert/src/litert/cc/litert_compiled_model.h"
#include "third_party/litert/src/litert/cc/litert_environment.h"

namespace brave_history_embeddings {

// Runs an embeddings model through LiteRT's CompiledModel on the CPU. This is
// the execution engine only: it knows nothing about
// //services/passage_embeddings, which owns tokenization and caching. Callers
// pass tokens already laid out for the model's input window (e.g. via
// GemmaModelExecutor::FormatInput); the runner just writes them and runs the
// model.
class LitertModelRunner {
 public:
  // Builds from the in-memory `.tflite`; nullptr on failure. `num_threads`
  // sizes the CPU backend's intra-op thread pool and is fixed for the life of
  // the runner, because LiteRT bakes it into the model at compile time.
  static std::unique_ptr<LitertModelRunner> Create(
      base::span<const uint8_t> tflite_model,
      int num_threads);

  // Maps the `.tflite` in `model_file` and builds a runner for it, or returns
  // nullptr if the file or the model is unusable.
  static std::unique_ptr<LitertModelRunner> CreateFromFile(
      base::File& model_file,
      int num_threads);

  ~LitertModelRunner();

  LitertModelRunner(const LitertModelRunner&) = delete;
  LitertModelRunner& operator=(const LitertModelRunner&) = delete;

  // Runs one window of `tokens` (sized to window()); returns the pooled Float32
  // embedding, or nullopt on failure.
  std::optional<std::vector<float>> Run(base::span<const int> tokens);

  // The model's input token window (for reporting and input sizing).
  size_t window() const { return input_window_size_; }

 private:
  LitertModelRunner();

  // `tflite_model` is aliased, not copied: litert wraps the pointer for as long
  // as model_ lives. Callers pass `mapped_model_` or `owned_model_`.
  bool Init(base::span<const uint8_t> tflite_model, int num_threads);

  // Backing storage for the model bytes; exactly one is populated. Keep these
  // declared before model_, which aliases them and so must be destroyed first.
  // The mapping keeps the model out of anonymous memory and makes recompiling
  // it for a new thread count cheap.
  base::MemoryMappedFile mapped_model_;
  std::vector<uint8_t> owned_model_;
  std::optional<litert::Environment> environment_;
  std::optional<litert::CompiledModel> model_;
  size_t input_window_size_ = 0;
};

}  // namespace brave_history_embeddings

#endif  // BRAVE_SERVICES_PASSAGE_EMBEDDINGS_LITERT_MODEL_RUNNER_H_
