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

constexpr int kSingleThread = 1;

std::unique_ptr<LitertModelRunner> CreateRunnerForTestModel(
    const char* model_name,
    int num_threads = kSingleThread) {
  const base::FilePath root =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);
  base::File model_file(root.AppendASCII(kTestDataDir).AppendASCII(model_name),
                        base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!model_file.IsValid()) {
    return nullptr;
  }
  return LitertModelRunner::CreateFromFile(model_file, num_threads);
}

}  // namespace

// A missing/withheld model must surface as nullptr rather than crash, so the
// utility process degrades gracefully when the component ships no valid model.
TEST(LitertModelRunnerTest, CreateFailsOnEmptyModel) {
  EXPECT_EQ(nullptr, LitertModelRunner::Create(base::span<const uint8_t>(),
                                               kSingleThread));
}

TEST(LitertModelRunnerTest, CreateFailsOnMalformedModel) {
  EXPECT_EQ(nullptr,
            LitertModelRunner::Create(MalformedModelBytes(), kSingleThread));
}

// An invalid file must fail rather than yield a runner over nothing.
TEST(LitertModelRunnerTest, CreateFromFileFailsOnInvalidFile) {
  base::File invalid_file;
  EXPECT_EQ(nullptr,
            LitertModelRunner::CreateFromFile(invalid_file, kSingleThread));
}

// A truncated download leaves a zero-length file, which must not map into an
// empty model the runner then accepts.
TEST(LitertModelRunnerTest, CreateFromFileFailsOnEmptyModelFile) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  const base::FilePath model_path =
      temp_dir.GetPath().AppendASCII("empty.tflite");
  ASSERT_TRUE(base::WriteFile(model_path, base::span<const uint8_t>()));

  base::File model_file(model_path,
                        base::File::FLAG_OPEN | base::File::FLAG_READ);
  ASSERT_TRUE(model_file.IsValid());
  EXPECT_EQ(nullptr,
            LitertModelRunner::CreateFromFile(model_file, kSingleThread));
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
  EXPECT_EQ(nullptr,
            LitertModelRunner::CreateFromFile(model_file, kSingleThread));
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

// LiteRT silently falls back to a single thread when no CPU options are
// attached, so check that a higher count still compiles and runs.
TEST(LitertModelRunnerTest, RunReturnsSameShapedOutputWithMultipleThreads) {
  std::unique_ptr<LitertModelRunner> single_thread_runner =
      CreateRunnerForTestModel(kInt32InputModel, kSingleThread);
  ASSERT_TRUE(single_thread_runner);
  std::unique_ptr<LitertModelRunner> multi_thread_runner =
      CreateRunnerForTestModel(kInt32InputModel, /*num_threads=*/4);
  ASSERT_TRUE(multi_thread_runner);
  ASSERT_EQ(single_thread_runner->window(), multi_thread_runner->window());

  const std::vector<int> tokens(multi_thread_runner->window(), 0);
  std::optional<std::vector<float>> single_thread_output =
      single_thread_runner->Run(tokens);
  std::optional<std::vector<float>> multi_thread_output =
      multi_thread_runner->Run(tokens);
  ASSERT_TRUE(single_thread_output.has_value());
  ASSERT_TRUE(multi_thread_output.has_value());
  EXPECT_FALSE(multi_thread_output->empty());
  EXPECT_EQ(single_thread_output->size(), multi_thread_output->size());
}

TEST(LitertModelRunnerTest, RunRejectsTokensThatDoNotFillTheWindow) {
  std::unique_ptr<LitertModelRunner> runner =
      CreateRunnerForTestModel(kInt32InputModel);
  ASSERT_TRUE(runner);

  const std::vector<int> tokens(runner->window() - 1, 0);
  EXPECT_FALSE(runner->Run(tokens).has_value());
}

}  // namespace brave_history_embeddings
