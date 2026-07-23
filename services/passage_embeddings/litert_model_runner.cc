// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/passage_embeddings/litert_model_runner.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "third_party/litert/src/litert/cc/litert_element_type.h"
#include "third_party/litert/src/litert/cc/litert_environment_options.h"
#include "third_party/litert/src/litert/cc/litert_layout.h"
#include "third_party/litert/src/litert/cc/litert_options.h"
#include "third_party/litert/src/litert/cc/litert_ranked_tensor_type.h"
#include "third_party/litert/src/litert/cc/litert_tensor_buffer.h"

namespace brave_history_embeddings {

namespace {

// [bos] + [eos] control tokens, matching upstream GemmaModelExecutor.
constexpr size_t kControlTokensCount = 2;

}  // namespace

LitertModelRunner::LitertModelRunner() = default;

LitertModelRunner::~LitertModelRunner() = default;

// static
std::unique_ptr<LitertModelRunner> LitertModelRunner::Create(
    base::span<const uint8_t> tflite_model,
    int bos_id,
    int eos_id,
    int pad_id) {
  auto runner = base::WrapUnique(new LitertModelRunner());
  runner->bos_id_ = bos_id;
  runner->eos_id_ = eos_id;
  runner->pad_id_ = pad_id >= 0 ? pad_id : 0;
  if (!runner->Init(tflite_model)) {
    return nullptr;
  }
  return runner;
}

bool LitertModelRunner::Init(base::span<const uint8_t> tflite_model) {
  // The runtime references the model bytes past compilation; keep our own copy.
  tflite_model_.assign(tflite_model.begin(), tflite_model.end());

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

  auto model = litert::CompiledModel::Create(
      *environment_,
      litert::BufferRef<uint8_t>(tflite_model_.data(), tflite_model_.size()),
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
  auto num_elements = input_type->Layout().NumElements();
  if (!num_elements) {
    return false;
  }
  input_window_size_ = *num_elements;
  return true;
}

std::vector<int32_t> LitertModelRunner::FormatInput(
    const std::vector<int>& raw_tokens) const {
  std::vector<int32_t> out;
  out.reserve(input_window_size_);
  out.push_back(bos_id_);
  const size_t max_content = input_window_size_ - kControlTokensCount;
  for (size_t i = 0; i < std::min(raw_tokens.size(), max_content); ++i) {
    out.push_back(raw_tokens[i]);
  }
  out.push_back(eos_id_);
  out.resize(input_window_size_, pad_id_);
  return out;
}

std::optional<std::vector<float>> LitertModelRunner::Run(
    const std::vector<int>& raw_tokens) {
  if (input_window_size_ < kControlTokensCount) {
    return std::nullopt;
  }
  const std::vector<int32_t> tokens = FormatInput(raw_tokens);

  // CreateInput/OutputBuffers hand back buffers already sized and typed for the
  // model (host or accelerator memory as the delegate requires); Write/Read
  // copy through them, so no manual allocation is needed.
  auto inputs = model_->CreateInputBuffers();
  auto outputs = model_->CreateOutputBuffers();
  if (!inputs || !outputs || inputs->size() != 1) {
    return std::nullopt;
  }

  auto input_type = (*inputs)[0].TensorType();
  if (!input_type) {
    return std::nullopt;
  }
  if ((*input_type).ElementType() == litert::ElementType::Int64) {
    std::vector<int64_t> tokens64(tokens.begin(), tokens.end());
    if (!(*inputs)[0].Write<int64_t>(
            litert::Span<const int64_t>(tokens64.data(), tokens64.size()))) {
      return std::nullopt;
    }
  } else if (!(*inputs)[0].Write<int32_t>(
                 litert::Span<const int32_t>(tokens.data(), tokens.size()))) {
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
    embedding.insert(embedding.end(), values.begin(), values.end());
  }
  if (embedding.empty()) {
    return std::nullopt;
  }
  return embedding;
}

std::unique_ptr<LitertModelRunner> MaybeCreateLitertModelRunner(
    base::File& embeddings_model_file,
    int bos_id,
    int eos_id,
    int pad_id) {
  // Runs the model on LiteRT's CPU backend. LiteRT is compiled from source on
  // every platform, so it owns execution rather than falling back to upstream's
  // tflite executor.
  const int64_t length = embeddings_model_file.GetLength();
  if (length <= 0) {
    return nullptr;
  }
  std::vector<uint8_t> bytes(static_cast<size_t>(length));
  if (!embeddings_model_file.ReadAndCheck(0, bytes)) {
    return nullptr;
  }
  return LitertModelRunner::Create(bytes, bos_id, eos_id, pad_id);
}

}  // namespace brave_history_embeddings
