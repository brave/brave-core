// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma allow_unsafe_buffers

#include "brave/components/local_ai/browser/mediapipe_llm_wrapper.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "brave/third_party/mediapipe/buildflags.h"

static_assert(BUILDFLAG(BRAVE_MEDIAPIPE_LLM_ENABLED),
              "Brave MediaPipe LLM must be enabled to build this wrapper");

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-const-int-float-conversion"
#include "third_party/mediapipe/src/mediapipe/tasks/cc/genai/inference/c/llm_inference_engine.h"
#pragma clang diagnostic pop

namespace local_ai {

class MediaPipeLLMWrapper::Impl {
 public:
  Impl() = default;
  ~Impl() {
    if (session_) {
      LlmInferenceEngine_Session_Delete(session_);
    }
    if (engine_) {
      LlmInferenceEngine_Engine_Delete(engine_);
    }
  }

  bool Initialize(const ModelSettings& settings, std::string* error) {
    LlmModelSettings model_settings = {};
    model_settings.model_path = settings.model_path.c_str();
    model_settings.cache_dir = settings.cache_dir.c_str();
    model_settings.max_num_tokens = settings.max_num_tokens;

    char* error_msg = nullptr;
    LlmInferenceEngine_Engine* engine_ptr = nullptr;
    int result = LlmInferenceEngine_CreateEngine(&model_settings, &engine_ptr,
                                                 &error_msg);
    engine_ = engine_ptr;
    if (result != 0) {
      if (error && error_msg) {
        *error = std::string(error_msg);
        free(error_msg);
      }
      return false;
    }

    return true;
  }

  bool CreateSession(const SessionConfig& config, std::string* error) {
    if (!engine_) {
      if (error) {
        *error = "Engine not initialized";
      }
      return false;
    }

    LlmSessionConfig session_config = {};
    session_config.topk = config.topk;
    session_config.topp = config.topp;
    session_config.temperature = config.temperature;
    session_config.random_seed = config.random_seed;

    char* error_msg = nullptr;
    LlmInferenceEngine_Session* session_ptr = nullptr;
    int result = LlmInferenceEngine_CreateSession(engine_, &session_config,
                                                  &session_ptr, &error_msg);
    session_ = session_ptr;
    if (result != 0) {
      if (error && error_msg) {
        *error = std::string(error_msg);
        free(error_msg);
      }
      return false;
    }

    return true;
  }

  bool AddQueryChunk(const std::string& input, std::string* error) {
    if (!session_) {
      if (error) {
        *error = "Session not created";
      }
      return false;
    }

    char* error_msg = nullptr;
    int result = LlmInferenceEngine_Session_AddQueryChunk(
        session_, input.c_str(), &error_msg);
    if (result != 0) {
      if (error && error_msg) {
        *error = std::string(error_msg);
        free(error_msg);
      }
      return false;
    }

    return true;
  }

  bool PredictSync(MediaPipeLLMWrapper::Response* response,
                   std::string* error) {
    if (!session_) {
      if (error) {
        *error = "Session not created";
      }
      return false;
    }

    LlmResponseContext response_context = {};
    char* error_msg = nullptr;
    int result = LlmInferenceEngine_Session_PredictSync(
        session_, &response_context, &error_msg);
    if (result != 0) {
      if (error && error_msg) {
        *error = std::string(error_msg);
        free(error_msg);
      }
      return false;
    }

    // Convert C response to C++ response
    response->responses.clear();
    for (int i = 0; i < response_context.response_count; ++i) {
      char* response_str = response_context.response_array[i];
      if (response_str) {
        response->responses.push_back(std::string(response_str));
      }
    }
    response->done = response_context.done;

    LlmInferenceEngine_CloseResponseContext(&response_context);
    return true;
  }

  bool PredictAsync(MediaPipeLLMWrapper::ResponseCallback response_callback,
                    MediaPipeLLMWrapper::ErrorCallback error_callback) {
    if (!session_) {
      error_callback.Run("Session not created");
      return false;
    }

    // Store callbacks for use in C callback
    response_callback_ = std::move(response_callback);
    error_callback_ = std::move(error_callback);

    char* error_msg = nullptr;
    int result = LlmInferenceEngine_Session_PredictAsync(
        session_, this, &error_msg, &AsyncCallbackTrampoline);

    if (result != 0) {
      if (error_msg) {
        error_callback_.Run(std::string(error_msg));
        free(error_msg);
      }
      return false;
    }

    return true;
  }

  int GetTokenCount(const std::string& input, std::string* error) {
    if (!session_) {
      if (error) {
        *error = "Session not created";
      }
      return -1;
    }

    char* error_msg = nullptr;
    int result = LlmInferenceEngine_Session_SizeInTokens(
        session_, input.c_str(), &error_msg);
    if (result < 0) {
      if (error && error_msg) {
        *error = std::string(error_msg);
        free(error_msg);
      }
    }

    return result;
  }

  bool CancelPending(std::string* error) {
    if (!session_) {
      if (error) {
        *error = "Session not created";
      }
      return false;
    }

    char* error_msg = nullptr;
    int result = LlmInferenceEngine_Session_PendingProcessCancellation(
        session_, &error_msg);
    if (result != 0) {
      if (error && error_msg) {
        *error = std::string(error_msg);
        free(error_msg);
      }
      return false;
    }

    return true;
  }

  bool IsReady() const { return engine_ != nullptr && session_ != nullptr; }

 private:
  static void AsyncCallbackTrampoline(void* callback_context,
                                      LlmResponseContext* response_context) {
    Impl* impl = static_cast<Impl*>(callback_context);
    impl->OnAsyncResponse(response_context);
  }

  void OnAsyncResponse(LlmResponseContext* response_context) {
    MediaPipeLLMWrapper::Response response;

    for (int i = 0; i < response_context->response_count; ++i) {
      char* response_str = response_context->response_array[i];
      if (response_str) {
        response.responses.push_back(std::string(response_str));
      }
    }
    response.done = response_context->done;

    response_callback_.Run(response);
    LlmInferenceEngine_CloseResponseContext(response_context);
  }

  raw_ptr<LlmInferenceEngine_Engine> engine_ = nullptr;
  raw_ptr<LlmInferenceEngine_Session> session_ = nullptr;
  MediaPipeLLMWrapper::ResponseCallback response_callback_;
  MediaPipeLLMWrapper::ErrorCallback error_callback_;
};

MediaPipeLLMWrapper::MediaPipeLLMWrapper() : impl_(std::make_unique<Impl>()) {}

MediaPipeLLMWrapper::~MediaPipeLLMWrapper() = default;

bool MediaPipeLLMWrapper::Initialize(const ModelSettings& settings,
                                     std::string* error) {
  return impl_->Initialize(settings, error);
}

bool MediaPipeLLMWrapper::CreateSession(const SessionConfig& config,
                                        std::string* error) {
  return impl_->CreateSession(config, error);
}

bool MediaPipeLLMWrapper::AddQueryChunk(const std::string& input,
                                        std::string* error) {
  return impl_->AddQueryChunk(input, error);
}

bool MediaPipeLLMWrapper::PredictSync(Response* response, std::string* error) {
  return impl_->PredictSync(response, error);
}

bool MediaPipeLLMWrapper::PredictAsync(ResponseCallback response_callback,
                                       ErrorCallback error_callback) {
  return impl_->PredictAsync(std::move(response_callback),
                             std::move(error_callback));
}

int MediaPipeLLMWrapper::GetTokenCount(const std::string& input,
                                       std::string* error) {
  return impl_->GetTokenCount(input, error);
}

bool MediaPipeLLMWrapper::CancelPending(std::string* error) {
  return impl_->CancelPending(error);
}

bool MediaPipeLLMWrapper::IsReady() const {
  return impl_->IsReady();
}

// Response struct implementations
MediaPipeLLMWrapper::Response::Response() = default;
MediaPipeLLMWrapper::Response::~Response() = default;
MediaPipeLLMWrapper::Response::Response(const Response&) = default;
MediaPipeLLMWrapper::Response& MediaPipeLLMWrapper::Response::operator=(
    const Response&) = default;
MediaPipeLLMWrapper::Response::Response(Response&&) = default;
MediaPipeLLMWrapper::Response& MediaPipeLLMWrapper::Response::operator=(
    Response&&) = default;

}  // namespace local_ai
