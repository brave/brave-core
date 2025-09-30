// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/og_llm_executor.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

class OgLlmExecutorTest : public testing::Test {
 protected:
  void SetUp() override {
    executor_ = std::make_unique<OgLlmExecutor>();
    // Note: You'll need to set these to valid paths for testing
#if defined(OS_MAC)
    library_path_ = base::FilePath(
        "/Users/darkdh/Projects/onnxruntime-genai/build/macOS/Release/"
        "libonnxruntime-genai.dylib");
#elif defined(OS_WIN)
    library_path_ = base::FilePath("path/to/onnxruntime-genai.dll");
#else
    library_path_ = base::FilePath("path/to/libonnxruntime-genai.so");
#endif
    model_path_ =
        base::FilePath("/Users/darkdh/Downloads/onnx-models/phi2-int4-cpu");
  }

  void TearDown() override { executor_.reset(); }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<OgLlmExecutor> executor_;
  base::FilePath library_path_;
  base::FilePath model_path_;
};

// Basic instantiation test
TEST_F(OgLlmExecutorTest, BasicInstantiation) {
  EXPECT_NE(executor_, nullptr);
  EXPECT_FALSE(executor_->IsInitialized());
}

// Test RAII wrappers for Model
TEST_F(OgLlmExecutorTest, ModelRAIIWrapper) {
  og::Model model1;
  EXPECT_FALSE(model1.IsValid());

  // Test move constructor
  og::Model model2(std::move(model1));
  EXPECT_FALSE(model2.IsValid());

  // Test move assignment
  og::Model model3;
  model3 = std::move(model2);
  EXPECT_FALSE(model3.IsValid());
}

// Test RAII wrappers for Sequences
TEST_F(OgLlmExecutorTest, SequencesRAIIWrapper) {
  og::Sequences sequences1;
  EXPECT_TRUE(sequences1.IsValid());

  // Test move constructor
  og::Sequences sequences2(std::move(sequences1));
  EXPECT_TRUE(sequences2.IsValid());
  EXPECT_FALSE(sequences1.IsValid());

  // Test move assignment
  og::Sequences sequences3;
  sequences3 = std::move(sequences2);
  EXPECT_TRUE(sequences3.IsValid());
  EXPECT_FALSE(sequences2.IsValid());
}

// Test model initialization with invalid path
TEST_F(OgLlmExecutorTest, InitializeWithInvalidPath) {
  base::FilePath invalid_path("/invalid/path/to/model");
  EXPECT_FALSE(executor_->Initialize(library_path_, invalid_path));
  EXPECT_FALSE(executor_->IsInitialized());
}

// Test model initialization with valid path
// NOTE: This test is disabled by default because it requires a real model
TEST_F(OgLlmExecutorTest, InitializeWithValidPath) {
  // Set model_path_ to a valid model directory
  // For example: base::FilePath("/path/to/gemma3-1b-it-int4")
  if (model_path_.empty() || !base::PathExists(model_path_)) {
    GTEST_SKIP() << "Model path not configured or doesn't exist";
  }
  if (library_path_.empty() || !base::PathExists(library_path_)) {
    GTEST_SKIP() << "Library path not configured or doesn't exist";
  }

  EXPECT_TRUE(executor_->Initialize(library_path_, model_path_));
  EXPECT_TRUE(executor_->IsInitialized());
}

// Test generation with valid model
// NOTE: This test is disabled by default because it requires a real model
TEST_F(OgLlmExecutorTest, GenerateWithValidModel) {
  if (model_path_.empty() || !base::PathExists(model_path_)) {
    GTEST_SKIP() << "Model path not configured or doesn't exist";
  }
  if (library_path_.empty() || !base::PathExists(library_path_)) {
    GTEST_SKIP() << "Library path not configured or doesn't exist";
  }

  ASSERT_TRUE(executor_->Initialize(library_path_, model_path_));
  ASSERT_TRUE(executor_->IsInitialized());

  std::vector<std::string> generated_tokens;
  base::RunLoop run_loop;

  auto token_callback = base::BindRepeating(
      [](std::vector<std::string>* tokens, const std::string& token) {
        tokens->push_back(token);
      },
      &generated_tokens);

  auto completion_callback = base::BindOnce(
      [](base::RunLoop* loop, bool success) {
        EXPECT_TRUE(success);
        loop->Quit();
      },
      &run_loop);

  executor_->Generate("How good is baccon in Canada?", 256, token_callback,
                      std::move(completion_callback));

  run_loop.Run();

  std::string full_output;
  for (const auto& token : generated_tokens) {
    full_output += token;
  }
  LOG(ERROR) << "Generated output: " << full_output;

  EXPECT_TRUE(generated_tokens.empty());
}

// Test generation without initialization
TEST_F(OgLlmExecutorTest, GenerateWithoutInitialization) {
  ASSERT_FALSE(executor_->IsInitialized());

  base::RunLoop run_loop;
  bool completion_success = false;

  auto token_callback = base::BindRepeating([](const std::string& token) {
    // Should not be called
    FAIL() << "Token callback should not be called";
  });

  auto completion_callback = base::BindOnce(
      [](base::RunLoop* loop, bool* success, bool result) {
        *success = result;
        loop->Quit();
      },
      &run_loop, &completion_success);

  executor_->Generate("Test prompt", 10, token_callback,
                      std::move(completion_callback));

  run_loop.Run();

  EXPECT_FALSE(completion_success);
}

// Test CheckResult helper
TEST_F(OgLlmExecutorTest, CheckResultHelper) {
  // nullptr result means success
  EXPECT_TRUE(og::CheckResult(nullptr));
}

// Test vision model with image input
// NOTE: This test requires a vision-capable model
TEST_F(OgLlmExecutorTest, GenerateWithImage) {
  base::FilePath vision_model_path(
      "/Users/darkdh/Downloads/onnx-models/phi-3.5-v-cpu-int4");
  base::FilePath image_path("/Users/darkdh/Downloads/tahoe-start.jpeg");

  if (!base::PathExists(vision_model_path) || !base::PathExists(image_path)) {
    GTEST_SKIP() << "Vision model or image not found";
  }
  if (library_path_.empty() || !base::PathExists(library_path_)) {
    GTEST_SKIP() << "Library path not configured or doesn't exist";
  }

  ASSERT_TRUE(executor_->Initialize(library_path_, vision_model_path));
  ASSERT_TRUE(executor_->IsInitialized());

  std::vector<std::string> generated_tokens;
  base::RunLoop run_loop;

  auto token_callback = base::BindRepeating(
      [](std::vector<std::string>* tokens, const std::string& token) {
        tokens->push_back(token);
      },
      &generated_tokens);

  auto completion_callback = base::BindOnce(
      [](base::RunLoop* loop, bool success) {
        EXPECT_TRUE(success);
        loop->Quit();
      },
      &run_loop);

  std::vector<base::FilePath> image_paths = {image_path};
  executor_->GenerateWithImage("Describe this image", image_paths, 4096,
                               token_callback, std::move(completion_callback));

  run_loop.Run();

  std::string full_output;
  for (const auto& token : generated_tokens) {
    full_output += token;
  }
  LOG(ERROR) << "Generated output: " << full_output;

  EXPECT_TRUE(generated_tokens.empty());
}

}  // namespace local_ai
