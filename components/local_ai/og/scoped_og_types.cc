// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/og/scoped_og_types.h"

#include "base/logging.h"
#include "brave/components/local_ai/og/og_status.h"
#include "ort_genai_c.h"

namespace local_ai::og {

// Helper to check and handle OgaResult errors (deprecated - use
// OGA_CALL_FAILED)
bool CheckResult(OgaResult* oga_result) {
  return !OGA_CALL_FAILED(oga_result);
}

// Model implementation
Model::Model() = default;
Model::~Model() = default;
Model::Model(Model&&) = default;
Model& Model::operator=(Model&&) = default;

bool Model::Load(const base::FilePath& model_path) {
  OgaModel* raw_model = nullptr;
  OgaResult* result =
      OGA_CALL(OgaCreateModel)(model_path.AsUTF8Unsafe().c_str(), &raw_model);

  if (!CheckResult(result)) {
    return false;
  }

  model_.reset(raw_model);
  return true;
}

// Tokenizer implementation
Tokenizer::Tokenizer(const Model& model) {
  if (!model.IsValid()) {
    return;
  }

  OgaTokenizer* raw_tokenizer = nullptr;
  OgaResult* result = OGA_CALL(OgaCreateTokenizer)(model.get(), &raw_tokenizer);
  if (CheckResult(result)) {
    tokenizer_.reset(raw_tokenizer);
  }
}

Tokenizer::~Tokenizer() = default;
Tokenizer::Tokenizer(Tokenizer&&) = default;
Tokenizer& Tokenizer::operator=(Tokenizer&&) = default;

std::string Tokenizer::ApplyChatTemplate(const std::string& template_str,
                                         const std::string& messages,
                                         const std::string& tools,
                                         bool add_generation_prompt) {
  if (!tokenizer_.is_valid()) {
    return "";
  }

  const char* out_string = nullptr;
  OgaResult* result = OGA_CALL(OgaTokenizerApplyChatTemplate)(
      tokenizer_.get(), template_str.empty() ? nullptr : template_str.c_str(),
      messages.c_str(), tools.empty() ? nullptr : tools.c_str(),
      add_generation_prompt, &out_string);

  if (!CheckResult(result) || !out_string) {
    return "";
  }

  std::string result_str(out_string);
  OGA_CALL(OgaDestroyString)(out_string);
  return result_str;
}

// TokenizerStream implementation
TokenizerStream::TokenizerStream(const Tokenizer& tokenizer) {
  if (!tokenizer.IsValid()) {
    return;
  }

  OgaTokenizerStream* raw_stream = nullptr;
  OgaResult* result =
      OGA_CALL(OgaCreateTokenizerStream)(tokenizer.get(), &raw_stream);
  if (CheckResult(result)) {
    stream_.reset(raw_stream);
  }
}

TokenizerStream::~TokenizerStream() = default;
TokenizerStream::TokenizerStream(TokenizerStream&&) = default;
TokenizerStream& TokenizerStream::operator=(TokenizerStream&&) = default;

std::string TokenizerStream::Decode(int32_t token) {
  if (!stream_.is_valid()) {
    return "";
  }

  const char* decoded = nullptr;
  OgaResult* result =
      OGA_CALL(OgaTokenizerStreamDecode)(stream_.get(), token, &decoded);
  if (!CheckResult(result) || !decoded) {
    return "";
  }

  return std::string(decoded);
}

// Sequences implementation
Sequences::Sequences() {
  OgaSequences* raw_sequences = nullptr;
  OgaResult* result = OGA_CALL(OgaCreateSequences)(&raw_sequences);
  if (CheckResult(result)) {
    sequences_.reset(raw_sequences);
  }
}

Sequences::~Sequences() = default;
Sequences::Sequences(Sequences&&) = default;
Sequences& Sequences::operator=(Sequences&&) = default;

// GeneratorParams implementation
GeneratorParams::GeneratorParams(const Model& model) {
  if (!model.IsValid()) {
    return;
  }

  OgaGeneratorParams* raw_params = nullptr;
  OgaResult* result =
      OGA_CALL(OgaCreateGeneratorParams)(model.get(), &raw_params);
  if (CheckResult(result)) {
    params_.reset(raw_params);
  }
}

GeneratorParams::~GeneratorParams() = default;
GeneratorParams::GeneratorParams(GeneratorParams&&) = default;
GeneratorParams& GeneratorParams::operator=(GeneratorParams&&) = default;

bool GeneratorParams::SetSearchNumber(const char* name, double value) {
  if (!params_.is_valid()) {
    return false;
  }

  OgaResult* result =
      OGA_CALL(OgaGeneratorParamsSetSearchNumber)(params_.get(), name, value);
  return CheckResult(result);
}

bool GeneratorParams::SetSearchBool(const char* name, bool value) {
  if (!params_.is_valid()) {
    return false;
  }

  OgaResult* result =
      OGA_CALL(OgaGeneratorParamsSetSearchBool)(params_.get(), name, value);
  return CheckResult(result);
}

// MultiModalProcessor implementation
MultiModalProcessor::MultiModalProcessor(const Model& model) {
  if (!model.IsValid()) {
    return;
  }

  OgaMultiModalProcessor* raw_processor = nullptr;
  OgaResult* result =
      OGA_CALL(OgaCreateMultiModalProcessor)(model.get(), &raw_processor);
  if (CheckResult(result)) {
    processor_.reset(raw_processor);
  }
}

MultiModalProcessor::~MultiModalProcessor() = default;
MultiModalProcessor::MultiModalProcessor(MultiModalProcessor&&) = default;
MultiModalProcessor& MultiModalProcessor::operator=(MultiModalProcessor&&) =
    default;

std::unique_ptr<NamedTensors> MultiModalProcessor::ProcessImages(
    const std::string& prompt,
    const Images* images) {
  if (!processor_.is_valid()) {
    return nullptr;
  }

  OgaImages* images_ptr = images ? images->get() : nullptr;
  OgaNamedTensors* raw_tensors = nullptr;
  OgaResult* result = OGA_CALL(OgaProcessorProcessImages)(
      processor_.get(), prompt.c_str(), images_ptr, &raw_tensors);

  if (!CheckResult(result) || !raw_tensors) {
    return nullptr;
  }

  return std::make_unique<NamedTensors>(raw_tensors);
}

// Images implementation
Images::Images() = default;
Images::~Images() = default;
Images::Images(Images&&) = default;
Images& Images::operator=(Images&&) = default;

bool Images::Load(const std::vector<base::FilePath>& image_paths) {
  if (image_paths.empty()) {
    return false;
  }

  // Create OgaStringArray
  OgaStringArray* string_array = nullptr;
  OgaResult* result = OGA_CALL(OgaCreateStringArray)(&string_array);
  if (!CheckResult(result) || !string_array) {
    return false;
  }

  // Add image paths to the string array
  for (const auto& image_path : image_paths) {
    result = OGA_CALL(OgaStringArrayAddString)(
        string_array, image_path.AsUTF8Unsafe().c_str());
    if (!CheckResult(result)) {
      OGA_CALL(OgaDestroyStringArray)(string_array);
      return false;
    }
  }

  // Load images
  OgaImages* raw_images = nullptr;
  result = OGA_CALL(OgaLoadImages)(string_array, &raw_images);
  OGA_CALL(OgaDestroyStringArray)(string_array);

  if (!CheckResult(result)) {
    return false;
  }

  images_.reset(raw_images);
  return true;
}

// NamedTensors implementation
NamedTensors::NamedTensors(OgaNamedTensors* tensors) : tensors_(tensors) {}
NamedTensors::~NamedTensors() = default;
NamedTensors::NamedTensors(NamedTensors&&) = default;
NamedTensors& NamedTensors::operator=(NamedTensors&&) = default;

// Generator implementation
Generator::Generator(const Model& model, const GeneratorParams& params) {
  if (!model.IsValid() || !params.IsValid()) {
    return;
  }

  OgaGenerator* raw_generator = nullptr;
  OgaResult* result =
      OGA_CALL(OgaCreateGenerator)(model.get(), params.get(), &raw_generator);
  if (CheckResult(result)) {
    generator_.reset(raw_generator);
  }
}

Generator::~Generator() = default;
Generator::Generator(Generator&&) = default;
Generator& Generator::operator=(Generator&&) = default;

bool Generator::GenerateNextToken() {
  if (!generator_.is_valid()) {
    return false;
  }

  OgaResult* result =
      OGA_CALL(OgaGenerator_GenerateNextToken)(generator_.get());
  return CheckResult(result);
}

bool Generator::IsDone() const {
  if (!generator_.is_valid()) {
    return true;
  }

  return OGA_CALL(OgaGenerator_IsDone)(generator_.get());
}

bool Generator::AppendTokenSequences(const Sequences& sequences) {
  if (!generator_.is_valid() || !sequences.IsValid()) {
    return false;
  }

  OgaResult* result = OGA_CALL(OgaGenerator_AppendTokenSequences)(
      generator_.get(), sequences.get());
  return CheckResult(result);
}

bool Generator::SetInputs(const NamedTensors& inputs) {
  if (!generator_.is_valid() || !inputs.IsValid()) {
    return false;
  }

  OgaResult* result =
      OGA_CALL(OgaGenerator_SetInputs)(generator_.get(), inputs.get());
  return CheckResult(result);
}

size_t Generator::GetSequenceCount(size_t index) const {
  if (!generator_.is_valid()) {
    return 0;
  }

  return OGA_CALL(OgaGenerator_GetSequenceCount)(generator_.get(), index);
}

const int32_t* Generator::GetSequenceData(size_t index) const {
  if (!generator_.is_valid()) {
    return nullptr;
  }

  return OGA_CALL(OgaGenerator_GetSequenceData)(generator_.get(), index);
}

}  // namespace local_ai::og
