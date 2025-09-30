// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/og_llm_executor.h"

#include "base/logging.h"
#include "base/notreached.h"
#include "ort_genai_c.h"

namespace local_ai {

namespace og {

// Helper to check and handle OgaResult errors
bool CheckResult(OgaResult* result) {
  if (result != nullptr) {
    const char* error = OgaResultGetError(result);
    LOG(ERROR) << "OnnxRuntime GenAI error: " << error;
    OgaDestroyResult(result);
    return false;
  }
  return true;
}

// Model implementation
Model::Model() = default;

Model::~Model() {
  if (model_) {
    OgaDestroyModel(model_);
    model_ = nullptr;
  }
}

Model::Model(Model&& other) : model_(other.model_) {
  other.model_ = nullptr;
}

Model& Model::operator=(Model&& other) {
  if (this != &other) {
    if (model_) {
      OgaDestroyModel(model_);
    }
    model_ = other.model_;
    other.model_ = nullptr;
  }
  return *this;
}

bool Model::Load(const base::FilePath& model_path) {
  if (model_) {
    OgaDestroyModel(model_);
    model_ = nullptr;
  }

  OgaResult* result =
      OgaCreateModel(model_path.AsUTF8Unsafe().c_str(), &model_);
  return CheckResult(result);
}

// Tokenizer implementation
Tokenizer::Tokenizer(const Model& model) {
  if (!model.IsValid()) {
    return;
  }

  OgaResult* result = OgaCreateTokenizer(model.get(), &tokenizer_);
  if (!CheckResult(result)) {
    tokenizer_ = nullptr;
  }
}

Tokenizer::~Tokenizer() {
  if (tokenizer_) {
    OgaDestroyTokenizer(tokenizer_);
    tokenizer_ = nullptr;
  }
}

Tokenizer::Tokenizer(Tokenizer&& other) : tokenizer_(other.tokenizer_) {
  other.tokenizer_ = nullptr;
}

Tokenizer& Tokenizer::operator=(Tokenizer&& other) {
  if (this != &other) {
    if (tokenizer_) {
      OgaDestroyTokenizer(tokenizer_);
    }
    tokenizer_ = other.tokenizer_;
    other.tokenizer_ = nullptr;
  }
  return *this;
}

std::string Tokenizer::ApplyChatTemplate(const std::string& template_str,
                                         const std::string& messages,
                                         const std::string& tools,
                                         bool add_generation_prompt) {
  if (!tokenizer_) {
    return "";
  }

  const char* out_string = nullptr;
  OgaResult* result = OgaTokenizerApplyChatTemplate(
      tokenizer_, template_str.empty() ? nullptr : template_str.c_str(),
      messages.c_str(), tools.empty() ? nullptr : tools.c_str(),
      add_generation_prompt, &out_string);

  if (!CheckResult(result) || !out_string) {
    return "";
  }

  std::string result_str(out_string);
  OgaDestroyString(out_string);
  return result_str;
}

// TokenizerStream implementation
TokenizerStream::TokenizerStream(const Tokenizer& tokenizer) {
  if (!tokenizer.IsValid()) {
    return;
  }

  OgaResult* result = OgaCreateTokenizerStream(tokenizer.get(), &stream_);
  if (!CheckResult(result)) {
    stream_ = nullptr;
  }
}

TokenizerStream::~TokenizerStream() {
  if (stream_) {
    OgaDestroyTokenizerStream(stream_);
    stream_ = nullptr;
  }
}

TokenizerStream::TokenizerStream(TokenizerStream&& other)
    : stream_(other.stream_) {
  other.stream_ = nullptr;
}

TokenizerStream& TokenizerStream::operator=(TokenizerStream&& other) {
  if (this != &other) {
    if (stream_) {
      OgaDestroyTokenizerStream(stream_);
    }
    stream_ = other.stream_;
    other.stream_ = nullptr;
  }
  return *this;
}

std::string TokenizerStream::Decode(int32_t token) {
  if (!stream_) {
    return "";
  }

  const char* decoded = nullptr;
  OgaResult* result = OgaTokenizerStreamDecode(stream_, token, &decoded);
  if (!CheckResult(result) || !decoded) {
    return "";
  }

  return std::string(decoded);
}

// Sequences implementation
Sequences::Sequences() {
  OgaResult* result = OgaCreateSequences(&sequences_);
  if (!CheckResult(result)) {
    sequences_ = nullptr;
  }
}

Sequences::~Sequences() {
  if (sequences_) {
    OgaDestroySequences(sequences_);
    sequences_ = nullptr;
  }
}

Sequences::Sequences(Sequences&& other) : sequences_(other.sequences_) {
  other.sequences_ = nullptr;
}

Sequences& Sequences::operator=(Sequences&& other) {
  if (this != &other) {
    if (sequences_) {
      OgaDestroySequences(sequences_);
    }
    sequences_ = other.sequences_;
    other.sequences_ = nullptr;
  }
  return *this;
}

// GeneratorParams implementation
GeneratorParams::GeneratorParams(const Model& model) {
  if (!model.IsValid()) {
    return;
  }

  OgaResult* result = OgaCreateGeneratorParams(model.get(), &params_);
  if (!CheckResult(result)) {
    params_ = nullptr;
  }
}

GeneratorParams::~GeneratorParams() {
  if (params_) {
    OgaDestroyGeneratorParams(params_);
    params_ = nullptr;
  }
}

GeneratorParams::GeneratorParams(GeneratorParams&& other)
    : params_(other.params_) {
  other.params_ = nullptr;
}

GeneratorParams& GeneratorParams::operator=(GeneratorParams&& other) {
  if (this != &other) {
    if (params_) {
      OgaDestroyGeneratorParams(params_);
    }
    params_ = other.params_;
    other.params_ = nullptr;
  }
  return *this;
}

bool GeneratorParams::SetSearchNumber(const char* name, double value) {
  if (!params_) {
    return false;
  }

  OgaResult* result = OgaGeneratorParamsSetSearchNumber(params_, name, value);
  return CheckResult(result);
}

bool GeneratorParams::SetSearchBool(const char* name, bool value) {
  if (!params_) {
    return false;
  }

  OgaResult* result = OgaGeneratorParamsSetSearchBool(params_, name, value);
  return CheckResult(result);
}

// MultiModalProcessor implementation
MultiModalProcessor::MultiModalProcessor(const Model& model) {
  if (!model.IsValid()) {
    return;
  }

  OgaResult* result = OgaCreateMultiModalProcessor(model.get(), &processor_);
  if (!CheckResult(result)) {
    processor_ = nullptr;
  }
}

MultiModalProcessor::~MultiModalProcessor() {
  if (processor_) {
    OgaDestroyMultiModalProcessor(processor_);
    processor_ = nullptr;
  }
}

MultiModalProcessor::MultiModalProcessor(MultiModalProcessor&& other)
    : processor_(other.processor_) {
  other.processor_ = nullptr;
}

MultiModalProcessor& MultiModalProcessor::operator=(
    MultiModalProcessor&& other) {
  if (this != &other) {
    if (processor_) {
      OgaDestroyMultiModalProcessor(processor_);
    }
    processor_ = other.processor_;
    other.processor_ = nullptr;
  }
  return *this;
}

std::unique_ptr<NamedTensors> MultiModalProcessor::ProcessImages(
    const std::string& prompt,
    const Images* images) {
  if (!processor_) {
    return nullptr;
  }

  OgaImages* images_ptr = images ? images->get() : nullptr;
  OgaNamedTensors* raw_tensors = nullptr;
  OgaResult* result = OgaProcessorProcessImages(processor_, prompt.c_str(),
                                                images_ptr, &raw_tensors);
  if (!CheckResult(result) || !raw_tensors) {
    return nullptr;
  }

  return std::make_unique<NamedTensors>(raw_tensors);
}

// Images implementation
Images::Images() = default;

Images::~Images() {
  if (images_) {
    OgaDestroyImages(images_);
    images_ = nullptr;
  }
}

Images::Images(Images&& other) : images_(other.images_) {
  other.images_ = nullptr;
}

Images& Images::operator=(Images&& other) {
  if (this != &other) {
    if (images_) {
      OgaDestroyImages(images_);
    }
    images_ = other.images_;
    other.images_ = nullptr;
  }
  return *this;
}

bool Images::Load(const std::vector<base::FilePath>& image_paths) {
  if (images_) {
    OgaDestroyImages(images_);
    images_ = nullptr;
  }

  if (image_paths.empty()) {
    return false;
  }

  // Create OgaStringArray
  OgaStringArray* string_array = nullptr;
  OgaResult* result = OgaCreateStringArray(&string_array);
  if (!CheckResult(result) || !string_array) {
    return false;
  }

  // Add image paths to the string array
  for (const auto& image_path : image_paths) {
    result = OgaStringArrayAddString(string_array,
                                     image_path.AsUTF8Unsafe().c_str());
    if (!CheckResult(result)) {
      OgaDestroyStringArray(string_array);
      return false;
    }
  }

  // Load images
  result = OgaLoadImages(string_array, &images_);
  OgaDestroyStringArray(string_array);
  return CheckResult(result);
}

// NamedTensors implementation
NamedTensors::NamedTensors(OgaNamedTensors* tensors) : tensors_(tensors) {}

NamedTensors::~NamedTensors() {
  if (tensors_) {
    OgaDestroyNamedTensors(tensors_);
    tensors_ = nullptr;
  }
}

NamedTensors::NamedTensors(NamedTensors&& other) : tensors_(other.tensors_) {
  other.tensors_ = nullptr;
}

NamedTensors& NamedTensors::operator=(NamedTensors&& other) {
  if (this != &other) {
    if (tensors_) {
      OgaDestroyNamedTensors(tensors_);
    }
    tensors_ = other.tensors_;
    other.tensors_ = nullptr;
  }
  return *this;
}

// Generator implementation
Generator::Generator(const Model& model, const GeneratorParams& params) {
  if (!model.IsValid() || !params.IsValid()) {
    return;
  }

  OgaResult* result =
      OgaCreateGenerator(model.get(), params.get(), &generator_);
  if (!CheckResult(result)) {
    generator_ = nullptr;
  }
}

Generator::~Generator() {
  if (generator_) {
    OgaDestroyGenerator(generator_);
    generator_ = nullptr;
  }
}

Generator::Generator(Generator&& other) : generator_(other.generator_) {
  other.generator_ = nullptr;
}

Generator& Generator::operator=(Generator&& other) {
  if (this != &other) {
    if (generator_) {
      OgaDestroyGenerator(generator_);
    }
    generator_ = other.generator_;
    other.generator_ = nullptr;
  }
  return *this;
}

bool Generator::GenerateNextToken() {
  if (!generator_) {
    return false;
  }

  OgaResult* result = OgaGenerator_GenerateNextToken(generator_);
  return CheckResult(result);
}

bool Generator::IsDone() const {
  if (!generator_) {
    return true;
  }

  return OgaGenerator_IsDone(generator_);
}

bool Generator::AppendTokenSequences(const Sequences& sequences) {
  if (!generator_ || !sequences.IsValid()) {
    return false;
  }

  OgaResult* result =
      OgaGenerator_AppendTokenSequences(generator_, sequences.get());
  return CheckResult(result);
}

bool Generator::SetInputs(const NamedTensors& inputs) {
  if (!generator_ || !inputs.IsValid()) {
    return false;
  }

  OgaResult* result = OgaGenerator_SetInputs(generator_, inputs.get());
  return CheckResult(result);
}

size_t Generator::GetSequenceCount(size_t index) const {
  if (!generator_) {
    return 0;
  }

  return OgaGenerator_GetSequenceCount(generator_, index);
}

const int32_t* Generator::GetSequenceData(size_t index) const {
  if (!generator_) {
    return nullptr;
  }

  return OgaGenerator_GetSequenceData(generator_, index);
}

}  // namespace og

// OgLlmExecutor implementation
OgLlmExecutor::OgLlmExecutor() = default;
OgLlmExecutor::~OgLlmExecutor() = default;

bool OgLlmExecutor::Initialize(const base::FilePath& model_path) {
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

  OgaResult* result =
      OgaTokenizerEncode(tokenizer_->get(), prompt.c_str(), sequences.get());
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
      OgaSequencesGetSequenceCount(sequences->get(), 0);

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
