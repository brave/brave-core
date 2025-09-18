// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/mediapipe_llm_wrapper.h"

#include <memory>

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

}  // namespace local_ai
