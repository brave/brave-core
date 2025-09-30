// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_OG_SCOPED_OG_TYPES_H_
#define BRAVE_COMPONENTS_LOCAL_AI_OG_SCOPED_OG_TYPES_H_

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "base/scoped_generic.h"
#include "brave/components/local_ai/og/platform_functions_og.h"
#include "ort_genai_c.h"

namespace local_ai::og {

namespace internal {

template <typename T>
  requires std::is_pointer<T>::value
struct ScopedOgTypeTraitsHelper;

template <typename T>
  requires std::is_pointer<T>::value
struct ScopedOgTypeTraits {
  static T InvalidValue() { return nullptr; }
  static void Free(T value) { ScopedOgTypeTraitsHelper<T>::Free(value); }
};

template <>
struct ScopedOgTypeTraitsHelper<OgaModel*> {
  static void Free(OgaModel* value) {
    if (auto* platform = PlatformFunctions::GetInstance()) {
      platform->OgaDestroyModelPtr(value);
    }
  }
};

template <>
struct ScopedOgTypeTraitsHelper<OgaTokenizer*> {
  static void Free(OgaTokenizer* value) {
    if (auto* platform = PlatformFunctions::GetInstance()) {
      platform->OgaDestroyTokenizerPtr(value);
    }
  }
};

template <>
struct ScopedOgTypeTraitsHelper<OgaTokenizerStream*> {
  static void Free(OgaTokenizerStream* value) {
    if (auto* platform = PlatformFunctions::GetInstance()) {
      platform->OgaDestroyTokenizerStreamPtr(value);
    }
  }
};

template <>
struct ScopedOgTypeTraitsHelper<OgaSequences*> {
  static void Free(OgaSequences* value) {
    if (auto* platform = PlatformFunctions::GetInstance()) {
      platform->OgaDestroySequencesPtr(value);
    }
  }
};

template <>
struct ScopedOgTypeTraitsHelper<OgaGeneratorParams*> {
  static void Free(OgaGeneratorParams* value) {
    if (auto* platform = PlatformFunctions::GetInstance()) {
      platform->OgaDestroyGeneratorParamsPtr(value);
    }
  }
};

template <>
struct ScopedOgTypeTraitsHelper<OgaMultiModalProcessor*> {
  static void Free(OgaMultiModalProcessor* value) {
    if (auto* platform = PlatformFunctions::GetInstance()) {
      platform->OgaDestroyMultiModalProcessorPtr(value);
    }
  }
};

template <>
struct ScopedOgTypeTraitsHelper<OgaImages*> {
  static void Free(OgaImages* value) {
    if (auto* platform = PlatformFunctions::GetInstance()) {
      platform->OgaDestroyImagesPtr(value);
    }
  }
};

template <>
struct ScopedOgTypeTraitsHelper<OgaNamedTensors*> {
  static void Free(OgaNamedTensors* value) {
    if (auto* platform = PlatformFunctions::GetInstance()) {
      platform->OgaDestroyNamedTensorsPtr(value);
    }
  }
};

template <>
struct ScopedOgTypeTraitsHelper<OgaGenerator*> {
  static void Free(OgaGenerator* value) {
    if (auto* platform = PlatformFunctions::GetInstance()) {
      platform->OgaDestroyGeneratorPtr(value);
    }
  }
};

template <typename T>
using ScopedOgType = base::ScopedGeneric<T*, ScopedOgTypeTraits<T*>>;

}  // namespace internal

using ScopedOgModel = internal::ScopedOgType<OgaModel>;
using ScopedOgTokenizer = internal::ScopedOgType<OgaTokenizer>;
using ScopedOgTokenizerStream = internal::ScopedOgType<OgaTokenizerStream>;
using ScopedOgSequences = internal::ScopedOgType<OgaSequences>;
using ScopedOgGeneratorParams = internal::ScopedOgType<OgaGeneratorParams>;
using ScopedOgMultiModalProcessor =
    internal::ScopedOgType<OgaMultiModalProcessor>;
using ScopedOgImages = internal::ScopedOgType<OgaImages>;
using ScopedOgNamedTensors = internal::ScopedOgType<OgaNamedTensors>;
using ScopedOgGenerator = internal::ScopedOgType<OgaGenerator>;

// Forward declarations of wrapper classes
class Images;
class NamedTensors;

// RAII wrapper for OgaModel
class COMPONENT_EXPORT(LOCAL_AI) Model {
 public:
  Model();
  ~Model();
  Model(const Model&) = delete;
  Model& operator=(const Model&) = delete;
  Model(Model&&);
  Model& operator=(Model&&);

  // Load model from path
  bool Load(const base::FilePath& model_path);

  OgaModel* get() const { return model_.get(); }
  bool IsValid() const { return model_.is_valid(); }

 private:
  ScopedOgModel model_;
};

// RAII wrapper for OgaTokenizer
class COMPONENT_EXPORT(LOCAL_AI) Tokenizer {
 public:
  explicit Tokenizer(const Model& model);
  ~Tokenizer();
  Tokenizer(const Tokenizer&) = delete;
  Tokenizer& operator=(const Tokenizer&) = delete;
  Tokenizer(Tokenizer&&);
  Tokenizer& operator=(Tokenizer&&);

  // Apply chat template to messages
  std::string ApplyChatTemplate(const std::string& template_str,
                                const std::string& messages,
                                const std::string& tools,
                                bool add_generation_prompt);

  OgaTokenizer* get() const { return tokenizer_.get(); }
  bool IsValid() const { return tokenizer_.is_valid(); }

 private:
  ScopedOgTokenizer tokenizer_;
};

// RAII wrapper for OgaTokenizerStream
class COMPONENT_EXPORT(LOCAL_AI) TokenizerStream {
 public:
  explicit TokenizerStream(const Tokenizer& tokenizer);
  ~TokenizerStream();
  TokenizerStream(const TokenizerStream&) = delete;
  TokenizerStream& operator=(const TokenizerStream&) = delete;
  TokenizerStream(TokenizerStream&&);
  TokenizerStream& operator=(TokenizerStream&&);

  // Decode a single token
  std::string Decode(int32_t token);

  OgaTokenizerStream* get() const { return stream_.get(); }
  bool IsValid() const { return stream_.is_valid(); }

 private:
  ScopedOgTokenizerStream stream_;
};

// RAII wrapper for OgaSequences
class COMPONENT_EXPORT(LOCAL_AI) Sequences {
 public:
  Sequences();
  ~Sequences();
  Sequences(const Sequences&) = delete;
  Sequences& operator=(const Sequences&) = delete;
  Sequences(Sequences&&);
  Sequences& operator=(Sequences&&);

  OgaSequences* get() const { return sequences_.get(); }
  bool IsValid() const { return sequences_.is_valid(); }

 private:
  ScopedOgSequences sequences_;
};

// RAII wrapper for OgaGeneratorParams
class COMPONENT_EXPORT(LOCAL_AI) GeneratorParams {
 public:
  explicit GeneratorParams(const Model& model);
  ~GeneratorParams();
  GeneratorParams(const GeneratorParams&) = delete;
  GeneratorParams& operator=(const GeneratorParams&) = delete;
  GeneratorParams(GeneratorParams&&);
  GeneratorParams& operator=(GeneratorParams&&);

  // Set search options
  bool SetSearchNumber(const char* name, double value);
  bool SetSearchBool(const char* name, bool value);

  OgaGeneratorParams* get() const { return params_.get(); }
  bool IsValid() const { return params_.is_valid(); }

 private:
  ScopedOgGeneratorParams params_;
};

// RAII wrapper for OgaImages
class COMPONENT_EXPORT(LOCAL_AI) Images {
 public:
  Images();
  ~Images();
  Images(const Images&) = delete;
  Images& operator=(const Images&) = delete;
  Images(Images&&);
  Images& operator=(Images&&);

  // Load images from file paths
  bool Load(const std::vector<base::FilePath>& image_paths);

  OgaImages* get() const { return images_.get(); }
  bool IsValid() const { return images_.is_valid(); }

 private:
  ScopedOgImages images_;
};

// RAII wrapper for OgaNamedTensors
class COMPONENT_EXPORT(LOCAL_AI) NamedTensors {
 public:
  explicit NamedTensors(OgaNamedTensors* tensors);
  ~NamedTensors();
  NamedTensors(const NamedTensors&) = delete;
  NamedTensors& operator=(const NamedTensors&) = delete;
  NamedTensors(NamedTensors&&);
  NamedTensors& operator=(NamedTensors&&);

  OgaNamedTensors* get() const { return tensors_.get(); }
  bool IsValid() const { return tensors_.is_valid(); }

 private:
  ScopedOgNamedTensors tensors_;
};

// RAII wrapper for OgaMultiModalProcessor
class COMPONENT_EXPORT(LOCAL_AI) MultiModalProcessor {
 public:
  explicit MultiModalProcessor(const Model& model);
  ~MultiModalProcessor();
  MultiModalProcessor(const MultiModalProcessor&) = delete;
  MultiModalProcessor& operator=(const MultiModalProcessor&) = delete;
  MultiModalProcessor(MultiModalProcessor&&);
  MultiModalProcessor& operator=(MultiModalProcessor&&);

  // Process images with prompt and return named tensors
  std::unique_ptr<NamedTensors> ProcessImages(const std::string& prompt,
                                              const Images* images);

  OgaMultiModalProcessor* get() const { return processor_.get(); }
  bool IsValid() const { return processor_.is_valid(); }

 private:
  ScopedOgMultiModalProcessor processor_;
};

// RAII wrapper for OgaGenerator
class COMPONENT_EXPORT(LOCAL_AI) Generator {
 public:
  Generator(const Model& model, const GeneratorParams& params);
  ~Generator();
  Generator(const Generator&) = delete;
  Generator& operator=(const Generator&) = delete;
  Generator(Generator&&);
  Generator& operator=(Generator&&);

  // Generation methods
  bool GenerateNextToken();
  bool IsDone() const;

  // Append token sequences
  bool AppendTokenSequences(const Sequences& sequences);

  // Set input tensors (for vision models)
  bool SetInputs(const NamedTensors& inputs);

  // Get sequence data
  size_t GetSequenceCount(size_t index) const;
  const int32_t* GetSequenceData(size_t index) const;

  OgaGenerator* get() const { return generator_.get(); }
  bool IsValid() const { return generator_.is_valid(); }

 private:
  ScopedOgGenerator generator_;
};

// Helper to check and handle OgaResult errors
bool CheckResult(OgaResult* result);

}  // namespace local_ai::og

#endif  // BRAVE_COMPONENTS_LOCAL_AI_OG_SCOPED_OG_TYPES_H_
