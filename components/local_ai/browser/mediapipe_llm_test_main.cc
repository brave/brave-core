// Copyright 2024 The MediaPipe Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// BRAVE: Modified version of upstream llm_inference_engine_cpu_main.cc
// Replaced glog with base/logging.h for Chromium compatibility

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/string_view.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop/message_pump_type.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "mediapipe/framework/deps/file_path.h"
#include "mediapipe/tasks/cc/genai/inference/c/llm_inference_engine.h"

ABSL_FLAG(std::optional<std::string>, model_path, std::nullopt,
          "Path to the tflite model file.");

ABSL_FLAG(std::optional<std::string>, cache_dir, std::nullopt,
          "Path to the cache directory.");

// Maximum number of sequence length for input + output.
ABSL_FLAG(int, max_tokens, 512,
          "Maximum number of input and output tokens. This value needs to be "
          "at least larger than the number of input tokens.");

ABSL_FLAG(std::optional<uint32_t>, topk, std::nullopt,
          "Number of tokens to sample from at each decoding step for top-k "
          "sampling.");

ABSL_FLAG(
    std::optional<float>, temperature, std::nullopt,
    "Softmax temperature. For any value less than 1/1024 (the difference "
    "between 1.0 and the next representable value for half-precision floats), "
    "the sampling op collapses to an ArgMax.");

ABSL_FLAG(std::optional<uint32_t>, random_seed, std::nullopt,
          "Random seed for sampling tokens.");

ABSL_FLAG(
    std::optional<std::string>, prompt, std::nullopt,
    "The input prompt to be fed to the model. The flag is not relevant when "
    "running the benchmark, i.e. the input_token_limit value is set.");


int main(int argc, char** argv) {
  // BRAVE: Initialize base infrastructure for Chromium compatibility
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_STDERR;
  logging::InitLogging(settings);
  base::SingleThreadTaskExecutor io_task_executor(base::MessagePumpType::IO);
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("MediaPipeLLMTest");

  absl::ParseCommandLine(argc, argv);

  ABSL_QCHECK(absl::GetFlag(FLAGS_model_path).has_value())
      << "--model_path is required.";
  const std::string model_path = absl::GetFlag(FLAGS_model_path).value();
  std::string cache_dir;
  if (absl::GetFlag(FLAGS_cache_dir).has_value()) {
    cache_dir = absl::GetFlag(FLAGS_cache_dir).value();
  } else {
    cache_dir = std::string(mediapipe::file::Dirname(model_path));
  }
  const size_t max_tokens =
      static_cast<size_t>(absl::GetFlag(FLAGS_max_tokens));

  std::optional<std::string> prompt = absl::GetFlag(FLAGS_prompt);
  if (!prompt.has_value()) {
    prompt.emplace("Write an email");
  }

  const uint32_t topk = absl::GetFlag(FLAGS_topk).value_or(1);
  const float temperature = absl::GetFlag(FLAGS_temperature).value_or(0.0f);
  const uint32_t random_seed = absl::GetFlag(FLAGS_random_seed).value_or(0);

  const LlmModelSettings model_settings = {
      .model_path = model_path.c_str(),
      .cache_dir = cache_dir.c_str(),
      .max_num_tokens = max_tokens,
  };

  const LlmSessionConfig session_config = {
      .topk = topk,
      .topp = 1.0f,
      .temperature = temperature,
      .random_seed = random_seed,
  };

  std::cout << "INFO: Prompt: " << prompt.value() << std::endl;

  // Create Llm inference engine session.
  void* llm_engine = nullptr;
  char* error_msg = nullptr;
  int error_code =
      LlmInferenceEngine_CreateEngine(&model_settings, &llm_engine, &error_msg);
  if (error_code) {
    std::cerr << "ERROR: Failed to create engine: " << std::string(error_msg) << std::endl;
    free(error_msg);
    return EXIT_FAILURE;
  }
  std::cout << "INFO: Created engine successfully" << std::endl;

  void* llm_engine_session = nullptr;
  error_code = LlmInferenceEngine_CreateSession(
      llm_engine, &session_config, &llm_engine_session, &error_msg);
  if (error_code) {
    std::cerr << "ERROR: Failed to create session: " << std::string(error_msg) << std::endl;
    free(error_msg);
    return EXIT_FAILURE;
  }
  std::cout << "INFO: Created session successfully" << std::endl;

  // Create a mutable character array to hold the string

  // Use a vector as a temporary container for the string characters
  std::vector<char> char_vec(prompt.value().begin(), prompt.value().end());
  // Add the null terminator to the vector
  char_vec.push_back('\0');
  // Get a pointer to the underlying character array
  char* prompt_str = char_vec.data();

  std::cout << "INFO: Adding query chunk" << std::endl;
  error_code = LlmInferenceEngine_Session_AddQueryChunk(llm_engine_session,
                                                        prompt_str, &error_msg);
  if (error_code) {
    std::cerr << "ERROR: Failed to add query chunk: " << std::string(error_msg) << std::endl;
    free(error_msg);
    return EXIT_FAILURE;
  }
  std::cout << "INFO: Added query chunk successfully" << std::endl;

  // Optional to receive the number of tokens of the input.
  // std::cout << "INFO: Getting token count" << std::endl;
  // int num_tokens = LlmInferenceEngine_Session_SizeInTokens(
  //     llm_engine_session, prompt_str, /** error_msg=*/nullptr);
  // std::cout << "INFO: Number of tokens for input prompt: " << num_tokens << std::endl;

  // std::cout << "INFO: Starting prediction (async)" << std::endl;
  // error_code = LlmInferenceEngine_Session_PredictAsync(
  //     llm_engine_session,
  //     /*callback_context=*/nullptr, &error_msg, async_callback_print);
  // if (error_code) {
  //   std::cerr << "ERROR: Failed to predict asyncously: " << std::string(error_msg) << std::endl;
  //   free(error_msg);
  //   return EXIT_FAILURE;
  // }
  // std::cout << "INFO: Prediction started successfully" << std::endl;

   // Optional to use the following for the sync version.
   LlmResponseContext output;
   error_code = LlmInferenceEngine_Session_PredictSync(llm_engine_session, &output, &error_msg);
   if (error_code) {
     std::cerr << "ERROR: Failed to predict synchronously: " << std::string(error_msg) << std::endl;
     free(error_msg);
     return EXIT_FAILURE;
   }
   if (output.response_array && output.response_array[0]) {
     std::cout << output.response_array[0];
   }
   std::cout << std::endl;

   LlmInferenceEngine_CloseResponseContext(&output);

  std::cout << "INFO: Deleting session and engine" << std::endl;
  LlmInferenceEngine_Session_Delete(llm_engine_session);
  LlmInferenceEngine_Engine_Delete(llm_engine);

  std::cout << "INFO: Cleanup completed successfully" << std::endl;

  base::ThreadPoolInstance::Get()->Shutdown();
  return EXIT_SUCCESS;
}
