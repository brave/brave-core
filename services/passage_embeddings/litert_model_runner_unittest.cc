// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/passage_embeddings/litert_model_runner.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "base/base_paths.h"
#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_history_embeddings {

namespace {

constexpr char kTestDataDir[] = "third_party/litert/src/litert/test/testdata/";

// The only LiteRT test model shaped like an embedder: one int32 input, float32
// output. Stands in for EmbeddingGemma, whose real model is too large to ship
// as test data.
constexpr char kInt32InputModel[] = "simple_scatter_nd_op.tflite";
constexpr size_t kInt32InputModelWindow = 2 * 4;

// Single float32 input -- the layout the runner must refuse, because writing
// int32 tokens into it would silently produce garbage.
constexpr char kFloatInputModel[] = "simple_l2_norm.tflite";

// Two inputs; the runner only drives single-input models.
constexpr char kTwoInputModel[] = "simple_atan2_op.tflite";

// Bytes that are not a valid .tflite flatbuffer.
std::vector<uint8_t> MalformedModelBytes() {
  return std::vector<uint8_t>(64, 0xAB);
}

std::unique_ptr<LitertModelRunner> CreateRunnerForTestModel(
    const char* model_name) {
  const base::FilePath root =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);
  base::File model_file(root.AppendASCII(kTestDataDir).AppendASCII(model_name),
                        base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!model_file.IsValid()) {
    return nullptr;
  }
  return LitertModelRunner::CreateFromFile(model_file);
}

}  // namespace

// A missing/withheld model must surface as nullptr rather than crash, so the
// utility process degrades gracefully when the component ships no valid model.
TEST(LitertModelRunnerTest, CreateFailsOnEmptyModel) {
  EXPECT_EQ(nullptr, LitertModelRunner::Create(base::span<const uint8_t>()));
}

TEST(LitertModelRunnerTest, CreateFailsOnMalformedModel) {
  EXPECT_EQ(nullptr, LitertModelRunner::Create(MalformedModelBytes()));
}

// CreateFromFile reads the model from a base::File; an invalid or empty file
// must fail without reading past the end.
TEST(LitertModelRunnerTest, CreateFromFileFailsOnInvalidFile) {
  base::File invalid_file;
  EXPECT_EQ(nullptr, LitertModelRunner::CreateFromFile(invalid_file));
}

TEST(LitertModelRunnerTest, CreateFromFileFailsOnMalformedModelFile) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  const base::FilePath model_path =
      temp_dir.GetPath().AppendASCII("model.tflite");
  ASSERT_TRUE(base::WriteFile(model_path, MalformedModelBytes()));

  base::File model_file(model_path,
                        base::File::FLAG_OPEN | base::File::FLAG_READ);
  ASSERT_TRUE(model_file.IsValid());
  EXPECT_EQ(nullptr, LitertModelRunner::CreateFromFile(model_file));
}

// Write() only checks that the buffer is large enough, so int32 tokens written
// into a float32 input would silently succeed with garbage. Rejecting at
// creation leaves the caller on upstream's tflite executor instead of an
// embedder that fails every request.
TEST(LitertModelRunnerTest, CreateFailsOnModelWithNonInt32Input) {
  EXPECT_EQ(nullptr, CreateRunnerForTestModel(kFloatInputModel));
}

TEST(LitertModelRunnerTest, CreateFailsOnModelWithMultipleInputs) {
  EXPECT_EQ(nullptr, CreateRunnerForTestModel(kTwoInputModel));
}

// The window comes from the model's input tensor; the controller reports it as
// the embedder's input window size.
TEST(LitertModelRunnerTest, CreateReportsModelInputWindow) {
  std::unique_ptr<LitertModelRunner> runner =
      CreateRunnerForTestModel(kInt32InputModel);
  ASSERT_TRUE(runner);
  EXPECT_EQ(kInt32InputModelWindow, runner->window());
}

TEST(LitertModelRunnerTest, RunReturnsFloatOutputForInt32Model) {
  std::unique_ptr<LitertModelRunner> runner =
      CreateRunnerForTestModel(kInt32InputModel);
  ASSERT_TRUE(runner);

  const std::vector<int> tokens(runner->window(), 0);
  std::optional<std::vector<float>> output = runner->Run(tokens);
  ASSERT_TRUE(output.has_value());
  EXPECT_FALSE(output->empty());
}

TEST(LitertModelRunnerTest, RunRejectsTokensThatDoNotFillTheWindow) {
  std::unique_ptr<LitertModelRunner> runner =
      CreateRunnerForTestModel(kInt32InputModel);
  ASSERT_TRUE(runner);

  const std::vector<int> tokens(runner->window() - 1, 0);
  EXPECT_FALSE(runner->Run(tokens).has_value());
}

}  // namespace brave_history_embeddings
