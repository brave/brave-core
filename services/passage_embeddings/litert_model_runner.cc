// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/passage_embeddings/litert_model_runner.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/base_paths.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "third_party/litert/src/litert/cc/litert_element_type.h"
#include "third_party/litert/src/litert/cc/litert_environment_options.h"
#include "third_party/litert/src/litert/cc/litert_layout.h"
#include "third_party/litert/src/litert/cc/litert_options.h"
#include "third_party/litert/src/litert/cc/litert_ranked_tensor_type.h"
#include "third_party/litert/src/litert/cc/litert_tensor_buffer.h"
#include "third_party/litert/src/litert/cc/options/litert_gpu_options.h"

#if BUILDFLAG(IS_MAC)
#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#endif

namespace brave_history_embeddings {

namespace {

// [bos] + [eos] control tokens, matching upstream GemmaModelExecutor.
constexpr size_t kControlTokensCount = 2;

// Forces the CPU backend even when the GPU accelerator is bundled, so the CPU
// path can be exercised on GPU-capable machines. Disabled by default.
BASE_FEATURE(kBraveHistoryEmbeddingsLitertForceCpu,
             "BraveHistoryEmbeddingsLitertForceCpu",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Directory holding the prebuilt accelerator (see //brave/third_party/litert):
// the framework's Libraries dir when bundled on macOS, else the module dir.
base::FilePath GetAcceleratorLibraryDir() {
#if BUILDFLAG(IS_MAC)
  if (base::apple::AmIBundled()) {
    return base::apple::FrameworkBundlePath().Append("Libraries");
  }
#endif
  return base::PathService::CheckedGet(base::DIR_MODULE);
}

// The accelerator is Metal on macOS, WebGPU on Linux/Windows.
bool AcceleratorPresent(const base::FilePath& dir) {
#if BUILDFLAG(IS_MAC)
  static constexpr char kAcceleratorName[] = "LiteRtMetalAccelerator";
#elif BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
  static constexpr char kAcceleratorName[] = "LiteRtWebGpuAccelerator";
#else
  return false;
#endif
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
  return !dir.empty() && base::PathExists(dir.AppendASCII(
                             base::GetNativeLibraryName(kAcceleratorName)));
#endif
}

// Writable directory for the serialized GPU program cache, keyed off the
// macOS Metal / ML Drift compiler. Reachable by the sandboxed service only when
// the passage-embeddings utility is launched with the user-dir access switch
// (see LitertServiceLauncher and the sandbox_parameters_mac.mm patch).
base::FilePath GetProgramCacheDir() {
  base::FilePath temp_dir;
  if (!base::GetTempDir(&temp_dir)) {
    return base::FilePath();
  }
  return temp_dir.AppendASCII("brave_litert_gpu_cache");
}

}  // namespace

LitertModelRunner::LitertModelRunner() = default;

LitertModelRunner::~LitertModelRunner() = default;

// static
std::unique_ptr<LitertModelRunner> LitertModelRunner::Create(
    base::span<const uint8_t> tflite_model,
    bool use_gpu,
    const base::FilePath& gpu_runtime_lib_dir,
    int bos_id,
    int eos_id,
    int pad_id) {
  auto runner = base::WrapUnique(new LitertModelRunner());
  runner->bos_id_ = bos_id;
  runner->eos_id_ = eos_id;
  runner->pad_id_ = pad_id >= 0 ? pad_id : 0;
  if (!runner->Init(tflite_model, use_gpu, gpu_runtime_lib_dir)) {
    return nullptr;
  }
  return runner;
}

bool LitertModelRunner::Init(base::span<const uint8_t> tflite_model,
                             bool use_gpu,
                             const base::FilePath& gpu_runtime_lib_dir) {
  // The runtime references the model bytes past compilation; keep our own copy.
  tflite_model_.assign(tflite_model.begin(), tflite_model.end());

  std::vector<litert::EnvironmentOptions::Option> env_options;
  const std::string runtime_lib_dir = gpu_runtime_lib_dir.AsUTF8Unsafe();
  if (use_gpu && !runtime_lib_dir.empty()) {
    env_options.emplace_back(
        litert::EnvironmentOptions::Tag::kRuntimeLibraryDir,
        runtime_lib_dir.c_str());
  }
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
  // Kept alive until CompiledModel::Create() below: the GPU options store the
  // pointers without taking ownership.
  std::string cache_dir;
  std::string model_cache_key;
  if (use_gpu) {
    compile_options->SetHardwareAccelerators(litert::HwAccelerators::kGpu);
    auto gpu_options = compile_options->GetGpuOptions();
    if (!gpu_options) {
      return false;
    }
    // EmbeddingGemma is mixed-precision (quantized FC/Conv weights); both knobs
    // are required for correct GPU output (fp16 collapses to zero).
    gpu_options->EnableAllowSrcQuantizedFcConvOps(true);
    if (!gpu_options->SetPrecision(litert::GpuOptions::Precision::kFp32)) {
      return false;
    }
    // Persist the compiled program so the expensive shader compilation only
    // happens on the first launch; a fresh process then loads it from disk
    // instead of recompiling. Best effort: if the dir is not writable the
    // delegate simply skips serialization and compiles as before.
    const base::FilePath dir = GetProgramCacheDir();
    if (!dir.empty() && base::CreateDirectory(dir)) {
      cache_dir = dir.AsUTF8Unsafe();
      model_cache_key = base::NumberToString(
          static_cast<uint64_t>(base::FastHash(tflite_model_)));
      gpu_options->SetSerializationDir(cache_dir.c_str());
      gpu_options->SetModelCacheKey(model_cache_key.c_str());
      gpu_options->SetSerializeProgramCache(true);
    }
  } else {
    compile_options->SetHardwareAccelerators(litert::HwAccelerators::kCpu);
  }

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
  // Use the GPU accelerator when it is bundled for this platform/arch,
  // otherwise run the same model on LiteRT's CPU backend. GPU is unavailable on
  // arches with no accelerator prebuilt (x64 macOS, Windows arm64); the CPU
  // backend is compiled from source everywhere, so LiteRT still owns execution
  // rather than falling back to upstream's tflite executor.
  const base::FilePath accelerator_dir = GetAcceleratorLibraryDir();
  const bool use_gpu =
      AcceleratorPresent(accelerator_dir) &&
      !base::FeatureList::IsEnabled(kBraveHistoryEmbeddingsLitertForceCpu);
  const int64_t length = embeddings_model_file.GetLength();
  if (length <= 0) {
    return nullptr;
  }
  std::vector<uint8_t> bytes(static_cast<size_t>(length));
  if (!embeddings_model_file.ReadAndCheck(0, bytes)) {
    return nullptr;
  }
  return LitertModelRunner::Create(bytes, use_gpu, accelerator_dir, bos_id,
                                   eos_id, pad_id);
}

}  // namespace brave_history_embeddings
