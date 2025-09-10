// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/fast_vlm_executor.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <queue>
#include <sstream>
#include <unordered_map>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/components/local_ai/ort/environment.h"
#include "brave/components/local_ai/ort/ort_status.h"
#include "brave/components/local_ai/ort/platform_functions_ort.h"
#include "brave/components/local_ai/ort/scoped_ort_types.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/cpp/decode_image.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSamplingOptions.h"

namespace {

data_decoder::DataDecoder* GetDataDecoder() {
  static base::NoDestructor<data_decoder::DataDecoder> data_decoder;
  return data_decoder.get();
}

// Scale and crop bitmap to specified size (free function used by lambda)
SkBitmap ScaleFastVLMBitmap(const SkBitmap& bitmap,
                            int target_width,
                            int target_height) {
  // Validate target dimensions to avoid negative dimensions
  if (target_width <= 0 || target_height <= 0) {
    LOG(ERROR) << "Invalid target dimensions: " << target_width << "x"
               << target_height << ", using default 1024x1024";
    target_width = target_height = 1024;  // Fallback to default
  }

  // If already the right size, return as-is
  if (bitmap.width() == target_width && bitmap.height() == target_height) {
    return bitmap;
  }

  SkBitmap scaled_bitmap;
  scaled_bitmap.allocN32Pixels(target_width, target_height);

  SkCanvas canvas(scaled_bitmap);
  canvas.clear(SK_ColorTRANSPARENT);

  // Use high-quality scaling options
  SkSamplingOptions sampling_options(SkFilterMode::kLinear,
                                     SkMipmapMode::kLinear);

  // FastVLM preprocessing: scale to fit target dimensions while preserving
  // aspect ratio
  float scale_x = static_cast<float>(target_width) / bitmap.width();
  float scale_y = static_cast<float>(target_height) / bitmap.height();
  float scale_factor =
      std::min(scale_x, scale_y);  // Scale to fit within target dimensions

  int scaled_width = static_cast<int>(bitmap.width() * scale_factor);
  int scaled_height = static_cast<int>(bitmap.height() * scale_factor);

  // Center the scaled image within the target dimensions
  int offset_x = (target_width - scaled_width) / 2;
  int offset_y = (target_height - scaled_height) / 2;

  // Source rect (entire source image)
  SkRect src_rect = SkRect::MakeWH(bitmap.width(), bitmap.height());

  // Destination rect (centered within target dimensions)
  SkRect dst_rect =
      SkRect::MakeXYWH(offset_x, offset_y, scaled_width, scaled_height);

  canvas.drawImageRect(bitmap.asImage(), src_rect, dst_rect, sampling_options,
                       nullptr, SkCanvas::kStrict_SrcRectConstraint);

  return scaled_bitmap;
}

// Convert SkBitmap to FastVLM float array format (free function used by lambda)
std::vector<float> ConvertBitmapToFastVLM(const SkBitmap& bitmap,
                                          int image_width,
                                          int image_height,
                                          int image_channels) {
  const size_t total_size = 1 * image_channels * image_height *
                            image_width;  // [1, channels, height, width]
  std::vector<float> pixel_data(total_size);

  // CHW format: all R values, then all G values, then all B values
  size_t pixels_per_channel = image_height * image_width;
  size_t r_offset = 0;
  size_t g_offset = pixels_per_channel;
  size_t b_offset = 2 * pixels_per_channel;

  for (int y = 0; y < image_height; ++y) {
    for (int x = 0; x < image_width; ++x) {
      SkColor pixel = bitmap.getColor(x, y);
      size_t idx = y * image_width + x;

      // Convert from [0,255] to [0,1] and store in CHW format
      pixel_data[r_offset + idx] = SkColorGetR(pixel) / 255.0f;
      pixel_data[g_offset + idx] = SkColorGetG(pixel) / 255.0f;
      pixel_data[b_offset + idx] = SkColorGetB(pixel) / 255.0f;
    }
  }

  return pixel_data;
}

}  // namespace

namespace local_ai {

// InferenceRequest implementation
InferenceRequest::InferenceRequest() = default;
InferenceRequest::~InferenceRequest() = default;
InferenceRequest::InferenceRequest(const InferenceRequest&) = default;
InferenceRequest& InferenceRequest::operator=(const InferenceRequest&) =
    default;
InferenceRequest::InferenceRequest(InferenceRequest&&) = default;
InferenceRequest& InferenceRequest::operator=(InferenceRequest&&) = default;

// InferenceResult implementation
InferenceResult::InferenceResult() = default;
InferenceResult::~InferenceResult() = default;
InferenceResult::InferenceResult(const InferenceResult&) = default;
InferenceResult& InferenceResult::operator=(const InferenceResult&) = default;
InferenceResult::InferenceResult(InferenceResult&&) = default;
InferenceResult& InferenceResult::operator=(InferenceResult&&) = default;

// FastVLMExecutor implementation
FastVLMExecutor::FastVLMExecutor() {
  LOG(INFO) << "FastVLMExecutor constructor called";
  // Use lazy initialization to avoid heap conflicts with Chromium's graphics
  // system ONNX Runtime will be initialized when first needed
  LOG(INFO)
      << "FastVLMExecutor constructor completed, will initialize on first use";
}

FastVLMExecutor::~FastVLMExecutor() {
  // Invalidate weak pointers first while sequence checker is still valid
  // In tests, this might fail due to task environment destruction, but it's
  // safe to ignore
  weak_ptr_factory_.InvalidateWeakPtrs();

  // Clean up ONNX Runtime wrapper components
  // The scoped wrapper types will automatically handle cleanup
  ort_environment_.reset();

  LOG(INFO) << "[LocalAI] FastVLMExecutor destroyed";
}

void FastVLMExecutor::InitializeOnnxRuntime() {
  LOG(INFO) << "Initializing ONNX Runtime using new wrapper infrastructure";

  // Initialize based on platform capabilities
  available_providers_.clear();

  // Use default hardcoded library path for now
  // TODO: Make this configurable via LoadModel parameters
  base::FilePath library_path(
      FILE_PATH_LITERAL("/Users/darkdh/Projects/onnxruntime/build/MacOS/"
                        "RelWithDebInfo/libonnxruntime.dylib"));

  // Initialize platform functions with the library path
  auto* platform_functions = ort::PlatformFunctions::GetInstance(library_path);
  if (!platform_functions || !platform_functions->IsInitialized()) {
    LOG(ERROR) << "Failed to initialize ONNX Runtime platform functions";
    onnx_runtime_initialized_ = false;
    return;
  }

  // Create thread-safe environment wrapper
  auto environment_result = ort::Environment::Create(ORT_LOGGING_LEVEL_WARNING);
  if (!environment_result.has_value()) {
    LOG(ERROR) << "Failed to create ONNX Runtime environment: "
               << environment_result.error();
    onnx_runtime_initialized_ = false;
    return;
  }

  ort_environment_ = std::move(environment_result.value());
  available_providers_.push_back("CPUExecutionProvider");

  LOG(INFO) << "ONNX Runtime wrapper infrastructure initialized successfully";
  onnx_runtime_initialized_ = true;

  LOG(INFO) << "Initialized with " << available_providers_.size()
            << " providers";
}

void FastVLMExecutor::LoadModel(const base::FilePath& model_dir,
                                base::OnceCallback<void(bool)> callback) {
  // Do not log the file path value immediately - it might be causing the crash
  LOG(INFO) << "LoadModel called, this=" << this;
  LOG(INFO) << "State check, current state=" << static_cast<int>(state_);
  LOG(INFO) << "Loading FastVLM model from path";
  model_dir_ = model_dir;
  LOG(INFO) << "Model dir assigned successfully";

  // Check if model directory exists first to avoid crashes
  if (!base::DirectoryExists(model_dir)) {
    LOG(ERROR) << "Model directory does not exist: " << model_dir.value();
    state_ = LoadState::kError;
    last_error_ = "Model directory not found";
    std::move(callback).Run(false);
    return;
  }

  // Initialize ONNX Runtime lazily when first needed
  if (!onnx_runtime_initialized_) {
    LOG(INFO) << "Initializing ONNX Runtime lazily for model loading";
    InitializeOnnxRuntime();
    if (!onnx_runtime_initialized_) {
      LOG(ERROR) << "Failed to initialize ONNX Runtime";
      state_ = LoadState::kError;
      last_error_ = "ONNX Runtime not initialized";
      std::move(callback).Run(false);
      return;
    }
  }

  state_ = LoadState::kLoadingModels;
  LoadOnnxModels(std::move(callback));
}

void FastVLMExecutor::RunInference(const InferenceRequest& request,
                                   InferenceCallback callback) {
  LOG(INFO) << "Running inference with prompt: " << request.text_prompt;
  LOG(INFO) << "Current state: " << static_cast<int>(state_);
  LOG(INFO) << "ONNX Runtime initialized: " << onnx_runtime_initialized_;

  if (state_ != LoadState::kReady) {
    LOG(ERROR) << "State is not ready: " << static_cast<int>(state_)
               << ", error: " << last_error_;
    InferenceResult result;
    result.success = false;
    result.error_message =
        (state_ == LoadState::kError) ? last_error_ : "Model not ready";
    std::move(callback).Run(std::move(result));
    return;
  }

  // Step 1: Process image data (if provided)
  if (!request.image_data.empty()) {
    RunVisionEncoding(
        request.image_data,
        base::BindOnce(&FastVLMExecutor::OnVisionEncodingComplete,
                       weak_ptr_factory_.GetWeakPtr(), request.text_prompt,
                       request.max_tokens, std::move(callback)));
  } else {
    // Text-only inference
    std::vector<float> empty_vision_features;
    RunTokenEmbedding(request.text_prompt,
                      base::BindOnce(&FastVLMExecutor::OnTokenEmbeddingComplete,
                                     weak_ptr_factory_.GetWeakPtr(),
                                     empty_vision_features, request.text_prompt,
                                     request.max_tokens, std::move(callback)));
  }
}

bool FastVLMExecutor::IsPreferredVariant(const std::string& new_filename,
                                         const std::string& existing_filename) {
  // Preference order: q4 > fp16 > int8 > others
  // This balances performance and quality for FastVLM

  auto get_variant_priority = [](const std::string& filename) -> int {
    if (filename.find("_q4.onnx") != std::string::npos) {
      return 4;  // Highest
    }
    if (filename.find("_fp16.onnx") != std::string::npos) {
      return 3;
    }
    if (filename.find("_int8.onnx") != std::string::npos) {
      return 2;
    }
    if (filename.find(".onnx") != std::string::npos) {
      return 1;  // Base model
    }
    return 0;  // Unknown variant
  };

  int new_priority = get_variant_priority(new_filename);
  int existing_priority = get_variant_priority(existing_filename);

  return new_priority > existing_priority;
}

void FastVLMExecutor::ExtractModelMetadata(const std::string& model_path,
                                           const std::string& model_type) {
  LOG(INFO) << "[LocalAI] Extracting metadata for " << model_type << " model";

  if (!ort_environment_ || !ort_environment_->IsValid()) {
    LOG(ERROR)
        << "[LocalAI] ORT environment not available for metadata extraction";
    return;
  }

  auto* platform_functions = ort::PlatformFunctions::GetInstance();
  if (!platform_functions || !platform_functions->IsInitialized()) {
    LOG(ERROR) << "[LocalAI] Platform functions not initialized";
    return;
  }

  const OrtApi* ort_api = platform_functions->ort_api();

  // Create session options using scoped wrapper
  ort::ScopedOrtSessionOptions session_options;
  if (ORT_CALL_FAILED(ort_api->CreateSessionOptions(
          ort::ScopedOrtSessionOptions::Receiver(session_options).get()))) {
    LOG(ERROR)
        << "[LocalAI] Failed to create session options for metadata extraction";
    return;
  }

  // Create session using scoped wrapper
  ort::ScopedOrtSession session;
  if (ORT_CALL_FAILED(ort_api->CreateSession(
          ort_environment_->get(), model_path.c_str(), session_options.get(),
          ort::ScopedOrtSession::Receiver(session).get()))) {
    LOG(ERROR) << "[LocalAI] Failed to create session for " << model_type
               << " at path: " << model_path;
    return;
  }

  LOG(INFO) << "[LocalAI] Successfully created session for metadata extraction";

  // Get allocator using scoped wrapper
  OrtAllocator* allocator = nullptr;
  if (ORT_CALL_FAILED(ort_api->GetAllocatorWithDefaultOptions(&allocator))) {
    LOG(ERROR) << "[LocalAI] Failed to get allocator";
    return;
  }

  // Extract input metadata
  size_t num_input_nodes = 0;
  if (ORT_CALL_FAILED(
          ort_api->SessionGetInputCount(session.get(), &num_input_nodes))) {
    LOG(ERROR) << "[LocalAI] Failed to get input count";
    return;
  }

  LOG(INFO) << "[LocalAI] Model " << model_type << " has " << num_input_nodes
            << " inputs";

  // Process each input
  for (size_t i = 0; i < num_input_nodes; ++i) {
    char* input_name = nullptr;
    if (ORT_CALL_FAILED(ort_api->SessionGetInputName(session.get(), i,
                                                     allocator, &input_name))) {
      continue;  // Skip on error
    }

    ort::ScopedOrtTypeInfo type_info;
    if (ORT_CALL_FAILED(ort_api->SessionGetInputTypeInfo(
            session.get(), i,
            ort::ScopedOrtTypeInfo::Receiver(type_info).get()))) {
      continue;
    }

    // Get tensor shape info (lifetime tied to type_info)
    const OrtTensorTypeAndShapeInfo* shape_info = nullptr;
    if (ORT_CALL_FAILED(
            ort_api->CastTypeInfoToTensorInfo(type_info.get(), &shape_info))) {
      continue;
    }

    size_t num_dims = 0;
    if (ORT_CALL_FAILED(ort_api->GetDimensionsCount(shape_info, &num_dims))) {
      continue;
    }

    std::vector<int64_t> dims(num_dims);
    if (ORT_CALL_FAILED(
            ort_api->GetDimensions(shape_info, dims.data(), num_dims))) {
      continue;
    }

    // Store the input shape information
    model_input_shapes_[model_type][input_name] = dims;

    // Log the shape information
    std::string dims_str;
    for (size_t j = 0; j < dims.size(); ++j) {
      if (j > 0) {
        dims_str += ", ";
      }
      dims_str += std::to_string(dims[j]);
    }
    LOG(INFO) << "[LocalAI] Input '" << input_name << "' shape: [" << dims_str
              << "]";

    // Extract vision encoder specific metadata
    if (model_type == "vision_encoder" &&
        std::string(input_name) == "pixel_values" && dims.size() == 4) {
      // Expected format: [batch, channels, height, width]

      // Extract channels (usually fixed at 3 for RGB)
      if (dims[1] > 0) {
        image_channels_ = static_cast<int>(dims[1]);
      }

      // Extract image dimensions
      if (dims[2] > 0 && dims[3] > 0) {
        // Fixed dimensions in ONNX model
        image_height_ = static_cast<int>(dims[2]);
        image_width_ = static_cast<int>(dims[3]);
        LOG(INFO) << "[LocalAI] Fixed image dimensions: " << image_channels_
                  << " channels, " << image_height_ << "x" << image_width_;
      } else {
        // Dynamic dimensions - try to read from preprocessor_config.json
        LOG(INFO) << "[LocalAI] Dynamic image dimensions detected, reading "
                     "preprocessor config";
        if (!TryLoadPreprocessorConfig()) {
          // Fallback to sensible defaults for FastVLM
          image_height_ = 336;  // Good balance for FastVLM performance
          image_width_ = 336;
          LOG(INFO)
              << "[LocalAI] No preprocessor config found, using default size: "
              << image_height_ << "x" << image_width_;
        }
      }
    }

    // Free the input name allocated by ONNX Runtime
    allocator->Free(allocator, input_name);
  }

  // Extract output metadata
  size_t num_output_nodes = 0;
  if (ORT_CALL_FAILED(
          ort_api->SessionGetOutputCount(session.get(), &num_output_nodes))) {
    LOG(ERROR) << "[LocalAI] Failed to get output count";
    return;
  }

  LOG(INFO) << "[LocalAI] Model " << model_type << " has " << num_output_nodes
            << " outputs";

  // Process each output
  for (size_t i = 0; i < num_output_nodes; ++i) {
    char* output_name = nullptr;
    if (ORT_CALL_FAILED(ort_api->SessionGetOutputName(
            session.get(), i, allocator, &output_name))) {
      continue;  // Skip on error
    }

    ort::ScopedOrtTypeInfo type_info;
    if (ORT_CALL_FAILED(ort_api->SessionGetOutputTypeInfo(
            session.get(), i,
            ort::ScopedOrtTypeInfo::Receiver(type_info).get()))) {
      continue;
    }

    // Get tensor shape info (lifetime tied to type_info)
    const OrtTensorTypeAndShapeInfo* shape_info = nullptr;
    if (ORT_CALL_FAILED(
            ort_api->CastTypeInfoToTensorInfo(type_info.get(), &shape_info))) {
      continue;
    }

    size_t num_dims = 0;
    if (ORT_CALL_FAILED(ort_api->GetDimensionsCount(shape_info, &num_dims))) {
      continue;
    }

    std::vector<int64_t> dims(num_dims);
    if (ORT_CALL_FAILED(
            ort_api->GetDimensions(shape_info, dims.data(), num_dims))) {
      continue;
    }

    // Log the shape information (no need to store it - we get it dynamically at
    // runtime)
    std::string dims_str;
    for (size_t j = 0; j < dims.size(); ++j) {
      if (j > 0) {
        dims_str += ", ";
      }
      dims_str += std::to_string(dims[j]);
    }
    LOG(INFO) << "[LocalAI] Output '" << output_name << "' shape: [" << dims_str
              << "]";

    // Extract embed_tokens specific metadata
    if (model_type == "embed_tokens" &&
        std::string(output_name) == "last_hidden_state" && dims.size() >= 3) {
      hidden_size_ =
          static_cast<int>(dims[2]);  // [batch, seq_len, hidden_size]
      LOG(INFO) << "[LocalAI] Detected hidden_size from embed_tokens model: "
                << hidden_size_;
    }

    // Free the output name allocated by ONNX Runtime
    allocator->Free(allocator, output_name);
  }

  LOG(INFO) << "[LocalAI] Metadata extraction complete for " << model_type;
  // Session and other resources are automatically cleaned up by scoped wrappers
}

void FastVLMExecutor::LoadOnnxModels(base::OnceCallback<void(bool)> callback) {
  LOG(INFO)
      << "Loading ONNX models for FastVLM using actual ONNX model parsing";

  // ONNX Runtime should already be initialized at this point

  // Use hardcoded model filenames based on README.md recommendations
  base::FilePath onnx_dir = model_dir_.AppendASCII("onnx");
  if (!base::DirectoryExists(onnx_dir)) {
    LOG(ERROR) << "ONNX models directory does not exist: " << onnx_dir;
    state_ = LoadState::kError;
    last_error_ = "ONNX models directory not found";
    std::move(callback).Run(false);
    return;
  }

  // Scan for available ONNX models instead of hardcoding filenames
  std::map<std::string, std::string> selected_models;

  // Required model types to find
  std::set<std::string> required_types = {"vision_encoder", "embed_tokens",
                                          "decoder_model_merged"};

  base::FileEnumerator file_enum(onnx_dir, false, base::FileEnumerator::FILES,
                                 "*.onnx");
  for (base::FilePath file_path = file_enum.Next(); !file_path.empty();
       file_path = file_enum.Next()) {
    std::string filename = file_path.BaseName().AsUTF8Unsafe();

    // Direct prefix matching for model types
    std::string model_type;
    if (filename.starts_with("vision_encoder")) {
      model_type = "vision_encoder";
    } else if (filename.starts_with("embed_tokens")) {
      model_type = "embed_tokens";
    } else if (filename.starts_with("decoder_model_merged")) {
      model_type = "decoder_model_merged";
    } else {
      continue;  // Skip unknown model types
    }

    // If we don't have this type yet, or this is a preferred variant, use it
    if (selected_models.find(model_type) == selected_models.end() ||
        IsPreferredVariant(filename, selected_models[model_type])) {
      selected_models[model_type] = filename;
      LOG(INFO) << "Selected " << filename << " for " << model_type;
    }
  }

  // Check that we found all required models
  for (const std::string& required_type : required_types) {
    if (selected_models.find(required_type) == selected_models.end()) {
      LOG(ERROR) << "Required model type not found: " << required_type;
      state_ = LoadState::kError;
      last_error_ = "Missing required model type: " + required_type;
      std::move(callback).Run(false);
      return;
    }
  }

  LOG(INFO) << "Found all required models by scanning directory";

  // Store selected model information
  for (const auto& [model_type, filename] : selected_models) {
    base::FilePath model_file = onnx_dir.AppendASCII(filename);

    // Store file information
    std::optional<int64_t> file_size = base::GetFileSize(model_file);
    if (file_size.has_value()) {
      model_file_sizes_[filename] = file_size.value();
      model_paths_[filename] = model_file.AsUTF8Unsafe();
      model_paths_[model_type] =
          model_file.AsUTF8Unsafe();  // Also store by type
      LOG(INFO) << "Model " << filename << " (" << model_type
                << ") size: " << file_size.value() << " bytes";
    }
  }

  // Extract metadata from each model using new ORT wrapper infrastructure
  LOG(INFO)
      << "Extracting metadata from ONNX models using wrapper infrastructure";

  for (const auto& [model_type, filename] : selected_models) {
    base::FilePath model_file = onnx_dir.AppendASCII(filename);
    std::string model_path = model_file.AsUTF8Unsafe();

    LOG(INFO) << "Extracting metadata for " << model_type << " from "
              << filename;
    ExtractModelMetadata(model_path, model_type);
  }

  // Create ONNX Runtime sessions for each model
  LOG(INFO) << "Creating ONNX Runtime sessions for models";

  for (const auto& [model_type, filename] : selected_models) {
    base::FilePath model_file = onnx_dir.AppendASCII(filename);
    std::string model_path = model_file.AsUTF8Unsafe();

    LOG(INFO) << "Creating session for " << model_type << " from " << filename;
    if (!CreateOnnxSession(model_path, model_type)) {
      LOG(ERROR) << "Failed to create ONNX session for " << model_type;
      state_ = LoadState::kError;
      last_error_ = "Session creation failed for " + model_type;
      std::move(callback).Run(false);
      return;
    }
  }

  LOG(INFO) << "All ONNX Runtime sessions created successfully";

  // Load model configuration
  if (!LoadModelConfig()) {
    LOG(ERROR) << "Failed to load model configuration";
    state_ = LoadState::kError;
    last_error_ = "Model config loading failed";
    std::move(callback).Run(false);
    return;
  }

  // Load tokenizer files
  if (!LoadTokenizerFiles()) {
    LOG(ERROR) << "Failed to load tokenizer files";
    state_ = LoadState::kError;
    last_error_ = "Tokenizer loading failed";
    std::move(callback).Run(false);
    return;
  }

  LOG(INFO) << "All FastVLM models and sessions loaded successfully with ONNX "
               "Runtime";
  state_ = LoadState::kReady;
  std::move(callback).Run(true);
}

// Image and text processing functions
void FastVLMExecutor::ProcessImageDataAsync(
    const std::vector<uint8_t>& image_data,
    base::OnceCallback<void(std::vector<float>)> callback) {
  LOG(INFO) << "ProcessImageDataAsync: Starting image processing";
  LOG(INFO) << "ProcessImageDataAsync: Image data size = " << image_data.size();

  if (image_data.empty()) {
    LOG(ERROR) << "ProcessImageDataAsync: Empty image data provided";
    std::move(callback).Run({});
    return;
  }

  LOG(INFO) << "ProcessImageDataAsync: Calling DecodeImage";
  // Use Chromium's data decoder service to decode the image
  data_decoder::DecodeImage(
      GetDataDecoder(), image_data, data_decoder::mojom::ImageCodec::kDefault,
      true, data_decoder::kDefaultMaxSizeInBytes, gfx::Size(),
      base::BindOnce(&FastVLMExecutor::OnImageDecoded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  LOG(INFO) << "ProcessImageDataAsync: DecodeImage call completed";
}

void FastVLMExecutor::OnImageDecoded(
    base::OnceCallback<void(std::vector<float>)> callback,
    const SkBitmap& decoded_bitmap) {
  LOG(INFO) << "OnImageDecoded: Called";

  if (decoded_bitmap.drawsNothing()) {
    LOG(ERROR) << "OnImageDecoded: Failed to decode image";
    std::move(callback).Run({});
    return;
  }

  LOG(INFO) << "OnImageDecoded: Image decoded successfully: "
            << decoded_bitmap.width() << "x" << decoded_bitmap.height();

  // Create a safe copy of the bitmap to avoid macOS UI memory issues
  SkBitmap bitmap_copy;
  if (!bitmap_copy.tryAllocPixels(decoded_bitmap.info())) {
    LOG(ERROR) << "Failed to allocate bitmap copy";
    std::move(callback).Run({});
    return;
  }

  if (!decoded_bitmap.readPixels(bitmap_copy.info(), bitmap_copy.getPixels(),
                                 bitmap_copy.rowBytes(), 0, 0)) {
    LOG(ERROR) << "Failed to copy bitmap pixels";
    std::move(callback).Run({});
    return;
  }

  // Process the bitmap on a background thread
  LOG(INFO) << "OnImageDecoded: Starting background image processing with "
               "image_dimensions="
            << image_height_ << "x" << image_width_
            << ", image_channels_=" << image_channels_;
  auto process_bitmap = base::BindOnce(
      [](int image_width, int image_height, int image_channels,
         const SkBitmap& bitmap) -> std::vector<float> {
        LOG(INFO) << "Background thread: Scaling bitmap to " << image_width
                  << "x" << image_height
                  << ", image_channels=" << image_channels;
        // Scale to model-specified dimensions while preserving aspect ratio
        SkBitmap scaled_bitmap =
            ScaleFastVLMBitmap(bitmap, image_width, image_height);
        LOG(INFO) << "Background thread: Bitmap scaled to "
                  << scaled_bitmap.width() << "x" << scaled_bitmap.height();

        LOG(INFO) << "Background thread: Converting bitmap to FastVLM format";
        // Convert to FastVLM format: [1, channels, height, width] CHW with
        // [0,1] pixel values
        auto result = ConvertBitmapToFastVLM(scaled_bitmap, image_width,
                                             image_height, image_channels);
        LOG(INFO) << "Background thread: Conversion complete, result size: "
                  << result.size();
        return result;
      },
      image_width_, image_height_, image_channels_, std::move(bitmap_copy));

  LOG(INFO) << "OnImageDecoded: Posting background task";
  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(process_bitmap),
                                               std::move(callback));
  LOG(INFO) << "OnImageDecoded: Background task posted";
}

std::vector<int32_t> FastVLMExecutor::TokenizeText(const std::string& text) {
  LOG(INFO) << "Tokenizing: " << text;

  // Use the chat template to format the text properly
  std::string formatted_text = ApplyChatTemplate(text, false);

  // Use the improved but safer tokenization
  return SimpleTokenizeText(formatted_text);
}

// Simple but safer tokenization approach
std::vector<int32_t> FastVLMExecutor::SimpleTokenizeText(
    const std::string& formatted_text) {
  LOG(INFO) << "BPE tokenizing: " << formatted_text;

  std::vector<int32_t> tokens;

  // Check if vocabulary is loaded
  if (vocab_token_to_id_.empty()) {
    LOG(ERROR) << "Vocabulary not loaded - cannot tokenize";
    return tokens;
  }

  // First, split text preserving special tokens as separate entities
  std::string text = formatted_text;
  std::vector<std::string> parts;

  // Replace special tokens with placeholder surrounded by spaces, then split,
  // then restore
  std::vector<std::pair<std::string, int32_t>> special_replacements;
  for (const auto& [token, id] : special_tokens_) {
    size_t pos = 0;
    while ((pos = text.find(token, pos)) != std::string::npos) {
      // Create a unique placeholder for this special token, surrounded by
      // spaces
      std::string placeholder_key =
          "<<SPECIAL_" + std::to_string(special_replacements.size()) + ">>";
      std::string placeholder = " " + placeholder_key + " ";
      special_replacements.push_back({placeholder_key, id});
      text.replace(pos, token.length(), placeholder);
      pos += placeholder.length();
    }
  }

  // Pre-tokenization: split on whitespace
  std::vector<std::string> words;
  std::istringstream iss(text);
  std::string word;

  while (iss >> word) {
    words.push_back(word);
  }

  // Process each word
  for (size_t word_idx = 0; word_idx < words.size(); ++word_idx) {
    const std::string& current_word = words[word_idx];

    // Check for special token placeholders first
    bool found_special = false;
    for (const auto& [placeholder, token_id] : special_replacements) {
      if (current_word == placeholder) {
        tokens.push_back(token_id);
        found_special = true;
        break;
      }
    }
    if (found_special) {
      continue;
    }

    // For non-first words, try with space prefix (Ġ)
    std::string word_to_process =
        (word_idx == 0) ? current_word : "Ġ" + current_word;

    // Try to find the word directly in vocabulary first
    auto vocab_it = vocab_token_to_id_.find(word_to_process);
    if (vocab_it != vocab_token_to_id_.end()) {
      tokens.push_back(vocab_it->second);
      continue;
    }

    // If not found directly, apply safer BPE tokenization
    std::vector<std::string> bpe_tokens = BPETokenizeWord(word_to_process);

    // Convert BPE tokens to IDs
    for (const auto& bpe_token : bpe_tokens) {
      auto token_it = vocab_token_to_id_.find(bpe_token);
      if (token_it != vocab_token_to_id_.end()) {
        tokens.push_back(token_it->second);
      } else {
        LOG(WARNING) << "Unknown token: " << bpe_token;
      }
    }
  }

  LOG(INFO) << "Tokenization complete: " << tokens.size() << " tokens (with "
            << special_replacements.size() << " special tokens preserved)";
  return tokens;
}

// Inference pipeline callbacks
void FastVLMExecutor::OnVisionEncodingComplete(
    const std::string& text_prompt,
    int max_tokens,
    InferenceCallback callback,
    std::vector<float> vision_features) {
  LOG(INFO) << "Vision encoding complete, running token embedding for text: "
            << text_prompt;

  RunTokenEmbedding(
      text_prompt,
      base::BindOnce(&FastVLMExecutor::OnTokenEmbeddingComplete,
                     weak_ptr_factory_.GetWeakPtr(), vision_features,
                     text_prompt, max_tokens, std::move(callback)));
}

void FastVLMExecutor::OnTokenEmbeddingComplete(
    const std::vector<float>& vision_features,
    const std::string& text_prompt,
    int max_tokens,
    InferenceCallback callback,
    std::vector<float> text_embeddings) {
  LOG(INFO) << "Token embedding complete, running decoding";

  if (text_embeddings.empty()) {
    LOG(ERROR) << "Token embedding failed";
    InferenceResult result;
    result.success = false;
    result.error_message = "Token embedding failed";
    std::move(callback).Run(std::move(result));
    return;
  }
  // Get the token IDs that were used for tokenization
  std::vector<int32_t> token_ids =
      SimpleTokenizeText(ApplyChatTemplate(text_prompt, false));

  RunDecoding(vision_features, text_embeddings, token_ids, max_tokens,
              std::move(callback));
}

void FastVLMExecutor::RunVisionEncoding(
    const std::vector<uint8_t>& image_data,
    base::OnceCallback<void(std::vector<float>)> vision_callback) {
  LOG(INFO) << "Running vision encoding using direct ONNX Runtime";

  if (image_data.empty()) {
    LOG(ERROR) << "Empty image data provided";
    std::move(vision_callback).Run(std::vector<float>());
    return;
  }

  if (state_ != LoadState::kReady) {
    LOG(ERROR) << "FastVLM executor not ready";
    std::move(vision_callback).Run(std::vector<float>());
    return;
  }

  // Process image data to FastVLM input format [1, channels, height, width]
  // asynchronously
  ProcessImageDataAsync(image_data,
                        base::BindOnce(&FastVLMExecutor::OnImageProcessed,
                                       weak_ptr_factory_.GetWeakPtr(),
                                       std::move(vision_callback)));
}

void FastVLMExecutor::OnImageProcessed(
    base::OnceCallback<void(std::vector<float>)> vision_callback,
    std::vector<float> processed_image) {
  if (processed_image.empty()) {
    LOG(ERROR) << "Failed to process image data";
    std::move(vision_callback).Run(std::vector<float>());
    return;
  }

  LOG(INFO) << "Image processed successfully, size: " << processed_image.size();

  // Execute vision encoder directly with ONNX Runtime
  std::vector<float> image_features;
  ExecuteVisionEncoder(processed_image, image_features);

  LOG(INFO) << "Vision encoding complete, generated " << image_features.size()
            << " features";
  std::move(vision_callback).Run(std::move(image_features));
}

void FastVLMExecutor::RunTokenEmbedding(
    const std::string& text_prompt,
    base::OnceCallback<void(std::vector<float>)> embedding_callback) {
  LOG(INFO) << "Running token embedding using direct ONNX Runtime";

  if (text_prompt.empty()) {
    LOG(ERROR) << "Empty text prompt provided";
    std::move(embedding_callback).Run(std::vector<float>());
    return;
  }

  if (state_ != LoadState::kReady) {
    LOG(ERROR) << "FastVLM executor not ready";
    std::move(embedding_callback).Run(std::vector<float>());
    return;
  }

  // Tokenize the text using loaded FastVLM vocabulary
  std::vector<int32_t> token_ids = TokenizeText(text_prompt);
  if (token_ids.empty()) {
    LOG(ERROR) << "Failed to tokenize text";
    std::move(embedding_callback).Run(std::vector<float>());
    return;
  }

  // Execute token embedder directly with ONNX Runtime
  std::vector<float> embeddings;
  ExecuteTokenEmbedder(token_ids, embeddings);

  LOG(INFO) << "Token embedding complete, generated " << embeddings.size()
            << " embeddings";
  std::move(embedding_callback).Run(std::move(embeddings));
}

void FastVLMExecutor::ExecuteVisionEncoder(
    const std::vector<float>& pixel_values,
    std::vector<float>& image_features) {
  LOG(INFO) << "Executing vision encoder with direct ONNX Runtime";

  // Execute the vision encoder using ONNX Runtime
  std::map<std::string, std::vector<float>> inputs;
  inputs["pixel_values"] = pixel_values;

  std::map<std::string, std::vector<float>> outputs;
  RunOnnxInference("vision_encoder", inputs, outputs);

  if (outputs.find("image_features") != outputs.end()) {
    image_features = outputs["image_features"];

    // The actual shape is already determined by ONNX Runtime when we extracted
    // the output
    LOG(INFO) << "Vision encoder output: " << image_features.size()
              << " elements";

    // Validate the output format based on expected structure
    if (image_features.size() % hidden_size_ == 0) {
      int64_t sequence_length = image_features.size() / hidden_size_;
      LOG(INFO) << "✓ Vision features: " << sequence_length << " tokens × "
                << hidden_size_ << " dimensions";
    } else {
      LOG(WARNING) << "Vision features size " << image_features.size()
                   << " not divisible by hidden_size " << hidden_size_;
    }
  } else {
    LOG(ERROR) << "Vision encoder failed - no image_features output found";
    image_features.clear();
  }
}

void FastVLMExecutor::ExecuteTokenEmbedder(
    const std::vector<int32_t>& input_ids,
    std::vector<float>& embeddings) {
  LOG(INFO) << "Executing token embedder with direct ONNX Runtime";

  if (input_ids.empty()) {
    LOG(ERROR) << "Empty input_ids provided";
    embeddings.clear();
    return;
  }

  // Convert int32_t to int64_t for ONNX Runtime input (tokens should be
  // integers)
  std::vector<int64_t> input_ids_int64;
  input_ids_int64.reserve(input_ids.size());
  for (int32_t id : input_ids) {
    input_ids_int64.push_back(static_cast<int64_t>(id));
  }

  // Execute the embed_tokens model using templated ONNX Runtime with integer
  // inputs
  std::map<std::string, std::vector<float>> outputs;
  RunOnnxInferenceTemplate("embed_tokens", "input_ids", input_ids_int64,
                           outputs);

  // Extract the embeddings output
  if (outputs.find("inputs_embeds") != outputs.end()) {
    embeddings = outputs["inputs_embeds"];
    LOG(INFO) << "Token embedder generated " << embeddings.size()
              << " embeddings";
  } else {
    LOG(ERROR) << "Token embedder failed - no inputs_embeds output found";
    embeddings.clear();
  }
}

void FastVLMExecutor::RunDecoding(const std::vector<float>& vision_features,
                                  const std::vector<float>& text_embeddings,
                                  const std::vector<int32_t>& token_ids,
                                  int max_tokens,
                                  InferenceCallback callback) {
  LOG(INFO) << "Running decoding with direct ONNX Runtime, max_tokens="
            << max_tokens;

  if (state_ != LoadState::kReady) {
    LOG(ERROR) << "FastVLM executor not ready";
    InferenceResult result;
    result.success = false;
    result.error_message = "FastVLM executor not ready";
    std::move(callback).Run(std::move(result));
    return;
  }

  if (vision_features.empty() || text_embeddings.empty()) {
    LOG(ERROR) << "Invalid input features for decoding";
    InferenceResult result;
    result.success = false;
    result.error_message = "Invalid input features";
    std::move(callback).Run(std::move(result));
    return;
  }

  LOG(INFO) << "Executing FastVLM decoder with vision features ("
            << vision_features.size() << ") and text embeddings ("
            << text_embeddings.size() << ") using direct ONNX Runtime";

  // Call the real decoder execution using direct ONNX Runtime
  ExecuteRealDecoder(vision_features, text_embeddings, token_ids, max_tokens,
                     std::move(callback));
}

bool FastVLMExecutor::LoadTokenizerFiles() {
  LOG(INFO) << "Loading FastVLM tokenizer files from model directory";

  // Load vocabulary file
  base::FilePath vocab_path = model_dir_.AppendASCII("vocab.json");
  std::string vocab_json;
  if (!base::ReadFileToString(vocab_path, &vocab_json)) {
    LOG(ERROR) << "Failed to read vocabulary file: " << vocab_path;
    return false;
  }

  std::optional<base::Value> vocab_value = base::JSONReader::Read(vocab_json);
  if (!vocab_value || !vocab_value->is_dict()) {
    LOG(ERROR) << "Invalid JSON format in vocabulary file";
    return false;
  }

  const base::Value::Dict& vocab_dict = vocab_value->GetDict();
  vocab_token_to_id_.clear();
  vocab_id_to_token_.clear();

  for (const auto [token, id_value] : vocab_dict) {
    if (id_value.is_int()) {
      int32_t id = id_value.GetInt();
      vocab_token_to_id_[token] = id;
      vocab_id_to_token_[id] = token;
    }
  }

  LOG(INFO) << "Loaded " << vocab_token_to_id_.size() << " vocabulary entries";

  // Load tokenizer config file
  base::FilePath config_path = model_dir_.AppendASCII("tokenizer_config.json");
  std::string config_json;
  if (!base::ReadFileToString(config_path, &config_json)) {
    LOG(ERROR) << "Failed to read tokenizer config file: " << config_path;
    return false;
  }

  std::optional<base::Value> config_value = base::JSONReader::Read(config_json);
  if (!config_value || !config_value->is_dict()) {
    LOG(ERROR) << "Invalid JSON format in tokenizer config file";
    return false;
  }

  const base::Value::Dict& config_dict = config_value->GetDict();
  special_tokens_.clear();

  // Load special tokens
  const base::Value::Dict* added_tokens =
      config_dict.FindDict("added_tokens_decoder");
  if (added_tokens) {
    for (const auto [id_str, token_info] : *added_tokens) {
      if (token_info.is_dict()) {
        const base::Value::Dict& token_dict = token_info.GetDict();
        const std::string* content = token_dict.FindString("content");
        if (content) {
          int32_t id = std::stoi(id_str);
          special_tokens_[*content] = id;
          vocab_id_to_token_[id] = *content;
          LOG(INFO) << "Special token: " << *content << " -> " << id;
        }
      }
    }
  }

  LOG(INFO) << "Loaded " << special_tokens_.size() << " special tokens";

  // Load BPE merges from tokenizer.json
  if (!LoadBPEMerges()) {
    LOG(WARNING) << "Failed to load BPE merges, tokenization may be incomplete";
  }

  // Load chat template
  const std::string* chat_template = config_dict.FindString("chat_template");
  if (chat_template) {
    chat_template_ = *chat_template;
    LOG(INFO) << "Loaded chat template";
  } else {
    // Use default FastVLM chat template if not found
    chat_template_ =
        "{% for message in messages %}{% if loop.first and messages[0]['role'] "
        "!= 'system' %}{{ '<|im_start|>system\nYou are a helpful "
        "assistant.<|im_end|>\n' }}{% endif %}{{'<|im_start|>' + "
        "message['role'] + '\n' + message['content'] + '<|im_end|>' + '\n'}}{% "
        "endfor %}{% if add_generation_prompt %}{{ '<|im_start|>assistant\n' "
        "}}{% endif %}";
    LOG(INFO) << "Using default chat template";
  }

  return true;
}

bool FastVLMExecutor::LoadBPEMerges() {
  LOG(INFO) << "Loading BPE merges from tokenizer.json";
  auto start_time = base::TimeTicks::Now();

  // Load tokenizer.json file
  base::FilePath tokenizer_path = model_dir_.AppendASCII("tokenizer.json");
  std::string tokenizer_json;
  if (!base::ReadFileToString(tokenizer_path, &tokenizer_json)) {
    LOG(ERROR) << "Failed to read tokenizer.json file: " << tokenizer_path;
    return false;
  }

  std::optional<base::Value> tokenizer_value =
      base::JSONReader::Read(tokenizer_json);
  if (!tokenizer_value || !tokenizer_value->is_dict()) {
    LOG(ERROR) << "Invalid JSON format in tokenizer.json";
    return false;
  }

  const base::Value::Dict& tokenizer_dict = tokenizer_value->GetDict();

  // Find the model section
  const base::Value::Dict* model_dict = tokenizer_dict.FindDict("model");
  if (!model_dict) {
    LOG(ERROR) << "No 'model' section found in tokenizer.json";
    return false;
  }

  // Find the merges array
  const base::Value::List* merges_list = model_dict->FindList("merges");
  if (!merges_list) {
    LOG(ERROR) << "No 'merges' array found in tokenizer.json model section";
    return false;
  }

  // Load merges and build bpe_ranks map
  bpe_merges_.clear();
  bpe_merges_.reserve(merges_list->size());
  bpe_ranks_.clear();

  int rank = 0;
  for (const auto& merge_item : *merges_list) {
    if (merge_item.is_list() && merge_item.GetList().size() == 2) {
      const auto& merge_pair = merge_item.GetList();
      if (merge_pair[0].is_string() && merge_pair[1].is_string()) {
        std::string left = merge_pair[0].GetString();
        std::string right = merge_pair[1].GetString();
        bpe_merges_.emplace_back(left, right);

        // Create rank key same way as transformers.js: JSON.stringify([left,
        // right])
        std::string rank_key = "[\"" + left + "\",\"" + right + "\"]";
        bpe_ranks_[rank_key] = rank;
        rank++;
      }
    }
  }

  auto end_time = base::TimeTicks::Now();
  LOG(INFO) << "Loaded " << bpe_merges_.size() << " BPE merges with ranks in "
            << (end_time - start_time).InMilliseconds() << "ms";
  return true;
}

bool FastVLMExecutor::LoadModelConfig() {
  LOG(INFO) << "Loading FastVLM model configuration from model directory";

  // Load config.json file
  base::FilePath config_path = model_dir_.AppendASCII("config.json");
  std::string config_json;
  if (!base::ReadFileToString(config_path, &config_json)) {
    LOG(WARNING) << "Failed to read config file, using default hidden_size: "
                 << config_path;
    return true;  // Use defaults
  }

  std::optional<base::Value> config_value = base::JSONReader::Read(config_json);
  if (!config_value || !config_value->is_dict()) {
    LOG(WARNING) << "Invalid JSON format in config file, using defaults";
    return true;  // Use defaults
  }

  const base::Value::Dict& config_dict = config_value->GetDict();

  // Read hidden_size from config
  std::optional<int> hidden_size_opt = config_dict.FindInt("hidden_size");
  if (hidden_size_opt.has_value()) {
    hidden_size_ = hidden_size_opt.value();
    LOG(INFO) << "Loaded hidden_size from config: " << hidden_size_;
  } else {
    LOG(WARNING) << "No hidden_size found in config, using default: "
                 << hidden_size_;
  }

  // Read num_hidden_layers from config
  std::optional<int> num_layers_opt = config_dict.FindInt("num_hidden_layers");
  if (num_layers_opt.has_value()) {
    num_hidden_layers_ = num_layers_opt.value();
    LOG(INFO) << "Loaded num_hidden_layers from config: " << num_hidden_layers_;
  } else {
    LOG(WARNING) << "No num_hidden_layers found in config, using default: "
                 << num_hidden_layers_;
  }

  // Read num_attention_heads from config
  std::optional<int> num_heads_opt = config_dict.FindInt("num_attention_heads");
  if (num_heads_opt.has_value()) {
    num_attention_heads_ = num_heads_opt.value();
    LOG(INFO) << "Loaded num_attention_heads from config: "
              << num_attention_heads_;
  } else {
    LOG(WARNING) << "No num_attention_heads found in config, using default: "
                 << num_attention_heads_;
  }

  // Read num_key_value_heads from config
  std::optional<int> num_kv_heads_opt =
      config_dict.FindInt("num_key_value_heads");
  if (num_kv_heads_opt.has_value()) {
    num_key_value_heads_ = num_kv_heads_opt.value();
    LOG(INFO) << "Loaded num_key_value_heads from config: "
              << num_key_value_heads_;
  } else {
    LOG(WARNING) << "No num_key_value_heads found in config, using default: "
                 << num_key_value_heads_;
  }

  return true;
}

// BPE Node structure for linked list
struct BPENode {
  std::string token;
  double bias;
  double score;
  bool deleted;
  std::shared_ptr<BPENode> prev;
  std::shared_ptr<BPENode> next;

  BPENode(const std::string& t, double b)
      : token(t),
        bias(b),
        score(0.0),
        deleted(false),
        prev(nullptr),
        next(nullptr) {}
};

// Priority queue comparator (lower score = higher priority)
struct BPENodeComparator {
  bool operator()(const std::shared_ptr<BPENode>& a,
                  const std::shared_ptr<BPENode>& b) const {
    return a->score > b->score;  // Note: reversed for min-heap behavior
  }
};

std::vector<std::string> FastVLMExecutor::BPETokenizeWord(
    const std::string& word) {
  if (word.empty()) {
    return {};
  }

  // Use simple character-by-character tokenization as fallback
  // This avoids the complex doubly-linked list that was causing infinite loops
  std::vector<std::string> tokens;

  // Start with character-level tokens
  for (size_t i = 0; i < word.length(); ++i) {
    // Handle basic UTF-8 characters
    size_t char_len = 1;
    if ((word[i] & 0x80) != 0) {
      if ((word[i] & 0xE0) == 0xC0) {
        char_len = 2;
      } else if ((word[i] & 0xF0) == 0xE0) {
        char_len = 3;
      } else if ((word[i] & 0xF8) == 0xF0) {
        char_len = 4;
      }
    }

    if (i + char_len <= word.length()) {
      tokens.push_back(word.substr(i, char_len));
      i += char_len - 1;  // -1 because the loop will increment
    } else {
      tokens.push_back(std::string(1, word[i]));
    }
  }

  // Apply only the most basic BPE merges (safety-limited approach)
  bool changed = true;
  int iterations = 0;
  const int max_iterations = 20;  // Safety limit

  while (changed && iterations < max_iterations) {
    changed = false;
    iterations++;

    for (size_t i = 0; i < tokens.size() - 1; ++i) {
      std::string pair = "[\"" + tokens[i] + "\",\"" + tokens[i + 1] + "\"]";

      if (bpe_ranks_.count(pair)) {
        // Merge these two tokens
        tokens[i] = tokens[i] + tokens[i + 1];
        tokens.erase(tokens.begin() + i + 1);
        changed = true;
        break;  // Only merge one pair per iteration for safety
      }
    }
  }

  return tokens;
}

std::string FastVLMExecutor::ApplyChatTemplate(const std::string& user_message,
                                               bool add_generation_prompt) {
  // Apply proper FastVLM ChatML template from tokenizer_config.json
  std::string result;

  // System message
  result += "<|im_start|>system\n";
  result += "You are a helpful assistant.";
  result += "<|im_end|>\n";

  // User message with image placeholder
  result += "<|im_start|>user\n";
  result += "<image>";  // Image placeholder
  result += user_message;
  result += "<|im_end|>\n";

  // Generation prompt for assistant
  if (add_generation_prompt) {
    result += "<|im_start|>assistant\n";
  }

  LOG(INFO) << "Applied chat template (ChatML format): '" << result << "'";
  return result;
}

std::string FastVLMExecutor::DetokenizeTokens(
    const std::vector<int32_t>& tokens) {
  LOG(INFO) << "Detokenizing " << tokens.size() << " tokens";

  std::string result;

  // Simple approach matching transformers.js batch_decode with
  // skip_special_tokens=true
  for (size_t i = 0; i < tokens.size(); ++i) {
    int32_t token = tokens[i];

    // Skip special tokens (matching transformers.js skip_special_tokens=true)
    if (special_tokens_.find("<|endoftext|>") != special_tokens_.end() &&
        token == special_tokens_.at("<|endoftext|>")) {
      continue;  // Skip, don't break
    }
    if (special_tokens_.find("<|im_end|>") != special_tokens_.end() &&
        token == special_tokens_.at("<|im_end|>")) {
      continue;  // Skip, don't break
    }
    if (special_tokens_.find("<|im_start|>") != special_tokens_.end() &&
        token == special_tokens_.at("<|im_start|>")) {
      continue;  // Skip
    }
    if (special_tokens_.find("<image>") != special_tokens_.end() &&
        token == special_tokens_.at("<image>")) {
      continue;  // Skip
    }

    // Look up token in vocabulary
    auto it = vocab_id_to_token_.find(token);
    if (it != vocab_id_to_token_.end()) {
      std::string token_text = it->second;

      // Direct concatenation like transformers.js does
      result += token_text;

    } else {
      // For unknown tokens, use the token ID
      LOG(WARNING) << "Unknown token ID: " << token;
      result += "<unk_" + std::to_string(token) + ">";
    }
  }

  // Post-process BPE encoding: convert "Ġ" to spaces (matching transformers.js)
  // This handles the byte-level BPE decoding
  if (!result.empty()) {
    // Replace "Ġ" with spaces
    std::string processed_result;
    for (size_t i = 0; i < result.length();) {
      if (i < result.length() - 1 &&
          static_cast<unsigned char>(result[i]) == 0xC4 &&
          static_cast<unsigned char>(result[i + 1]) == 0xA0) {
        // This is the UTF-8 encoding for "Ġ" - replace with space
        processed_result += " ";
        i += 2;
      } else {
        processed_result += result[i];
        i++;
      }
    }
    result = processed_result;

    // Trim leading and trailing whitespace
    size_t start = result.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
      result = "";
    } else {
      size_t end = result.find_last_not_of(" \t\n\r");
      result = result.substr(start, end - start + 1);
    }
  }

  return result;
}

// Direct ONNX Runtime execution methods
void FastVLMExecutor::ExecuteRealDecoder(
    const std::vector<float>& vision_features,
    const std::vector<float>& text_embeddings,
    const std::vector<int32_t>& token_ids,
    int max_tokens,
    InferenceCallback inference_callback) {
  std::vector<int32_t> generated_tokens;

  // Calculate initial sequence dimensions
  // FastVLM approach: Replace <image> token with vision features
  const int64_t vision_seq_len = vision_features.size() / hidden_size_;
  const int64_t text_tokens = text_embeddings.size() / hidden_size_;

  if (vision_features.size() % hidden_size_ != 0) {
    LOG(ERROR) << "Vision features size " << vision_features.size()
               << " not divisible by hidden_size " << hidden_size_;

    InferenceResult result;
    result.success = false;
    result.error_message = "Vision encoder output has incorrect dimensions";
    std::move(inference_callback).Run(std::move(result));
    return;
  }

  LOG(ERROR) << "Building sequence: " << vision_seq_len << " vision tokens, "
             << text_tokens << " text tokens";

  // Find <image> token positions in the token sequence (like Transformers.js)
  std::vector<int32_t> image_token_positions;
  int32_t image_token_id =
      special_tokens_.count("<image>")
          ? special_tokens_.at("<image>")
          : (special_tokens_.count("<|image|>")
                 ? special_tokens_.at("<|image|>")
                 : 151646);  // fallback to hardcoded value if not found

  for (size_t i = 0; i < token_ids.size(); ++i) {
    if (token_ids[i] == image_token_id) {
      image_token_positions.push_back(static_cast<int32_t>(i));
    }
  }

  LOG(INFO) << "Found " << image_token_positions.size()
            << " <image> token(s) in sequence";

  if (image_token_positions.empty()) {
    LOG(ERROR) << "No <image> tokens found in tokenized sequence!";
    InferenceResult result;
    result.success = false;
    result.error_message = "No <image> tokens found in input";
    std::move(inference_callback).Run(std::move(result));
    return;
  }

  // Check that we have matching number of vision features and image tokens
  if (static_cast<int32_t>(image_token_positions.size()) * vision_seq_len !=
      vision_seq_len) {
    // We expect exactly one <image> token that gets replaced with
    // vision_seq_len features
    if (image_token_positions.size() != 1) {
      LOG(ERROR) << "Expected 1 <image> token, found "
                 << image_token_positions.size();
      InferenceResult result;
      result.success = false;
      result.error_message = "Incorrect number of <image> tokens";
      std::move(inference_callback).Run(std::move(result));
      return;
    }
  }

  // Replace <image> token embeddings with vision features (Transformers.js
  // approach)
  std::vector<float> final_embeddings;
  size_t expected_final_size =
      text_embeddings.size() - hidden_size_ + vision_features.size();
  final_embeddings.reserve(expected_final_size);

  for (int64_t token_idx = 0; token_idx < text_tokens; ++token_idx) {
    bool is_image_token = false;

    // Check if this token position contains an <image> token
    for (int32_t img_pos : image_token_positions) {
      if (token_idx == img_pos) {
        is_image_token = true;
        break;
      }
    }

    if (is_image_token) {
      // Replace this token's embedding with all vision features
      final_embeddings.insert(final_embeddings.end(), vision_features.begin(),
                              vision_features.end());
      LOG(INFO) << "Replaced <image> token at position " << token_idx
                << " with " << vision_seq_len << " vision features";
    } else {
      // Keep original text token embedding
      const size_t embed_offset = token_idx * hidden_size_;
      final_embeddings.insert(
          final_embeddings.end(), text_embeddings.begin() + embed_offset,
          text_embeddings.begin() + embed_offset + hidden_size_);
    }
  }

  int64_t current_seq_len = final_embeddings.size() / hidden_size_;

  LOG(INFO) << "Final sequence length: " << current_seq_len << " tokens";

  // Store sequence length for later use in logits extraction (before moving
  // final_embeddings)
  const int64_t initial_sequence_length = current_seq_len;

  // Initialize sequence with vision and text features

  // Initialize KV cache that will persist across generation steps
  const int64_t num_layers = num_hidden_layers_;
  std::map<std::string, std::vector<float>> persistent_kv_cache;

  // Create position_ids for the full sequence [0, 1, 2, ..., current_seq_len-1]
  std::vector<int64_t> position_ids;
  position_ids.reserve(current_seq_len);
  for (int64_t i = 0; i < current_seq_len; ++i) {
    position_ids.push_back(i);
  }

  // Create attention_mask (all 1s for the initial prompt)
  std::vector<int64_t> attention_mask(current_seq_len, 1);

  // Initialize empty KV cache for first step
  for (int64_t layer = 0; layer < num_layers; ++layer) {
    persistent_kv_cache["past_key_values." + std::to_string(layer) + ".key"] =
        std::vector<float>();
    persistent_kv_cache["past_key_values." + std::to_string(layer) + ".value"] =
        std::vector<float>();
  }

  // First inference step with full prompt
  std::map<std::string, std::vector<float>> decoder_inputs;
  std::map<std::string, std::vector<float>> decoder_outputs;
  decoder_inputs["inputs_embeds"] = std::move(final_embeddings);

  // Run first inference with full prompt
  RunOnnxInferenceWithKeyValueCache("decoder_model", decoder_inputs,
                                    position_ids, attention_mask,
                                    persistent_kv_cache, decoder_outputs);

  if (decoder_outputs.empty() ||
      decoder_outputs.find("logits") == decoder_outputs.end()) {
    LOG(ERROR) << "No logits output from first decoder step";
    InferenceResult result;
    result.success = false;
    result.error_message = "Failed to generate initial logits";
    std::move(inference_callback).Run(std::move(result));
    return;
  }

  // Update persistent KV cache with results from first step
  for (int64_t layer = 0; layer < num_layers; ++layer) {
    std::string key_name = "present." + std::to_string(layer) + ".key";
    std::string value_name = "present." + std::to_string(layer) + ".value";

    if (decoder_outputs.count(key_name)) {
      persistent_kv_cache["past_key_values." + std::to_string(layer) + ".key"] =
          std::move(decoder_outputs[key_name]);
    }
    if (decoder_outputs.count(value_name)) {
      persistent_kv_cache["past_key_values." + std::to_string(layer) +
                          ".value"] = std::move(decoder_outputs[value_name]);
    }
  }

  // Autoregressive generation loop
  for (int step = 0; step < max_tokens; ++step) {
    // Extract logits from the last position
    std::vector<float>& all_logits = decoder_outputs["logits"];

    // Model outputs logits for ALL tokens in sequence: [batch_size,
    // sequence_length, vocab_size] We only need the last token: [batch_size,
    // vocab_size]
    const size_t expected_vocab_size = vocab_token_to_id_.size();
    const int64_t sequence_length = (step == 0)
                                        ? initial_sequence_length
                                        : (initial_sequence_length + step);
    const size_t expected_total_size = sequence_length * expected_vocab_size;

    LOG(INFO) << "Model logits size: " << all_logits.size()
              << ", Expected total size: " << expected_total_size
              << " (seq_len=" << sequence_length
              << " × vocab_size=" << expected_vocab_size << ")";

    // Extract only the last token's logits
    std::vector<float> last_token_logits;
    if (all_logits.size() >= expected_vocab_size) {
      // Take the last vocab_size values (logits for the last token)
      const size_t start_offset = all_logits.size() - expected_vocab_size;
      last_token_logits.assign(all_logits.begin() + start_offset,
                               all_logits.end());
      LOG(INFO) << "Extracted last token logits, size: "
                << last_token_logits.size();
    } else {
      LOG(ERROR) << "Model output too small, using all logits";
      last_token_logits = all_logits;
    }

    const size_t model_vocab_size = last_token_logits.size();

    // Handle vocabulary size mismatch between model and tokenizer
    std::vector<float> extended_logits;
    if (model_vocab_size < expected_vocab_size) {
      LOG(INFO) << "Extending logits from " << model_vocab_size << " to "
                << expected_vocab_size << " to include special tokens";

      // Reserve exact size to avoid reallocations during push_back
      const size_t missing_tokens = expected_vocab_size - model_vocab_size;
      extended_logits.reserve(expected_vocab_size);
      extended_logits = last_token_logits;  // Copy existing logits

      // Add logits for special tokens with pre-calculated values
      std::vector<float> special_token_logits(missing_tokens, -1e9f);

      for (size_t i = 0; i < missing_tokens; ++i) {
        int32_t token_id = static_cast<int32_t>(model_vocab_size + i);

        // Give special tokens reasonable probabilities
        if (special_tokens_.count("<|im_end|>") &&
            token_id == special_tokens_.at("<|im_end|>")) {
          special_token_logits[i] = 5.0f;  // High probability for EOS token
          LOG(INFO) << "Added EOS token <|im_end|> at position " << token_id
                    << " with logit 5.0";
        } else if (special_tokens_.count("<|endoftext|>") &&
                   token_id == special_tokens_.at("<|endoftext|>")) {
          special_token_logits[i] =
              3.0f;  // Moderate probability for alternative EOS
          LOG(INFO) << "Added EOS token <|endoftext|> at position " << token_id
                    << " with logit 3.0";
        }
        // Other special tokens already initialized to -1e9f
      }

      // Append all special token logits at once to avoid multiple reallocations
      extended_logits.insert(extended_logits.end(),
                             special_token_logits.begin(),
                             special_token_logits.end());
    } else {
      extended_logits = last_token_logits;
    }

    // We already have the last token's logits, no need to extract again
    int32_t next_token = ExtractNextToken(extended_logits);

    // Check for stop tokens - various EOS and structural tokens
    if (special_tokens_.count("<|im_end|>") &&
        next_token == special_tokens_.at("<|im_end|>")) {
      LOG(INFO) << "Found EOS token <|im_end|>, stopping generation after "
                << (step + 1) << " tokens";
      break;
    }
    if (special_tokens_.count("<|endoftext|>") &&
        next_token == special_tokens_.at("<|endoftext|>")) {
      LOG(INFO) << "Found EOS token <|endoftext|>, stopping generation after "
                << (step + 1) << " tokens";
      break;
    }
    if (special_tokens_.count("<|im_start|>") &&
        next_token == special_tokens_.at("<|im_start|>")) {
      LOG(INFO)
          << "Found structural token <|im_start|>, stopping generation after "
          << (step + 1) << " tokens";
      break;
    }

    // Additional stopping conditions for repetitive/low-quality output
    if (step > 100 && generated_tokens.size() >= 3) {
      // Check for simple repetition patterns
      if (next_token == generated_tokens[generated_tokens.size() - 1] &&
          next_token == generated_tokens[generated_tokens.size() - 2]) {
        LOG(WARNING)
            << "Detected token repetition, stopping generation early at step "
            << (step + 1);
        break;
      }
    }

    generated_tokens.push_back(next_token);

    // Debug: Show progress periodically
    if ((step + 1) % 10 == 0) {
      LOG(INFO) << "Generated " << (step + 1) << " tokens so far...";
    }

    // For next step: embed the new token and run inference
    if (step + 1 < max_tokens) {
      // Get embedding for the new token using proper int64 input
      std::vector<int32_t> single_token_vec = {next_token};
      std::vector<float> token_embedding;
      ExecuteTokenEmbedder(single_token_vec, token_embedding);

      if (token_embedding.empty()) {
        LOG(ERROR) << "Failed to embed new token " << next_token;
        break;
      }

      // Prepare inputs for next decoder step (only the new token embedding)
      std::map<std::string, std::vector<float>> next_decoder_inputs;
      next_decoder_inputs["inputs_embeds"] = token_embedding;

      // Position ID for the new token
      std::vector<int64_t> next_position_ids = {current_seq_len};
      current_seq_len++;  // Increment sequence length

      // Attention mask: attend to all previous tokens + the new one
      std::vector<int64_t> next_attention_mask(current_seq_len, 1);

      // Run decoder with the new token + KV cache from previous steps
      std::map<std::string, std::vector<float>> next_decoder_outputs;
      RunOnnxInferenceWithKeyValueCache(
          "decoder_model", next_decoder_inputs, next_position_ids,
          next_attention_mask, persistent_kv_cache, next_decoder_outputs);

      if (next_decoder_outputs.empty() ||
          next_decoder_outputs.find("logits") == next_decoder_outputs.end()) {
        LOG(ERROR) << "Failed to get logits for step " << step + 1;
        break;
      }

      // Update KV cache for next iteration
      for (int64_t layer = 0; layer < num_layers; ++layer) {
        std::string key_name = "present." + std::to_string(layer) + ".key";
        std::string value_name = "present." + std::to_string(layer) + ".value";

        if (next_decoder_outputs.count(key_name)) {
          persistent_kv_cache["past_key_values." + std::to_string(layer) +
                              ".key"] =
              std::move(next_decoder_outputs[key_name]);
        }
        if (next_decoder_outputs.count(value_name)) {
          persistent_kv_cache["past_key_values." + std::to_string(layer) +
                              ".value"] =
              std::move(next_decoder_outputs[value_name]);
        }
      }

      // Update decoder_outputs for next iteration
      decoder_outputs = std::move(next_decoder_outputs);
    }
  }

  if (static_cast<int>(generated_tokens.size()) >= max_tokens) {
    LOG(WARNING) << "Generation reached max_tokens limit of " << max_tokens
                 << " without finding EOS token";
  } else {
    LOG(INFO) << "Generation completed with EOS token after "
              << generated_tokens.size() << " tokens";
  }

  // Detokenize to get final text
  std::string generated_text = DetokenizeTokens(generated_tokens);

  InferenceResult result;
  result.success = true;
  result.generated_text = generated_text;

  // Decoder execution complete
  std::move(inference_callback).Run(std::move(result));
}

int32_t FastVLMExecutor::ExtractNextToken(const std::vector<float>& logits) {
  if (logits.empty()) {
    LOG(WARNING) << "Empty logits, returning default token";
    return special_tokens_.count("<|endoftext|>")
               ? special_tokens_.at("<|endoftext|>")
               : (special_tokens_.count("<|im_end|>")
                      ? special_tokens_.at("<|im_end|>")
                      : static_cast<int32_t>(vocab_token_to_id_.size() - 1));
  }

  // Filter out problematic tokens by setting their logits to very low values
  std::vector<float> filtered_logits = logits;

  // Block token 151642 (corrupted byte sequence 'â½Ĺ')
  if (151642 < filtered_logits.size()) {
    filtered_logits[151642] = -1e9f;  // Very low probability
    LOG(INFO) << "Blocked problematic token 151642";
  }

  // Block other high-numbered tokens that might be corrupted (last 2643 tokens
  // before special tokens)
  const int vocab_size = static_cast<int>(vocab_token_to_id_.size());
  const int special_token_start =
      vocab_size - 4;  // Assuming last 4 are special tokens
  const int block_start = std::max(0, special_token_start - 2643);
  for (int token_id = block_start; token_id < special_token_start; ++token_id) {
    if (token_id < static_cast<int>(filtered_logits.size())) {
      // Only block if it's not a known special token
      bool is_special = false;
      for (const auto& [token_name, id] : special_tokens_) {
        if (id == token_id) {
          is_special = true;
          break;
        }
      }
      if (!is_special) {
        filtered_logits[token_id] = -1e9f;
      }
    }
  }

  // Find the token with the highest logit value (greedy decoding)
  auto max_it =
      std::max_element(filtered_logits.begin(), filtered_logits.end());
  int32_t token_position = std::distance(filtered_logits.begin(), max_it);

  LOG(INFO) << "Selected token " << token_position << " with logit " << *max_it;

  // Validate token is in reasonable range
  if (token_position >= 0 &&
      token_position < static_cast<int32_t>(filtered_logits.size())) {
    // Check if it's a known vocabulary token
    if (vocab_id_to_token_.count(token_position)) {
      return token_position;
    }

    // Check if it's a known special token
    for (const auto& [token_name, id] : special_tokens_) {
      if (id == token_position) {
        LOG(INFO) << "Token " << token_position
                  << " is special token: " << token_name;
        return token_position;
      }
    }

    LOG(WARNING) << "Token " << token_position
                 << " not found in vocabulary or special tokens";

    // Return a safe fallback token instead of the unknown token
    return special_tokens_.count("<|endoftext|>")
               ? special_tokens_.at("<|endoftext|>")
               : (special_tokens_.count("<|im_end|>")
                      ? special_tokens_.at("<|im_end|>")
                      : static_cast<int32_t>(vocab_token_to_id_.size() - 1));
  } else {
    LOG(ERROR) << "Invalid token position: " << token_position;
    return special_tokens_.count("<|endoftext|>")
               ? special_tokens_.at("<|endoftext|>")
               : (special_tokens_.count("<|im_end|>")
                      ? special_tokens_.at("<|im_end|>")
                      : static_cast<int32_t>(vocab_token_to_id_.size() - 1));
  }
}

bool FastVLMExecutor::CreateOnnxSession(const std::string& model_path,
                                        const std::string& model_type) {
  LOG(INFO) << "Creating ONNX Runtime session for " << model_type << " from "
            << model_path;

  if (!ort_environment_) {
    LOG(ERROR) << "ORT environment not initialized";
    return false;
  }

  // Get the ORT API from platform functions
  auto* platform_functions = ort::PlatformFunctions::GetInstance();
  if (!platform_functions || !platform_functions->ort_api()) {
    LOG(ERROR) << "ORT API not available";
    return false;
  }

  const OrtApi* ort_api = platform_functions->ort_api();

  // Create session options using RAII with Receiver pattern
  ort::ScopedOrtSessionOptions session_options;
  if (ORT_CALL_FAILED(ort_api->CreateSessionOptions(
          ort::ScopedOrtSessionOptions::Receiver(session_options).get()))) {
    LOG(ERROR) << "Failed to create session options";
    return false;
  }

  // Create session using RAII with Receiver pattern
  ort::ScopedOrtSession session;
  if (ORT_CALL_FAILED(ort_api->CreateSession(
          ort_environment_->get(), model_path.c_str(), session_options.get(),
          ort::ScopedOrtSession::Receiver(session).get()))) {
    LOG(ERROR) << "Failed to create ONNX session for " << model_type;
    return false;
  }

  if (!session.get()) {
    LOG(ERROR) << "Failed to create ONNX session for " << model_type;
    return false;
  }

  // Store the session
  ort_sessions_[model_type] = std::move(session);

  LOG(INFO) << "Successfully created ONNX session for " << model_type;
  return true;
}

template <typename T>
void FastVLMExecutor::RunOnnxInferenceTemplate(
    const std::string& model_name,
    const std::string& input_name,
    const std::vector<T>& input_data,
    std::map<std::string, std::vector<float>>& outputs) {
  LOG(INFO) << "Running ONNX inference for " << model_name << " with "
            << (std::is_same_v<T, float> ? "float" : "integer") << " input";

  // Check if we have a session for this model
  auto session_it = ort_sessions_.find(model_name);
  if (session_it == ort_sessions_.end() || !session_it->second.get()) {
    LOG(ERROR) << "ONNX session not found for model: " << model_name;
    return;
  }

  // Get the ORT API
  auto* platform_functions = ort::PlatformFunctions::GetInstance();
  if (!platform_functions || !platform_functions->ort_api()) {
    LOG(ERROR) << "ORT API not available";
    return;
  }

  const OrtApi* ort_api = platform_functions->ort_api();
  OrtSession* session = session_it->second.get();

  // Create input tensors using RAII
  std::vector<const char*> input_names_vec = {input_name.c_str()};
  std::vector<ort::ScopedOrtValue> input_tensors_storage;
  std::vector<OrtValue*> input_tensors;

  input_tensors_storage.reserve(1);
  input_tensors.reserve(1);

  // Get allocator using RAII with Receiver pattern
  ort::ScopedOrtMemoryInfo memory_info;
  if (ORT_CALL_FAILED(ort_api->CreateCpuMemoryInfo(
          OrtArenaAllocator, OrtMemTypeDefault,
          ort::ScopedOrtMemoryInfo::Receiver(memory_info).get()))) {
    LOG(ERROR) << "Failed to create memory info";
    return;
  }

  // Get input shape - find from model metadata
  std::vector<int64_t> shape;
  bool shape_found = false;

  for (const auto& [model_type, input_shapes] : model_input_shapes_) {
    if (input_shapes.find(input_name) != input_shapes.end()) {
      std::vector<int64_t> template_shape = input_shapes.at(input_name);
      if (input_name == "input_ids" && template_shape.size() >= 2) {
        shape = template_shape;
        shape[0] = 1;                                        // batch_size
        shape[1] = static_cast<int64_t>(input_data.size());  // sequence_length
        shape_found = true;
        break;
      } else if (input_name == "pixel_values" && template_shape.size() >= 4) {
        shape = template_shape;
        shape[0] = 1;  // batch_size = 1
        shape_found = true;
        break;
      } else {
        shape = template_shape;
        if (shape.size() > 0) {
          shape[0] = 1;  // batch_size = 1
        }
        shape_found = true;
        break;
      }
    }
  }

  if (!shape_found) {
    LOG(WARNING) << "Could not find shape for input " << input_name
                 << " in model metadata, using data size";
    shape = {1, static_cast<int64_t>(input_data.size())};
  }

  // Create tensor with appropriate data type
  ort::ScopedOrtValue input_tensor;

  if constexpr (std::is_same_v<T, float>) {
    // Float tensor
    if (ORT_CALL_FAILED(ort_api->CreateTensorWithDataAsOrtValue(
            memory_info.get(), const_cast<T*>(input_data.data()),
            input_data.size() * sizeof(T), shape.data(), shape.size(),
            ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
            ort::ScopedOrtValue::Receiver(input_tensor).get()))) {
      LOG(ERROR) << "Failed to create float input tensor for: " << input_name;
      return;
    }
  } else {
    // Integer tensor (int64)
    if (ORT_CALL_FAILED(ort_api->CreateTensorWithDataAsOrtValue(
            memory_info.get(), const_cast<T*>(input_data.data()),
            input_data.size() * sizeof(T), shape.data(), shape.size(),
            ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64,
            ort::ScopedOrtValue::Receiver(input_tensor).get()))) {
      LOG(ERROR) << "Failed to create integer input tensor for: " << input_name;
      return;
    }
  }

  input_tensors_storage.push_back(std::move(input_tensor));
  input_tensors.push_back(input_tensors_storage.back().get());

  LOG(INFO) << "Created input tensor '" << input_name << "' with shape: [";
  for (size_t i = 0; i < shape.size(); ++i) {
    LOG(INFO) << (i > 0 ? ", " : "") << shape[i];
  }
  LOG(INFO) << "] and " << input_data.size() << " elements";

  // Get output names and allocate output tensors
  OrtAllocator* allocator = nullptr;
  auto allocator_status = ort::CreateScopedStatus(
      ort_api->GetAllocatorWithDefaultOptions(&allocator));
  if (allocator_status.get()) {
    LOG(ERROR) << "Failed to get allocator: "
               << ort_api->GetErrorMessage(allocator_status.get());
    return;
  }

  // Get number of outputs
  size_t num_outputs = 0;
  auto output_count_status = ort::CreateScopedStatus(
      ort_api->SessionGetOutputCount(session, &num_outputs));
  if (output_count_status.get()) {
    LOG(ERROR) << "Failed to get output count: "
               << ort_api->GetErrorMessage(output_count_status.get());
    return;
  }

  // Get output names
  std::vector<std::string> output_names_storage(num_outputs);
  std::vector<const char*> output_names_vec(num_outputs);

  for (size_t i = 0; i < num_outputs; ++i) {
    char* output_name = nullptr;
    auto name_status = ort::CreateScopedStatus(
        ort_api->SessionGetOutputName(session, i, allocator, &output_name));
    if (name_status.get()) {
      LOG(ERROR) << "Failed to get output name " << i << ": "
                 << ort_api->GetErrorMessage(name_status.get());
      return;
    }
    output_names_storage[i] = std::string(output_name);
    output_names_vec[i] = output_names_storage[i].c_str();

    // Free the allocated name using the allocator
    auto free_status =
        ort::CreateScopedStatus(ort_api->AllocatorFree(allocator, output_name));
    if (free_status.get()) {
      LOG(WARNING) << "Failed to free output name: "
                   << ort_api->GetErrorMessage(free_status.get());
    }
  }

  // Run inference
  std::vector<OrtValue*> output_tensors(num_outputs, nullptr);
  auto run_status = ort::CreateScopedStatus(ort_api->Run(
      session,
      nullptr,  // run options
      input_names_vec.data(), input_tensors.data(), input_tensors.size(),
      output_names_vec.data(), num_outputs, output_tensors.data()));

  if (run_status.get()) {
    LOG(ERROR) << "Failed to run ONNX inference: "
               << ort_api->GetErrorMessage(run_status.get());
    return;
  }

  // Extract output data using RAII
  outputs.clear();
  for (size_t i = 0; i < output_tensors.size(); ++i) {
    if (output_tensors[i]) {
      // Wrap output tensor in scoped type for RAII
      ort::ScopedOrtValue scoped_output_tensor(output_tensors[i]);

      // Get tensor data
      float* output_data = nullptr;
      auto data_status = ort::CreateScopedStatus(ort_api->GetTensorMutableData(
          scoped_output_tensor.get(), reinterpret_cast<void**>(&output_data)));
      if (data_status.get()) {
        LOG(ERROR) << "Failed to get tensor data: "
                   << ort_api->GetErrorMessage(data_status.get());
        continue;
      }

      // Get tensor shape info using RAII with Receiver pattern
      ort::ScopedOrtTensorTypeAndShapeInfo type_info;
      if (ORT_CALL_FAILED(ort_api->GetTensorTypeAndShape(
              scoped_output_tensor.get(),
              ort::ScopedOrtTensorTypeAndShapeInfo::Receiver(type_info)
                  .get()))) {
        LOG(ERROR) << "Failed to get tensor shape info";
        continue;
      }

      size_t num_elements = 0;
      auto element_count_status = ort::CreateScopedStatus(
          ort_api->GetTensorShapeElementCount(type_info.get(), &num_elements));
      if (element_count_status.get()) {
        LOG(ERROR) << "Failed to get element count: "
                   << ort_api->GetErrorMessage(element_count_status.get());
        continue;
      }

      // Copy output data from ORT tensor
      if (output_data && num_elements > 0) {
        UNSAFE_BUFFERS({
          std::vector<float> output_vec(output_data,
                                        output_data + num_elements);
          outputs[output_names_storage[i]] = std::move(output_vec);
        });
        LOG(INFO) << "Extracted output '" << output_names_storage[i]
                  << "' with " << num_elements << " elements";
      }
      // Scoped types automatically clean up when they go out of scope
    }
  }

  LOG(INFO) << "ONNX inference completed for " << model_name << ", produced "
            << outputs.size() << " outputs";
}

void FastVLMExecutor::RunOnnxInference(
    const std::string& model_name,
    const std::map<std::string, std::vector<float>>& inputs,
    std::map<std::string, std::vector<float>>& outputs) {
  LOG(INFO) << "Running ONNX inference for " << model_name;

  // Check if we have a session for this model
  auto session_it = ort_sessions_.find(model_name);
  if (session_it == ort_sessions_.end() || !session_it->second.get()) {
    LOG(ERROR) << "ONNX session not found for model: " << model_name;
    return;
  }

  // Get the ORT API
  auto* platform_functions = ort::PlatformFunctions::GetInstance();
  if (!platform_functions || !platform_functions->ort_api()) {
    LOG(ERROR) << "ORT API not available";
    return;
  }

  const OrtApi* ort_api = platform_functions->ort_api();
  OrtSession* session = session_it->second.get();

  // Create input tensors
  std::vector<std::string> input_names_storage;
  std::vector<const char*> input_names;
  std::vector<ort::ScopedOrtValue> input_tensors_storage;
  std::vector<OrtValue*> input_tensors;

  input_names_storage.reserve(inputs.size());
  input_names.reserve(inputs.size());
  input_tensors_storage.reserve(inputs.size());
  input_tensors.reserve(inputs.size());

  for (const auto& [name, data] : inputs) {
    // Get input shape from extracted metadata
    std::vector<int64_t> shape;
    bool shape_found = false;

    for (const auto& [model_type, input_shapes] : model_input_shapes_) {
      if (input_shapes.find(name) != input_shapes.end()) {
        std::vector<int64_t> template_shape = input_shapes.at(name);
        shape = template_shape;

        // Handle dynamic dimensions
        if (name == "pixel_values" && template_shape.size() == 4) {
          shape[0] = 1;                // batch_size
          shape[1] = image_channels_;  // channels
          shape[2] = image_height_;    // height
          shape[3] = image_width_;     // width
          shape_found = true;
          break;
        } else if (name == "input_ids" && template_shape.size() >= 2) {
          shape[0] = 1;                                  // batch_size
          shape[1] = static_cast<int64_t>(data.size());  // sequence_length
          shape_found = true;
          break;
        } else if (name == "inputs_embeds" && template_shape.size() >= 3) {
          const int64_t total_sequence_length = data.size() / hidden_size_;
          if (data.size() % hidden_size_ == 0) {
            shape[0] = 1;                      // batch_size
            shape[1] = total_sequence_length;  // sequence_length
            shape[2] = hidden_size_;           // hidden_size
            shape_found = true;
            break;
          }
        }
      }
    }

    if (!shape_found) {
      LOG(ERROR) << "Could not determine shape for input: " << name;
      continue;
    }

    // Create memory info using RAII with Receiver pattern
    ort::ScopedOrtMemoryInfo memory_info;
    if (ORT_CALL_FAILED(ort_api->CreateCpuMemoryInfo(
            OrtArenaAllocator, OrtMemTypeDefault,
            ort::ScopedOrtMemoryInfo::Receiver(memory_info).get()))) {
      LOG(ERROR) << "Failed to create memory info";
      continue;
    }

    // Create input tensor using RAII with Receiver pattern
    ort::ScopedOrtValue input_tensor;
    if (ORT_CALL_FAILED(ort_api->CreateTensorWithDataAsOrtValue(
            memory_info.get(),
            const_cast<float*>(data.data()),  // ORT expects non-const pointer
            data.size() * sizeof(float), shape.data(), shape.size(),
            ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
            ort::ScopedOrtValue::Receiver(input_tensor).get()))) {
      LOG(ERROR) << "Failed to create input tensor for: " << name;
      continue;
    }

    if (!input_tensor.get()) {
      LOG(ERROR) << "Failed to create input tensor for: " << name;
      continue;
    }

    // Store everything
    input_names_storage.push_back(name);
    input_names.push_back(input_names_storage.back().c_str());
    input_tensors_storage.push_back(std::move(input_tensor));
    input_tensors.push_back(input_tensors_storage.back().get());
  }

  if (input_tensors.empty()) {
    LOG(ERROR) << "No valid input tensors created";
    return;
  }

  // Get output names directly from ONNX Runtime session
  std::vector<std::string> output_names_storage;
  std::vector<const char*> output_names;

  size_t num_outputs = 0;
  auto count_status = ort::CreateScopedStatus(
      ort_api->SessionGetOutputCount(session, &num_outputs));
  if (count_status.get()) {
    LOG(ERROR) << "Failed to get output count: "
               << ort_api->GetErrorMessage(count_status.get());
    return;
  }

  // Get allocator for string allocation
  OrtAllocator* allocator = nullptr;
  auto allocator_status = ort::CreateScopedStatus(
      ort_api->GetAllocatorWithDefaultOptions(&allocator));
  if (allocator_status.get()) {
    LOG(ERROR) << "Failed to get allocator: "
               << ort_api->GetErrorMessage(allocator_status.get());
    return;
  }

  output_names_storage.reserve(num_outputs);
  output_names.reserve(num_outputs);

  for (size_t i = 0; i < num_outputs; ++i) {
    char* output_name = nullptr;
    auto name_status = ort::CreateScopedStatus(
        ort_api->SessionGetOutputName(session, i, allocator, &output_name));
    if (name_status.get()) {
      LOG(ERROR) << "Failed to get output name " << i << ": "
                 << ort_api->GetErrorMessage(name_status.get());
      continue;
    }

    if (output_name) {
      output_names_storage.emplace_back(output_name);
      output_names.push_back(output_names_storage.back().c_str());
      (void)ort_api->AllocatorFree(
          allocator,
          output_name);  // Free the allocated name with proper allocator
    }
  }

  if (output_names.empty()) {
    LOG(ERROR) << "No outputs found for model session: " << model_name;
    return;
  }

  LOG(INFO) << "Found " << num_outputs << " outputs for model " << model_name;

  // Run inference
  std::vector<OrtValue*> output_tensors(output_names.size(), nullptr);

  auto run_status = ort::CreateScopedStatus(ort_api->Run(
      session,
      nullptr,  // run_options
      input_names.data(), input_tensors.data(), input_tensors.size(),
      output_names.data(), output_names.size(), output_tensors.data()));

  if (run_status.get()) {
    LOG(ERROR) << "Failed to run ONNX inference: "
               << ort_api->GetErrorMessage(run_status.get());
    return;
  }

  // Extract output data using RAII
  outputs.clear();
  for (size_t i = 0; i < output_tensors.size(); ++i) {
    if (output_tensors[i]) {
      // Wrap output tensor in scoped type for RAII
      ort::ScopedOrtValue scoped_output_tensor(output_tensors[i]);

      // Get tensor data
      float* output_data = nullptr;
      auto data_status = ort::CreateScopedStatus(ort_api->GetTensorMutableData(
          scoped_output_tensor.get(), reinterpret_cast<void**>(&output_data)));
      if (data_status.get()) {
        LOG(ERROR) << "Failed to get tensor data: "
                   << ort_api->GetErrorMessage(data_status.get());
        continue;
      }

      // Get tensor shape info using RAII with Receiver pattern
      ort::ScopedOrtTensorTypeAndShapeInfo type_info;
      if (ORT_CALL_FAILED(ort_api->GetTensorTypeAndShape(
              scoped_output_tensor.get(),
              ort::ScopedOrtTensorTypeAndShapeInfo::Receiver(type_info)
                  .get()))) {
        LOG(ERROR) << "Failed to get tensor shape info";
        continue;
      }

      size_t num_elements = 0;
      auto element_count_status = ort::CreateScopedStatus(
          ort_api->GetTensorShapeElementCount(type_info.get(), &num_elements));
      if (element_count_status.get()) {
        LOG(ERROR) << "Failed to get element count: "
                   << ort_api->GetErrorMessage(element_count_status.get());
        continue;
      }

      // Copy output data from ORT tensor
      if (output_data && num_elements > 0) {
        UNSAFE_BUFFERS({
          std::vector<float> output_vec(output_data,
                                        output_data + num_elements);
          outputs[output_names_storage[i]] = std::move(output_vec);
        });
        LOG(INFO) << "Extracted output '" << output_names_storage[i]
                  << "' with " << num_elements << " elements";
      }
      // Scoped types automatically clean up when they go out of scope
    }
  }

  LOG(INFO) << "ONNX inference completed for " << model_name << ", produced "
            << outputs.size() << " outputs";
}

void FastVLMExecutor::RunOnnxInferenceWithKeyValueCache(
    const std::string& model_name,
    const std::map<std::string, std::vector<float>>& inputs,
    const std::vector<int64_t>& position_ids,
    const std::vector<int64_t>& attention_mask,
    const std::map<std::string, std::vector<float>>& past_key_values,
    std::map<std::string, std::vector<float>>& outputs) {
  LOG(INFO) << "Running ONNX inference with KV cache for " << model_name;

  // For now, implement a simplified version that calls the basic
  // RunOnnxInference
  // TODO: Implement full KV cache support

  // Map the model name to the correct session - decoder_model ->
  // decoder_model_merged
  std::string session_name = model_name;
  if (model_name == "decoder_model") {
    session_name = "decoder_model_merged";
  }

  // Prepare inputs (convert int64 to float as needed)
  std::map<std::string, std::vector<float>> all_inputs = inputs;

  // Convert position_ids to float
  if (!position_ids.empty()) {
    std::vector<float> position_ids_float;
    position_ids_float.reserve(position_ids.size());
    for (int64_t id : position_ids) {
      position_ids_float.push_back(static_cast<float>(id));
    }
    all_inputs["position_ids"] = position_ids_float;
  }

  // Convert attention_mask to float
  if (!attention_mask.empty()) {
    std::vector<float> attention_mask_float;
    attention_mask_float.reserve(attention_mask.size());
    for (int64_t mask : attention_mask) {
      attention_mask_float.push_back(static_cast<float>(mask));
    }
    all_inputs["attention_mask"] = attention_mask_float;
  }

  // Add past key values to inputs (for KV cache)
  for (const auto& [key, value] : past_key_values) {
    if (!value.empty()) {
      all_inputs[key] = value;
    }
  }

  // Call the basic inference method
  RunOnnxInference(session_name, all_inputs, outputs);

  LOG(INFO) << "KV cache inference completed for " << model_name
            << ", produced " << outputs.size() << " outputs";
}

bool FastVLMExecutor::TryLoadPreprocessorConfig() {
  base::FilePath config_file =
      model_dir_.AppendASCII("preprocessor_config.json");

  std::string config_json;
  if (!base::ReadFileToString(config_file, &config_json)) {
    LOG(WARNING) << "[LocalAI] Could not read preprocessor_config.json";
    return false;
  }

  // Parse JSON using Chromium's JSON reader
  auto parsed_json = base::JSONReader::ReadAndReturnValueWithError(config_json);
  if (!parsed_json.has_value()) {
    LOG(ERROR) << "[LocalAI] Failed to parse preprocessor_config.json: "
               << parsed_json.error().message;
    return false;
  }

  const base::Value& root = parsed_json.value();
  if (!root.is_dict()) {
    LOG(ERROR) << "[LocalAI] preprocessor_config.json is not a valid object";
    return false;
  }

  // Follow Transformers.js logic: config.size ?? config.image_size
  const base::Value* size_value = root.GetDict().Find("size");
  if (!size_value) {
    size_value = root.GetDict().Find("image_size");
  }

  if (!size_value) {
    LOG(WARNING) << "[LocalAI] No 'size' or 'image_size' found in "
                    "preprocessor_config.json";
    return false;
  }

  // Handle different size formats (matching Transformers.js)
  if (size_value->is_int()) {
    // Single number format (square image)
    int size = size_value->GetInt();
    if (size > 0) {
      image_height_ = size;
      image_width_ = size;
      LOG(INFO) << "[LocalAI] Loaded square image size from config: " << size
                << "x" << size;
      return true;
    }
  } else if (size_value->is_dict()) {
    // Object format: {"height": X, "width": Y}
    const base::Value::Dict& size_dict = size_value->GetDict();

    const base::Value* height_val = size_dict.Find("height");
    const base::Value* width_val = size_dict.Find("width");

    if (height_val && width_val && height_val->is_int() &&
        width_val->is_int()) {
      int height = height_val->GetInt();
      int width = width_val->GetInt();

      if (height > 0 && width > 0) {
        image_height_ = height;
        image_width_ = width;
        LOG(INFO) << "[LocalAI] Loaded image dimensions from config: " << height
                  << "x" << width;
        return true;
      }
    }

    // Also support shortest_edge/longest_edge for compatibility
    const base::Value* shortest_edge = size_dict.Find("shortest_edge");
    if (shortest_edge && shortest_edge->is_int()) {
      int size = shortest_edge->GetInt();
      if (size > 0) {
        image_height_ = size;
        image_width_ = size;
        LOG(INFO) << "[LocalAI] Loaded square image size from shortest_edge: "
                  << size << "x" << size;
        return true;
      }
    }
  }

  LOG(WARNING) << "[LocalAI] Could not parse size information from "
                  "preprocessor_config.json";
  return false;
}

}  // namespace local_ai
