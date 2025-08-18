/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/browser/text_embedder.h"

#include <string.h>

#include <memory>
#include <string_view>
#include <utility>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/local_ai/browser/local_models_updater.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

class TextEmbedderUnitTest : public testing::Test {
 public:
  TextEmbedderUnitTest()
      : embedder_(nullptr, base::OnTaskRunnerDeleter(nullptr)) {}

  ~TextEmbedderUnitTest() override = default;

  // prepare test env before each test run involving TextEmbedderUnitTest
  void SetUp() override {
    // task runner where TFLite embedding work will run
    embedder_task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT});

    // fetch path to test data dir.
    base::FilePath test_dir =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
            .AppendASCII("brave")
            .AppendASCII("test")
            .AppendASCII("data");

    // constructs sub path like ../local-ai/local-models-updater
    model_dir_ =
        test_dir.AppendASCII("local-ai").AppendASCII("local-models-updater");

    // create text embedder using the given model file. also passes test runner
    // so it can work off thread. kUniversalQAModelName is defined in
    // local_models_updater.h file.
    embedder_ = TextEmbedder::Create(
        base::FilePath(model_dir_.AppendASCII(kUniversalQAModelName)),
        embedder_task_runner_);

    // if embedder creation fails, test should fail
    ASSERT_TRUE(embedder_);

    base::RunLoop run_loop;
    embedder_->Initialize(
        base::BindLambdaForTesting([&run_loop](bool initialized) {
          ASSERT_TRUE(initialized);
          run_loop.Quit();
        }));

    run_loop.Run();
    ASSERT_TRUE(embedder_->IsInitialized());
  }

  void SetTabs(local_ai::TextEmbedder* embedder,
               const std::vector<std::string>& tabs) {
    embedder->tabs_ = tabs;
  }

  absl::Status EmbedTabs(local_ai::TextEmbedder* embedder) {
    absl::Status status;
    base::RunLoop run_loop;
    embedder_task_runner_->PostTask(FROM_HERE,
                                    base::BindLambdaForTesting([&]() {
                                      status = embedder->EmbedTabs();
                                      run_loop.Quit();
                                    }));
    run_loop.Run();
    return status;
  }

  size_t embeddings_size() const { return embedder_->embeddings_.size(); }

  size_t single_embed_size() const {
    size_t x = embedder_->embeddings_[0]
                   .embeddings(0)
                   .feature_vector()
                   .value_float()
                   .size();
    return x;
  }

  std::vector<double> single_embed_all_values() const {
    const auto& value_float =
        embedder_->embeddings_[0].embeddings(0).feature_vector().value_float();
    return std::vector<double>(value_float.begin(), value_float.end());
  }

  absl::StatusOr<std::vector<int>> VerifySuggestTabsForGroup(
      std::vector<std::pair<int, std::string>> group_tabs,
      std::vector<std::pair<int, std::string>> candiate_tabs) {
    absl::StatusOr<std::vector<int>> result;

    base::RunLoop run_loop;
    embedder_task_runner_->PostTask(
        FROM_HERE, base::BindLambdaForTesting([&]() {
          result = embedder_->SuggestTabsForGroup(group_tabs, candiate_tabs);
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

 protected:
  std::unique_ptr<local_ai::TextEmbedder, base::OnTaskRunnerDeleter> embedder_;
  base::FilePath model_dir_;
  scoped_refptr<base::SequencedTaskRunner> embedder_task_runner_;
  base::test::TaskEnvironment task_environment_;
};

// test to check if an embedder pointer is created
TEST_F(TextEmbedderUnitTest, Create) {
  EXPECT_FALSE(TextEmbedder::Create(base::FilePath(), embedder_task_runner_));
  EXPECT_TRUE(TextEmbedder::Create(
      base::FilePath(model_dir_.AppendASCII("model.tflite")),
      embedder_task_runner_));
  EXPECT_TRUE(TextEmbedder::Create(
      base::FilePath(model_dir_.AppendASCII(kUniversalQAModelName)),
      embedder_task_runner_));
}

// test to check if embedder object is getting initialized
TEST_F(TextEmbedderUnitTest, Initialize) {
  auto embedder = TextEmbedder::Create(
      model_dir_.AppendASCII(kUniversalQAModelName), embedder_task_runner_);
  ASSERT_TRUE(embedder);
  base::RunLoop run_loop;
  embedder->Initialize(
      base::BindLambdaForTesting([&run_loop](bool initialized) {
        EXPECT_TRUE(initialized);
        run_loop.Quit();
      }));
  run_loop.Run();
  EXPECT_TRUE(embedder->IsInitialized());
}

// test to check if tab embeddings are being generated
TEST_F(TextEmbedderUnitTest, EmbedTabs) {
  // Test empty segments.
  auto status = EmbedTabs(embedder_.get());
  EXPECT_FALSE(status.ok());
  EXPECT_TRUE(absl::IsFailedPrecondition(status));
  EXPECT_EQ(status.ToString(), "FAILED_PRECONDITION: No tabs to embed.");
  EXPECT_EQ(embeddings_size(), 0u);

  SetTabs(embedder_.get(),
          {"Best time to visit Bali lonelyplanet.com",
           "Train travel tips across Europe eurotripadvisor.net",
           "Understanding stock market indices nasdaq.com"});
  EXPECT_TRUE(EmbedTabs(embedder_.get()).ok());
  EXPECT_EQ(embeddings_size(), 3u);

  SetTabs(embedder_.get(),
          {"Best time to visit Bali lonelyplanet.com",
           "Train travel tips across Europe eurotripadvisor.net"});
  EXPECT_TRUE(EmbedTabs(embedder_.get()).ok());
  EXPECT_EQ(embeddings_size(), 2u);
}

TEST_F(TextEmbedderUnitTest, InspectEmbedding) {
  SetTabs(embedder_.get(), {"Best time to visit Bali lonelyplanet.com"});
  EXPECT_TRUE(EmbedTabs(embedder_.get()).ok());

  size_t embed_size;
  embed_size = single_embed_size();
  EXPECT_EQ(embed_size, 100u);

  std::vector<double> ref_embed;

  ref_embed = {1.13038528,  0.83160812,  0.06623636,  1.53322351,  0.80242991,
               0.53564548,  -1.35267901, -0.20925859, 2.54806185,  0.76327729,
               0.38529554,  -1.32557571, 0.83827466,  -2.23799992, 0.68120342,
               1.98290706,  -0.8830418,  -1.01441491, -0.34110293, -0.96301019,
               -0.84333646, 0.39633867,  1.57250464,  -0.68522394, 2.12945008,
               0.31963441,  -0.20059782, -1.47930455, 1.57752252,  1.47504377,
               1.0764029,   0.37355256,  -0.42052221, -0.19836292, 0.34847736,
               1.81372452,  -0.51717269, 0.61680889,  -0.66423297, -0.31588519,
               -0.62898684, 2.61123896,  1.22074258,  1.20217431,  -1.53226435,
               0.71343607,  -1.1455394,  1.47197676,  1.42579842,  -0.45529655,
               -0.38407931, 0.99821067,  1.13552809,  -0.00381027, -0.29902685,
               -1.69532633, 1.23231995,  -0.02990777, 0.1631268,   2.29497313,
               -1.03595591, -1.48891973, 1.62486315,  0.75086355,  -0.19678396,
               1.11072791,  0.71181858,  0.02584061,  -1.30660319, 0.48642394,
               0.09257797,  -0.86246204, -1.1063292,  -0.18176673, 0.31313393,
               0.21751097,  -0.06199005, -0.42951369, 0.1223064,   -0.19713967,
               0.40490884,  -1.4795959,  1.10003591,  0.17397788,  -0.13940017,
               1.53504717,  3.41920114,  0.39580631,  -0.1163614,  0.07005535,
               -1.81727087, -0.07773433, -1.27058601, -1.97312176, -1.59849489,
               0.41968283,  -0.35715488, 1.08086693,  0.23084836,  -0.7961573};

  size_t ref_size;
  ref_size = ref_embed.size();

  // check if both sizes are equal
  EXPECT_EQ(ref_size, embed_size);

  // check actual embed values
  std::vector<double> embed_output = single_embed_all_values();

  for (size_t i = 0; i < ref_embed.size(); ++i) {
    EXPECT_NEAR(ref_embed[i], embed_output[i], 1e-5f)
        << "Mismatch at index " << i;
  }
}

TEST_F(TextEmbedderUnitTest, VerifySuggestTabsForGroup) {
  std::vector<std::pair<int, std::string>> travel_group_tabs = {
      {1, "Top 10 places to visit in Italy travelblog.com"},
      {3, "Flight comparison: Rome vs Venice skyscanner.com"},
      {4, "Train travel tips across Europe eurotripadvisor.net"},
      {6, "Travel insurance for international trips safetravel.com"},
      {7, "Visa requirements for Schengen countries visaguide.world"}};
  std::vector<std::pair<int, std::string>> candidate_tabs = {
      {0, "Compare savings accounts interest rates bankrate.com"},
      {10, "Tips to improve credit score nerdwallet.com"},
      {2, "Live coverage of Formula 1 race formula1.com"},
      {5, "Visiting Italy in October: all you need to know mamalovesitaly.com"},
      {9, "Review of hotels in Venice booking.com"}};

  absl::StatusOr<std::vector<int>> result;
  result = VerifySuggestTabsForGroup(travel_group_tabs, candidate_tabs);

  const auto& indices = result.value();
  std::vector<int> expected = {candidate_tabs[4].first,
                               candidate_tabs[3].first};

  EXPECT_EQ(indices, expected);
}

}  // namespace local_ai
