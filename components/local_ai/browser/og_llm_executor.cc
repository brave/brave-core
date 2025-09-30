// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/og_llm_executor.h"

#include "base/logging.h"
#include "brave/components/local_ai/og/og_status.h"
#include "ort_genai_c.h"

namespace local_ai {

// OgLlmExecutor implementation
OgLlmExecutor::OgLlmExecutor() = default;
OgLlmExecutor::~OgLlmExecutor() = default;

bool OgLlmExecutor::Initialize(const base::FilePath& library_path,
                               const base::FilePath& model_path) {
  // Load the ONNX Runtime GenAI library if not already loaded
  auto* platform = og::PlatformFunctions::GetInstance();
  if (!platform->IsInitialized()) {
    platform = og::PlatformFunctions::GetInstance(library_path);
    if (!platform->IsInitialized()) {
      LOG(ERROR) << "Failed to load ONNX Runtime GenAI library from: "
                 << library_path;
      return false;
    }
  }

  model_ = std::make_unique<og::Model>();
  if (!model_->Load(model_path)) {
    LOG(ERROR) << "Failed to load model from: " << model_path;
    model_.reset();
    return false;
  }

  tokenizer_ = std::make_unique<og::Tokenizer>(*model_);
  if (!tokenizer_->IsValid()) {
    LOG(ERROR) << "Failed to create tokenizer";
    model_.reset();
    tokenizer_.reset();
    return false;
  }

  tokenizer_stream_ = std::make_unique<og::TokenizerStream>(*tokenizer_);
  if (!tokenizer_stream_->IsValid()) {
    LOG(ERROR) << "Failed to create tokenizer stream";
    model_.reset();
    tokenizer_.reset();
    tokenizer_stream_.reset();
    return false;
  }

  return true;
}

bool OgLlmExecutor::EncodePrompt(const std::string& prompt,
                                 og::Sequences& sequences) {
  if (!tokenizer_ || !tokenizer_->IsValid()) {
    return false;
  }

  OgaResult* result = OGA_CALL(OgaTokenizerEncode)(
      tokenizer_->get(), prompt.c_str(), sequences.get());
  return og::CheckResult(result);
}

void OgLlmExecutor::Generate(const std::string& prompt,
                             int max_tokens,
                             TokenCallback token_callback,
                             CompletionCallback completion_callback) {
  if (!IsInitialized()) {
    LOG(ERROR) << "Executor not initialized";
    std::move(completion_callback).Run(false);
    return;
  }

  // Create generator params
  auto params = std::make_unique<og::GeneratorParams>(*model_);
  if (!params->IsValid()) {
    LOG(ERROR) << "Failed to create generator params";
    std::move(completion_callback).Run(false);
    return;
  }

  // Set max_length
  if (!params->SetSearchNumber("max_length", static_cast<double>(max_tokens))) {
    LOG(ERROR) << "Failed to set max_length";
    std::move(completion_callback).Run(false);
    return;
  }

  // Encode the prompt
  auto sequences = std::make_unique<og::Sequences>();
  if (!sequences->IsValid()) {
    LOG(ERROR) << "Failed to create sequences";
    std::move(completion_callback).Run(false);
    return;
  }

  if (!EncodePrompt(prompt, *sequences)) {
    LOG(ERROR) << "Failed to encode prompt";
    std::move(completion_callback).Run(false);
    return;
  }

  // Create generator
  auto generator = std::make_unique<og::Generator>(*model_, *params);
  if (!generator->IsValid()) {
    LOG(ERROR) << "Failed to create generator";
    std::move(completion_callback).Run(false);
    return;
  }

  // Append token sequences
  if (!generator->AppendTokenSequences(*sequences)) {
    LOG(ERROR) << "Failed to append token sequences";
    std::move(completion_callback).Run(false);
    return;
  }

  // Get initial token count
  const size_t prompt_token_count =
      OGA_CALL(OgaSequencesGetSequenceCount)(sequences->get(), 0);

  // Generate tokens
  bool success = true;
  while (!generator->IsDone()) {
    if (!generator->GenerateNextToken()) {
      LOG(ERROR) << "Failed to generate next token";
      success = false;
      break;
    }

    const size_t num_tokens = generator->GetSequenceCount(0);
    if (num_tokens <= prompt_token_count) {
      continue;
    }

    const int32_t* sequence_data = generator->GetSequenceData(0);
    if (!sequence_data) {
      LOG(ERROR) << "Failed to get sequence data";
      success = false;
      break;
    }

    // SAFETY: sequence_data is guaranteed to have num_tokens elements by the
    // OnnxRuntime GenAI API. We've already checked num_tokens > 0 and
    // sequence_data != nullptr above.
    const int32_t new_token = UNSAFE_BUFFERS(sequence_data[num_tokens - 1]);
    std::string token_string = tokenizer_stream_->Decode(new_token);

    if (!token_string.empty()) {
      token_callback.Run(token_string);
    }
  }

  std::move(completion_callback).Run(success);
}

void OgLlmExecutor::GenerateWithImage(
    const std::string& prompt,
    const std::vector<base::FilePath>& image_paths,
    int max_tokens,
    TokenCallback token_callback,
    CompletionCallback completion_callback) {
  if (!IsInitialized()) {
    LOG(ERROR) << "Executor not initialized";
    std::move(completion_callback).Run(false);
    return;
  }

  // Create multimodal processor
  auto processor = std::make_unique<og::MultiModalProcessor>(*model_);
  if (!processor->IsValid()) {
    LOG(ERROR) << "Failed to create multimodal processor";
    std::move(completion_callback).Run(false);
    return;
  }

  // Load images if provided
  std::unique_ptr<og::Images> images;
  if (!image_paths.empty()) {
    images = std::make_unique<og::Images>();
    if (!images->Load(image_paths)) {
      LOG(ERROR) << "Failed to load images";
      std::move(completion_callback).Run(false);
      return;
    }
  }

  // Construct messages in JSON format for phi3v model
  std::string content;
  if (!image_paths.empty()) {
    for (size_t i = 0; i < image_paths.size(); ++i) {
      content += "<|image_" + std::to_string(i + 1) + "|>\\n";
    }
  }
  content += prompt;
  std::string messages =
      R"([{"role": "user", "content": ")" + content + R"("}])";

  // Apply chat template to format the prompt
  std::string formatted_prompt =
      tokenizer_->ApplyChatTemplate("", messages, "", true);
  if (formatted_prompt.empty()) {
    LOG(ERROR) << "Failed to apply chat template";
    std::move(completion_callback).Run(false);
    return;
  }

  // Process images with formatted prompt
  auto input_tensors = processor->ProcessImages(formatted_prompt, images.get());
  if (!input_tensors) {
    LOG(ERROR) << "Failed to process images";
    std::move(completion_callback).Run(false);
    return;
  }

  // Create generator params
  auto params = std::make_unique<og::GeneratorParams>(*model_);
  if (!params->IsValid()) {
    LOG(ERROR) << "Failed to create generator params";
    std::move(completion_callback).Run(false);
    return;
  }

  // Set max_length
  if (!params->SetSearchNumber("max_length", static_cast<double>(max_tokens))) {
    LOG(ERROR) << "Failed to set max_length";
    std::move(completion_callback).Run(false);
    return;
  }

  // Create generator
  auto generator = std::make_unique<og::Generator>(*model_, *params);
  if (!generator->IsValid()) {
    LOG(ERROR) << "Failed to create generator";
    std::move(completion_callback).Run(false);
    return;
  }

  // Set input tensors
  if (!generator->SetInputs(*input_tensors)) {
    LOG(ERROR) << "Failed to set input tensors";
    std::move(completion_callback).Run(false);
    return;
  }

  // Generate tokens
  bool success = true;
  while (!generator->IsDone()) {
    if (!generator->GenerateNextToken()) {
      LOG(ERROR) << "Failed to generate next token";
      success = false;
      break;
    }

    const size_t num_tokens = generator->GetSequenceCount(0);
    if (num_tokens == 0) {
      continue;
    }

    const int32_t* sequence_data = generator->GetSequenceData(0);
    if (!sequence_data) {
      LOG(ERROR) << "Failed to get sequence data";
      success = false;
      break;
    }

    // SAFETY: sequence_data is guaranteed to have num_tokens elements by the
    // OnnxRuntime GenAI API. We've already checked num_tokens > 0 and
    // sequence_data != nullptr above.
    const int32_t new_token = UNSAFE_BUFFERS(sequence_data[num_tokens - 1]);
    std::string token_string = tokenizer_stream_->Decode(new_token);

    if (!token_string.empty()) {
      token_callback.Run(token_string);
    }
  }

  std::move(completion_callback).Run(success);
}

}  // namespace local_ai
