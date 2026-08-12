// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/passage_embeddings/litert_model_runner.h"

#include <type_traits>
#include <utility>

#include "base/containers/extend.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "third_party/litert/src/litert/cc/litert_element_type.h"
#include "third_party/litert/src/litert/cc/litert_environment_options.h"
#include "third_party/litert/src/litert/cc/litert_layout.h"
#include "third_party/litert/src/litert/cc/litert_options.h"
#include "third_party/litert/src/litert/cc/litert_ranked_tensor_type.h"
#include "third_party/litert/src/litert/cc/litert_tensor_buffer.h"
#include "third_party/litert/src/litert/cc/options/litert_cpu_options.h"

namespace brave_history_embeddings {

LitertModelRunner::LitertModelRunner() = default;

LitertModelRunner::~LitertModelRunner() = default;

// static
std::unique_ptr<LitertModelRunner> LitertModelRunner::Create(
    base::span<const uint8_t> tflite_model,
    int num_threads) {
  auto runner = base::WrapUnique(new LitertModelRunner());
  runner->owned_model_.assign(tflite_model.begin(), tflite_model.end());
  if (!runner->Init(runner->owned_model_, num_threads)) {
    return nullptr;
  }
  return runner;
}

// static
std::unique_ptr<LitertModelRunner> LitertModelRunner::CreateFromFile(
    base::File& model_file,
    int num_threads) {
  if (!model_file.IsValid()) {
    return nullptr;
  }
  auto runner = base::WrapUnique(new LitertModelRunner());
  // Duplicate because the caller keeps ownership of `model_file`; the mapping
  // needs a descriptor of its own for as long as the runner lives.
  if (!runner->mapped_model_.Initialize(model_file.Duplicate())) {
    return nullptr;
  }
  if (!runner->Init(runner->mapped_model_.bytes(), num_threads)) {
    return nullptr;
  }
  return runner;
}

bool LitertModelRunner::Init(base::span<const uint8_t> tflite_model,
                             int num_threads) {
  std::vector<litert::EnvironmentOptions::Option> env_options;
  auto environment = litert::Environment::Create(litert::EnvironmentOptions(
      litert::Span<const litert::EnvironmentOptions::Option>(
          env_options.data(), env_options.size())));
  if (!environment) {
    LOG(ERROR) << "LiteRT runner: cannot create environment: "
               << environment.Error().Message();
    return false;
  }
  environment_ = std::move(*environment);

  auto compile_options = litert::Options::Create();
  if (!compile_options) {
    return false;
  }
  compile_options->SetHardwareAccelerators(litert::HwAccelerators::kCpu);

  // XNNPACK sizes its intra-op thread pool from this; LiteRT defaults to a
  // single thread when no CPU options are attached.
  auto cpu_options = compile_options->GetCpuOptions();
  if (!cpu_options) {
    LOG(ERROR) << "LiteRT runner: cannot create CPU options: "
               << cpu_options.Error().Message();
    return false;
  }
  if (!cpu_options->SetNumThreads(num_threads)) {
    LOG(ERROR) << "LiteRT runner: rejected num_threads=" << num_threads;
    return false;
  }

  auto model = litert::CompiledModel::Create(
      *environment_,
      litert::BufferRef<uint8_t>(tflite_model.data(), tflite_model.size()),
      *compile_options);
  if (!model) {
    LOG(ERROR) << "LiteRT runner: CompiledModel::Create failed: "
               << model.Error().Message();
    return false;
  }
  model_ = std::move(*model);

  auto input_buffers = model_->CreateInputBuffers();
  if (!input_buffers || input_buffers->size() != 1) {
    return false;
  }
  auto input_type = (*input_buffers)[0].TensorType();
  if (!input_type) {
    return false;
  }
  // Every EmbeddingGemma export takes int32 token ids, and Write() memcpy's
  // after only a size check, so tokens written into a wider input would
  // silently be garbage. The model ships via the component updater,
  // independently of this binary, so reject a mismatched one here rather than
  // failing every request later.
  static_assert(std::is_same_v<int, int32_t>);
  if (input_type->ElementType() != litert::ElementType::Int32) {
    LOG(ERROR) << "LiteRT runner: model input is not int32";
    return false;
  }
  auto num_elements = input_type->Layout().NumElements();
  if (!num_elements) {
    return false;
  }
  input_window_size_ = *num_elements;
  return true;
}

std::optional<std::vector<float>> LitertModelRunner::Run(
    base::span<const int> tokens) {
  // Callers must lay `tokens` out for the model's input window; reject anything
  // else rather than writing a wrongly-sized buffer.
  if (tokens.size() != input_window_size_) {
    return std::nullopt;
  }

  // CreateInput/OutputBuffers hand back buffers already sized and typed for the
  // model (host or accelerator memory as the delegate requires); Write/Read
  // copy through them, so no manual allocation is needed.
  auto inputs = model_->CreateInputBuffers();
  auto outputs = model_->CreateOutputBuffers();
  if (!inputs || !outputs || inputs->size() != 1) {
    return std::nullopt;
  }

  // Init() verified the input is int32, so the tokens can be written as-is.
  if (!(*inputs)[0].Write<int>(
          litert::Span<const int>(tokens.data(), tokens.size()))) {
    return std::nullopt;
  }

  // Synchronous Run waits for accelerator completion internally.
  if (!model_->Run(*inputs, *outputs)) {
    return std::nullopt;
  }

  std::vector<float> embedding;
  for (litert::TensorBuffer& buf : *outputs) {
    auto type = buf.TensorType();
    if (!type || (*type).ElementType() != litert::ElementType::Float32) {
      continue;
    }
    auto packed = buf.PackedSize();
    if (!packed) {
      return std::nullopt;
    }
    std::vector<float> values(*packed / sizeof(float));
    if (!buf.Read<float>(litert::Span<float>(values.data(), values.size()))) {
      return std::nullopt;
    }
    base::Extend(embedding, values);
  }
  if (embedding.empty()) {
    return std::nullopt;
  }
  return embedding;
}

}  // namespace brave_history_embeddings
