// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_BROWSER_FAST_VLM_EXECUTOR_H_
#define BRAVE_COMPONENTS_LOCAL_AI_BROWSER_FAST_VLM_EXECUTOR_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/ort/scoped_ort_types.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

class SkBitmap;

namespace local_ai::ort {
class Environment;
}  // namespace local_ai::ort

namespace local_ai {

struct InferenceRequest {
  InferenceRequest();
  ~InferenceRequest();
  InferenceRequest(const InferenceRequest&);
  InferenceRequest& operator=(const InferenceRequest&);
  InferenceRequest(InferenceRequest&&);
  InferenceRequest& operator=(InferenceRequest&&);

  std::vector<uint8_t> image_data;
  std::string text_prompt;
  int max_tokens = 512;
};

struct InferenceResult {
  InferenceResult();
  ~InferenceResult();
  InferenceResult(const InferenceResult&);
  InferenceResult& operator=(const InferenceResult&);
  InferenceResult(InferenceResult&&);
  InferenceResult& operator=(InferenceResult&&);

  bool success = false;
  std::string generated_text;
  std::string error_message;
};

class FastVLMExecutor {
 public:
  FastVLMExecutor();
  ~FastVLMExecutor();
  FastVLMExecutor(const FastVLMExecutor&) = delete;
  FastVLMExecutor& operator=(const FastVLMExecutor&) = delete;

  using InferenceCallback = base::OnceCallback<void(InferenceResult)>;

  // Initialize ONNX Runtime for model execution
  void InitializeOnnxRuntime();

  void LoadModel(const base::FilePath& model_dir,
                 base::OnceCallback<void(bool)> callback);
  void RunInference(const InferenceRequest& request,
                    InferenceCallback callback);

 private:
  enum class LoadState { kUninitialized, kLoadingModels, kReady, kError };

  // Model loading pipeline
  void LoadOnnxModels(base::OnceCallback<void(bool)> callback);
  void LoadOnnxModelsWithFallback(base::OnceCallback<void(bool)> callback);
  void LoadOnnxModelsWithWebNN(base::OnceCallback<void(bool)> callback);

  // Model classification and metadata extraction helpers
  void ExtractModelMetadata(const std::string& model_path,
                            const std::string& model_type);
  bool IsPreferredVariant(const std::string& new_filename,
                          const std::string& existing_filename);

  // Tokenizer loading
  bool LoadTokenizerFiles();

  // Chat template method
  std::string ApplyChatTemplate(const std::string& user_message,
                                bool add_generation_prompt = true);

  // Model configuration loading
  bool LoadModelConfig();

  // Direct ONNX Runtime execution
  void ExecuteVisionEncoder(const std::vector<float>& pixel_values,
                            std::vector<float>& image_features);
  void ExecuteTokenEmbedder(const std::vector<int32_t>& input_ids,
                            std::vector<float>& embeddings);

  // Real decoder execution using ONNX Runtime
  void ExecuteRealDecoder(const std::vector<float>& vision_features,
                          const std::vector<float>& text_embeddings,
                          const std::vector<int32_t>& token_ids,
                          int max_tokens,
                          InferenceCallback callback);

  // Token processing
  int32_t ExtractNextToken(const std::vector<float>& logits);

  // Inference pipeline
  void RunVisionEncoding(const std::vector<uint8_t>& image_data,
                         base::OnceCallback<void(std::vector<float>)> callback);
  void RunTokenEmbedding(const std::string& text_prompt,
                         base::OnceCallback<void(std::vector<float>)> callback);
  void RunDecoding(const std::vector<float>& vision_features,
                   const std::vector<float>& text_embeddings,
                   const std::vector<int32_t>& token_ids,
                   int max_tokens,
                   InferenceCallback callback);

  // Image and text processing
  void ProcessImageDataAsync(
      const std::vector<uint8_t>& image_data,
      base::OnceCallback<void(std::vector<float>)> callback);
  void OnImageDecoded(base::OnceCallback<void(std::vector<float>)> callback,
                      const SkBitmap& decoded_bitmap);
  void OnImageProcessed(
      base::OnceCallback<void(std::vector<float>)> vision_callback,
      std::vector<float> processed_image);

  std::vector<int32_t> TokenizeText(const std::string& text);
  std::vector<int32_t> SimpleTokenizeText(const std::string& formatted_text);
  std::vector<std::string> SimpleTokenizeWord(const std::string& word);
  std::string DetokenizeTokens(const std::vector<int32_t>& tokens);

  // ONNX Runtime session management
  bool CreateOnnxSession(const std::string& model_path,
                         const std::string& model_type);
  // Unified ONNX inference with automatic type detection
  struct TensorData {
    std::vector<float> float_data;
    std::vector<int64_t> int64_data;
    std::vector<int32_t> int32_data;
    ONNXTensorElementDataType data_type;
    std::vector<int64_t> shape;

    TensorData();
    ~TensorData();
  };

  void RunOnnxInference(const std::string& model_name,
                        const std::map<std::string, TensorData>& inputs,
                        std::map<std::string, TensorData>& outputs);

  void RunOnnxInferenceWithKeyValueCache(
      const std::string& model_name,
      const std::map<std::string, std::vector<float>>& inputs,
      const std::vector<int64_t>& position_ids,
      const std::vector<int64_t>& attention_mask,
      const std::map<std::string, std::vector<float>>& past_key_values,
      std::map<std::string, std::vector<float>>& outputs);

  // Vision configuration helpers
  std::pair<int, int> GetImageDimensions() const {
    return {image_height_, image_width_};
  }

  // Preprocessor configuration loading
  bool TryLoadPreprocessorConfig();

  // Inference pipeline callbacks
  void OnVisionEncodingComplete(const std::string& text_prompt,
                                int max_tokens,
                                InferenceCallback callback,
                                std::vector<float> vision_features);
  void OnTokenEmbeddingComplete(const std::vector<float>& vision_features,
                                const std::string& text_prompt,
                                int max_tokens,
                                InferenceCallback callback,
                                std::vector<float> text_embeddings);

  // State
  LoadState state_ = LoadState::kUninitialized;
  base::FilePath model_dir_;
  std::string last_error_;

  // Model file information
  std::map<std::string, int64_t> model_file_sizes_;

  // Model input shape information extracted from ONNX models
  // (output shapes are obtained dynamically from ONNX Runtime)
  std::map<std::string, std::map<std::string, std::vector<int64_t>>>
      model_input_shapes_;  // model_type -> input_name -> shape

  // Tokenizer data
  std::map<int32_t, std::string> vocab_id_to_token_;
  std::map<std::string, int32_t> vocab_token_to_id_;
  std::map<std::string, int32_t> special_tokens_;

  // Chat template
  std::string chat_template_;

  // Model configuration
  int64_t hidden_size_ = 896;         // Default from FastVLM-0.5B config
  int64_t num_hidden_layers_ = 24;    // Default from FastVLM-0.5B config
  int64_t num_attention_heads_ = 14;  // Default from FastVLM-0.5B config
  int64_t num_key_value_heads_ = 2;   // Default from FastVLM-0.5B config

  // Vision processing configuration (extracted from vision_encoder model)
  int image_height_ =
      1024;  // Default, will be updated from model metadata or config
  int image_width_ =
      1024;  // Default, will be updated from model metadata or config
  int image_channels_ = 3;  // Default, will be updated from model metadata

  // ONNX Runtime components
  std::map<std::string, std::string> model_paths_;
  std::vector<std::string> available_providers_;
  bool onnx_runtime_initialized_ = false;

  // ONNX Runtime wrapper components
  scoped_refptr<ort::Environment> ort_environment_;
  std::map<std::string, ort::ScopedOrtSession> ort_sessions_;

  base::WeakPtrFactory<FastVLMExecutor> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_BROWSER_FAST_VLM_EXECUTOR_H_
