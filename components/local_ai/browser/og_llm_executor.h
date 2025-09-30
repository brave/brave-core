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
#include "brave/components/local_ai/og/scoped_og_types.h"

namespace local_ai {

// High-level executor for LLM inference using OnnxRuntime GenAI
class OgLlmExecutor {
 public:
  OgLlmExecutor();
  ~OgLlmExecutor();
  OgLlmExecutor(const OgLlmExecutor&) = delete;
  OgLlmExecutor& operator=(const OgLlmExecutor&) = delete;

  using TokenCallback = base::RepeatingCallback<void(const std::string& token)>;
  using CompletionCallback = base::OnceCallback<void(bool success)>;

  // Initialize the executor with library and model paths
  bool Initialize(const base::FilePath& library_path,
                  const base::FilePath& model_path);

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
