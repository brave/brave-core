// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/mediapipe_llm_wrapper.h"

#include <memory>
#include <fstream>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/task_environment.h"
#include "brave/third_party/mediapipe/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

static_assert(BUILDFLAG(BRAVE_MEDIAPIPE_LLM_ENABLED),
              "These tests require Brave MediaPipe LLM to be enabled");

namespace local_ai {

class MediaPipeLLMWrapperTest : public ::testing::Test {
 public:
  MediaPipeLLMWrapperTest() = default;
  ~MediaPipeLLMWrapperTest() override = default;

 protected:
  void SetUp() override { wrapper_ = std::make_unique<MediaPipeLLMWrapper>(); }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<MediaPipeLLMWrapper> wrapper_;
};

TEST_F(MediaPipeLLMWrapperTest, InitializeWithoutModel) {
  MediaPipeLLMWrapper::ModelSettings settings;
  settings.model_path = "/nonexistent/path";
  settings.cache_dir = "/tmp";

  std::string error;
  bool result = wrapper_->Initialize(settings, &error);

  // Should fail since model doesn't exist
  EXPECT_FALSE(result);
  EXPECT_FALSE(error.empty());
}

TEST_F(MediaPipeLLMWrapperTest, CreateSessionWithoutInitialization) {
  MediaPipeLLMWrapper::SessionConfig config;
  config.topk = 10;
  config.temperature = 0.7f;

  std::string error;
  bool result = wrapper_->CreateSession(config, &error);

  EXPECT_FALSE(result);
  EXPECT_EQ(error, "Engine not initialized");
}

TEST_F(MediaPipeLLMWrapperTest, IsReadyWithoutInitialization) {
  EXPECT_FALSE(wrapper_->IsReady());
}

TEST_F(MediaPipeLLMWrapperTest, GetTokenCountWithoutSession) {
  std::string error;
  int result = wrapper_->GetTokenCount("test input", &error);

  EXPECT_EQ(result, -1);
  EXPECT_EQ(error, "Session not created");
}

TEST_F(MediaPipeLLMWrapperTest, AddQueryChunkWithoutSession) {
  std::string error;
  bool result = wrapper_->AddQueryChunk("test query", &error);

  EXPECT_FALSE(result);
  EXPECT_EQ(error, "Session not created");
}

class MediaPipeLLMWrapperRealModelTest : public ::testing::Test {
 public:
  MediaPipeLLMWrapperRealModelTest() = default;
  ~MediaPipeLLMWrapperRealModelTest() override = default;

 protected:
  void SetUp() override {
    wrapper_ = std::make_unique<MediaPipeLLMWrapper>();

    // Check if the test model exists
    base::FilePath test_data_dir;
    base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &test_data_dir);
    model_path_ = test_data_dir.AppendASCII("brave")
                      .AppendASCII("test")
                      .AppendASCII("data")
                      .AppendASCII("local_ai")
                      .AppendASCII("Gemma3-1B-IT_multi-prefill-seq_q4_ekv2048.task");

    model_available_ = base::PathExists(model_path_);
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<MediaPipeLLMWrapper> wrapper_;
  base::FilePath model_path_;
  bool model_available_ = false;
};

TEST_F(MediaPipeLLMWrapperRealModelTest, InitializeWithRealModel) {
  if (!model_available_) {
    GTEST_SKIP() << "Test model not available at " << model_path_;
  }

  MediaPipeLLMWrapper::ModelSettings settings;
  settings.model_path = model_path_.AsUTF8Unsafe();
  settings.cache_dir = "/tmp";

  std::string error;
  bool result = wrapper_->Initialize(settings, &error);

  EXPECT_TRUE(result) << "Failed to initialize with error: " << error;
  EXPECT_TRUE(error.empty());
}

TEST_F(MediaPipeLLMWrapperRealModelTest, CreateSessionWithRealModel) {
  if (!model_available_) {
    GTEST_SKIP() << "Test model not available at " << model_path_;
  }

  // First initialize the model
  MediaPipeLLMWrapper::ModelSettings settings;
  settings.model_path = model_path_.AsUTF8Unsafe();
  settings.cache_dir = "/tmp";

  std::string error;
  bool init_result = wrapper_->Initialize(settings, &error);
  ASSERT_TRUE(init_result) << "Failed to initialize: " << error;

  // Then create a session
  MediaPipeLLMWrapper::SessionConfig config;
  config.topk = 10;
  config.temperature = 0.7f;

  bool session_result = wrapper_->CreateSession(config, &error);
  EXPECT_TRUE(session_result) << "Failed to create session: " << error;
  EXPECT_TRUE(error.empty());
  EXPECT_TRUE(wrapper_->IsReady());
}

TEST_F(MediaPipeLLMWrapperRealModelTest, SimpleInference) {
  if (!model_available_) {
    GTEST_SKIP() << "Test model not available at " << model_path_;
  }

  // Initialize model
  MediaPipeLLMWrapper::ModelSettings settings;
  settings.model_path = model_path_.AsUTF8Unsafe();
  settings.cache_dir = "/tmp";

  std::string error;
  bool init_result = wrapper_->Initialize(settings, &error);
  ASSERT_TRUE(init_result) << "Failed to initialize: " << error;

  // Create session
  MediaPipeLLMWrapper::SessionConfig config;
  config.topk = 10;
  config.temperature = 0.7f;

  bool session_result = wrapper_->CreateSession(config, &error);
  ASSERT_TRUE(session_result) << "Failed to create session: " << error;

  // Test token counting
  int token_count = wrapper_->GetTokenCount("Hello, world!", &error);
  EXPECT_GT(token_count, 0) << "Token count should be positive: " << error;

  // Test adding a query chunk
  bool chunk_result = wrapper_->AddQueryChunk("What is the capital of France?", &error);
  EXPECT_TRUE(chunk_result) << "Failed to add query chunk: " << error;

  // Test prediction
  MediaPipeLLMWrapper::Response response;
  bool predict_result = wrapper_->PredictSync(&response, &error);
  EXPECT_TRUE(predict_result) << "Failed to predict: " << error;
  EXPECT_TRUE(response.done) << "Response should be marked as done";
  EXPECT_FALSE(response.responses.empty()) << "Response should contain at least one result";
  if (!response.responses.empty()) {
    EXPECT_FALSE(response.responses[0].empty()) << "First response should not be empty";
  }
}

}  // namespace local_ai
