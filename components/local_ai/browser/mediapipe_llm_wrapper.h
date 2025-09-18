// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_BROWSER_MEDIAPIPE_LLM_WRAPPER_H_
#define BRAVE_COMPONENTS_LOCAL_AI_BROWSER_MEDIAPIPE_LLM_WRAPPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "brave/third_party/mediapipe/buildflags.h"

namespace local_ai {

// Wrapper around MediaPipe's LLM inference engine to provide a C++ interface
// that integrates with Brave's local AI infrastructure.
class MediaPipeLLMWrapper {
 public:
  struct ModelSettings {
    std::string model_path;
    std::string cache_dir;
    size_t max_num_tokens = 512;
    size_t max_num_images = 0;
    bool use_gpu = false;
  };

  struct SessionConfig {
    size_t topk = 1;
    float topp = 1.0f;
    float temperature = 0.0f;
    size_t random_seed = 0;
    std::string lora_path;
  };

  struct Response {
    Response();
    ~Response();
    Response(const Response&);
    Response& operator=(const Response&);
    Response(Response&&);
    Response& operator=(Response&&);

    std::vector<std::string> responses;
    bool done = false;
  };

  using ResponseCallback = base::RepeatingCallback<void(Response)>;
  using ErrorCallback = base::RepeatingCallback<void(const std::string&)>;

  MediaPipeLLMWrapper();
  ~MediaPipeLLMWrapper();

  // Not copyable or movable
  MediaPipeLLMWrapper(const MediaPipeLLMWrapper&) = delete;
  MediaPipeLLMWrapper& operator=(const MediaPipeLLMWrapper&) = delete;

  // Initialize the engine with the given model settings
  bool Initialize(const ModelSettings& settings, std::string* error = nullptr);

  // Create a new session with the given configuration
  bool CreateSession(const SessionConfig& config, std::string* error = nullptr);

  // Add a query chunk to the current session
  bool AddQueryChunk(const std::string& input, std::string* error = nullptr);

  // Generate response synchronously
  bool PredictSync(Response* response, std::string* error = nullptr);

  // Generate response asynchronously with callback
  bool PredictAsync(ResponseCallback response_callback,
                    ErrorCallback error_callback);

  // Get token count for input text
  int GetTokenCount(const std::string& input, std::string* error = nullptr);

  // Cancel pending operations
  bool CancelPending(std::string* error = nullptr);

  // Check if the wrapper is initialized and ready
  bool IsReady() const;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_BROWSER_MEDIAPIPE_LLM_WRAPPER_H_
