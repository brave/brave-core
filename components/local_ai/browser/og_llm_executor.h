// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_BROWSER_OG_LLM_EXECUTOR_H_
#define BRAVE_COMPONENTS_LOCAL_AI_BROWSER_OG_LLM_EXECUTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"

// Forward declarations of C API types (at global scope)
struct OgaModel;
struct OgaTokenizer;
struct OgaGenerator;
struct OgaGeneratorParams;
struct OgaSequences;
struct OgaTokenizerStream;
struct OgaResult;
struct OgaMultiModalProcessor;
struct OgaImages;
struct OgaNamedTensors;
struct OgaStringArray;

namespace local_ai {

// RAII wrapper for OnnxRuntime GenAI C API types
namespace og {

// RAII wrapper for OgaModel
class Model {
 public:
  Model();
  ~Model();
  Model(const Model&) = delete;
  Model& operator=(const Model&) = delete;
  Model(Model&&);
  Model& operator=(Model&&);

  // Load model from path
  bool Load(const base::FilePath& model_path);

  OgaModel* get() const { return model_; }
  bool IsValid() const { return model_ != nullptr; }

 private:
  RAW_PTR_EXCLUSION OgaModel* model_ = nullptr;
};

// RAII wrapper for OgaTokenizer
class Tokenizer {
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

  OgaTokenizer* get() const { return tokenizer_; }
  bool IsValid() const { return tokenizer_ != nullptr; }

 private:
  RAW_PTR_EXCLUSION OgaTokenizer* tokenizer_ = nullptr;
};

// RAII wrapper for OgaTokenizerStream
class TokenizerStream {
 public:
  explicit TokenizerStream(const Tokenizer& tokenizer);
  ~TokenizerStream();
  TokenizerStream(const TokenizerStream&) = delete;
  TokenizerStream& operator=(const TokenizerStream&) = delete;
  TokenizerStream(TokenizerStream&&);
  TokenizerStream& operator=(TokenizerStream&&);

  // Decode a single token
  std::string Decode(int32_t token);

  OgaTokenizerStream* get() const { return stream_; }
  bool IsValid() const { return stream_ != nullptr; }

 private:
  RAW_PTR_EXCLUSION OgaTokenizerStream* stream_ = nullptr;
};

// RAII wrapper for OgaSequences
class Sequences {
 public:
  Sequences();
  ~Sequences();
  Sequences(const Sequences&) = delete;
  Sequences& operator=(const Sequences&) = delete;
  Sequences(Sequences&&);
  Sequences& operator=(Sequences&&);

  OgaSequences* get() const { return sequences_; }
  bool IsValid() const { return sequences_ != nullptr; }

 private:
  RAW_PTR_EXCLUSION OgaSequences* sequences_ = nullptr;
};

// RAII wrapper for OgaGeneratorParams
class GeneratorParams {
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

  OgaGeneratorParams* get() const { return params_; }
  bool IsValid() const { return params_ != nullptr; }

 private:
  RAW_PTR_EXCLUSION OgaGeneratorParams* params_ = nullptr;
};

// Forward declarations of wrapper classes
class Images;
class NamedTensors;

// RAII wrapper for OgaImages
class Images {
 public:
  Images();
  ~Images();
  Images(const Images&) = delete;
  Images& operator=(const Images&) = delete;
  Images(Images&&);
  Images& operator=(Images&&);

  // Load images from file paths
  bool Load(const std::vector<base::FilePath>& image_paths);

  OgaImages* get() const { return images_; }
  bool IsValid() const { return images_ != nullptr; }

 private:
  RAW_PTR_EXCLUSION OgaImages* images_ = nullptr;
};

// RAII wrapper for OgaNamedTensors
class NamedTensors {
 public:
  explicit NamedTensors(OgaNamedTensors* tensors);
  ~NamedTensors();
  NamedTensors(const NamedTensors&) = delete;
  NamedTensors& operator=(const NamedTensors&) = delete;
  NamedTensors(NamedTensors&&);
  NamedTensors& operator=(NamedTensors&&);

  OgaNamedTensors* get() const { return tensors_; }
  bool IsValid() const { return tensors_ != nullptr; }

 private:
  RAW_PTR_EXCLUSION OgaNamedTensors* tensors_ = nullptr;
};

// RAII wrapper for OgaMultiModalProcessor
class MultiModalProcessor {
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

  OgaMultiModalProcessor* get() const { return processor_; }
  bool IsValid() const { return processor_ != nullptr; }

 private:
  RAW_PTR_EXCLUSION OgaMultiModalProcessor* processor_ = nullptr;
};

// RAII wrapper for OgaGenerator
class Generator {
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

  OgaGenerator* get() const { return generator_; }
  bool IsValid() const { return generator_ != nullptr; }

 private:
  RAW_PTR_EXCLUSION OgaGenerator* generator_ = nullptr;
};

// Helper to check and handle OgaResult errors
bool CheckResult(OgaResult* result);

}  // namespace og

// High-level executor for LLM inference using OnnxRuntime GenAI
class OgLlmExecutor {
 public:
  OgLlmExecutor();
  ~OgLlmExecutor();
  OgLlmExecutor(const OgLlmExecutor&) = delete;
  OgLlmExecutor& operator=(const OgLlmExecutor&) = delete;

  using TokenCallback = base::RepeatingCallback<void(const std::string& token)>;
  using CompletionCallback = base::OnceCallback<void(bool success)>;

  // Initialize the executor with a model
  bool Initialize(const base::FilePath& model_path);

  // Generate text from a prompt
  void Generate(const std::string& prompt,
                int max_tokens,
                TokenCallback token_callback,
                CompletionCallback completion_callback);

  // Generate text from a prompt with images (for vision models)
  void GenerateWithImage(const std::string& prompt,
                         const std::vector<base::FilePath>& image_paths,
                         int max_tokens,
                         TokenCallback token_callback,
                         CompletionCallback completion_callback);

  // Check if initialized
  bool IsInitialized() const { return model_ && model_->IsValid(); }

 private:
  // Encode the prompt to token sequences
  bool EncodePrompt(const std::string& prompt, og::Sequences& sequences);

  std::unique_ptr<og::Model> model_;
  std::unique_ptr<og::Tokenizer> tokenizer_;
  std::unique_ptr<og::TokenizerStream> tokenizer_stream_;

  base::WeakPtrFactory<OgLlmExecutor> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_BROWSER_OG_LLM_EXECUTOR_H_
