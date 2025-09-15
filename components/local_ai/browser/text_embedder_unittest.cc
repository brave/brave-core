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
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/local_ai/browser/local_models_updater.h"
#include "brave/components/local_ai/browser/yake_keyword_extractor.h"
#include "build/build_config.h"
#include "testing/gmock/include/gmock/gmock.h"
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

    base::test::TestFuture<bool> init_future;
    embedder_->Initialize(init_future.GetCallback());
    ASSERT_TRUE(init_future.Get());
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

  // Helper method to create mock TabGroupIds for testing
  tab_groups::TabGroupId CreateMockTabGroupId() {
    return tab_groups::TabGroupId::GenerateNew();
  }

  // Helper method to access private SerializeTabInfo for testing
  std::string CallSerializeTabInfo(
      const local_ai::TextEmbedder::TabInfo& tab_info) {
    base::test::TestFuture<std::string> future;
    embedder_task_runner_->PostTask(
        FROM_HERE, base::BindLambdaForTesting(
                       [embedder = embedder_.get(), tab_info,
                        callback = base::BindPostTaskToCurrentDefault(
                            future.GetCallback())]() mutable {
                         std::string result =
                             embedder->SerializeTabInfo(tab_info);
                         std::move(callback).Run(std::move(result));
                       }));
    return future.Get();
  }

  std::vector<double> single_embed_all_values() const {
    const auto& value_float =
        embedder_->embeddings_[0].embeddings(0).feature_vector().value_float();
    return std::vector<double>(value_float.begin(), value_float.end());
  }

  absl::StatusOr<std::vector<int>> VerifySuggestTabsForGroup(
      std::vector<local_ai::TextEmbedder::TabInfo> group_tabs,
      std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs) {
    base::test::TestFuture<absl::StatusOr<std::vector<int>>> future;
    embedder_->SuggestTabsForGroup(
        std::move(group_tabs), std::move(candidate_tabs), future.GetCallback());
    return future.Get();
  }

  absl::StatusOr<std::vector<int>> VerifySuggestTabsForGroupWithMethod(
      std::vector<local_ai::TextEmbedder::TabInfo> group_tabs,
      std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs,
      local_ai::ClusteringMethod method) {
    base::test::TestFuture<absl::StatusOr<std::vector<int>>> future;
    embedder_->SuggestTabsForGroup(std::move(group_tabs),
                                   std::move(candidate_tabs),
                                   future.GetCallback(), method);
    return future.Get();
  }

  absl::StatusOr<tab_groups::TabGroupId> VerifySuggestGroupForTab(
      local_ai::TextEmbedder::CandidateTab candidate_tab,
      std::map<tab_groups::TabGroupId,
               std::vector<local_ai::TextEmbedder::TabInfo>> group_tabs) {
    base::test::TestFuture<absl::StatusOr<tab_groups::TabGroupId>> future;
    embedder_->SuggestGroupForTab(std::move(candidate_tab),
                                  std::move(group_tabs), future.GetCallback());
    return future.Get();
  }

  absl::StatusOr<tab_groups::TabGroupId> VerifySuggestGroupForTabWithMethod(
      local_ai::TextEmbedder::CandidateTab candidate_tab,
      std::map<tab_groups::TabGroupId,
               std::vector<local_ai::TextEmbedder::TabInfo>> group_tabs,
      local_ai::ClusteringMethod method) {
    base::test::TestFuture<absl::StatusOr<tab_groups::TabGroupId>> future;
    embedder_->SuggestGroupForTab(std::move(candidate_tab),
                                  std::move(group_tabs), future.GetCallback(),
                                  method);
    return future.Get();
  }

  // Helper method to access ScoreGroupKNN for testing
  double CallScoreGroupKNN(
      const tflite::task::processor::EmbeddingResult& candidate,
      const std::vector<tflite::task::processor::EmbeddingResult>&
          group_embeddings) {
    double result = -1.0;
    base::RunLoop run_loop;
    embedder_task_runner_->PostTask(
        FROM_HERE, base::BindLambdaForTesting([&]() {
          result = embedder_->ScoreGroupKNN(candidate, group_embeddings);
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

  // Helper method to get a specific embedding for testing
  tflite::task::processor::EmbeddingResult GetEmbedding(size_t index) {
    return embedder_->embeddings_[index];
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
  base::test::TestFuture<bool> future;
  embedder->Initialize(future.GetCallback());
  EXPECT_TRUE(future.Get());
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

  ref_embed = {0.10092739016,  0.07425074279,  0.00591397611,  0.13689494133,
               0.07164548337,  0.04782557487,  -0.12077489495, -0.01868362352,
               0.22750549018,  0.06814983487,  0.03440141305,  -0.11835493892,
               0.07484595478,  -0.19982145727, 0.06082181633,  0.17704525590,
               -0.07884288579, -0.09057270736, -0.03045560606, -0.08598326147,
               -0.07529779524, 0.03538763523,  0.14040215313,  -0.06118085608,
               0.19012954831,  0.02853878960,  -0.01791056618, -0.13208074868,
               0.14085020125,  0.13170032203,  0.09610727429,  0.03335298598,
               -0.03754656389, -0.01771099679, 0.03111405671,  0.16193972528,
               -0.04617633298, 0.05507221818,  -0.05930658057, -0.02820394561,
               -0.05615946278, 0.23314623535,  0.10899495333,  0.10733693093,
               -0.13680922985, 0.06369972229,  -0.10228036344, 0.13142640889,
               0.12730343640,  -0.04065163061, -0.03429286927, 0.08912606537,
               0.10138636827,  -0.00034019700, -0.02669881843, -0.15136839449,
               0.11002867669,  -0.00267032092, 0.01456489321,  0.20490823686,
               -0.09249607474, -0.13293913007, 0.14507712424,  0.06704143435,
               -0.01757001877, 0.09917218238,  0.06355525553,  0.00230723666,
               -0.11666078866, 0.04343061894,  0.00826582499,  -0.07700549066,
               -0.09877946228, -0.01622928493, 0.02795844525,  0.01942070946,
               -0.00553485472, -0.03834943846, 0.01092033274,  -0.01760163158,
               0.03615249321,  -0.13210675120, 0.09821762890,  0.01553387661,
               -0.01244646497, 0.13705776632,  0.30528572202,  0.03533969447,
               -0.01038954034, 0.00625515683,  -0.16225637496, -0.00694054272,
               -0.11344513297, -0.17617160082, -0.14272285998, 0.03747169301,
               -0.03188877180, 0.09650610387,  0.02061141841,  -0.07108546048};

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
  // Create group tabs (travel-related content)
  std::vector<local_ai::TextEmbedder::TabInfo> group_tabs = {
      {u"Top 10 places to visit in Italy", GURL("https://travelblog.com"), "",
       ""},
      {u"Flight comparison: Rome vs Venice", GURL("https://skyscanner.com"), "",
       ""},
      {u"Train travel tips across Europe", GURL("https://eurotripadvisor.net"),
       "", ""},
      {u"Travel insurance for international trips",
       GURL("https://safetravel.com"), "", ""},
      {u"Visa requirements for Schengen countries",
       GURL("https://visaguide.world"), "", ""}};

  // Create candidate tabs (mix of travel and non-travel content)
  std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs = {
      {0,
       {u"Compare savings accounts interest rates",
        GURL("https://bankrate.com"), "", ""}},
      {10,
       {u"Tips to improve credit score", GURL("https://nerdwallet.com"), "",
        ""}},
      {2,
       {u"Live coverage of Formula 1 race", GURL("https://formula1.com"), "",
        ""}},
      {5,
       {u"Visiting Italy in October: all you need to know",
        GURL("https://mamalovesitaly.com"), "", ""}},  // Travel-related
      {9,
       {u"Review of hotels in Venice", GURL("https://booking.com"), "",
        ""}}  // Travel-related
  };

  // Keep a copy of candidate indices for validation
  std::vector<int> expected_candidate_indices;
  for (const auto& candidate : candidate_tabs) {
    expected_candidate_indices.push_back(candidate.index);
  }

  auto result = VerifySuggestTabsForGroup(std::move(group_tabs),
                                          std::move(candidate_tabs));

  ASSERT_TRUE(result.ok()) << "SuggestTabsForGroup failed: "
                           << result.status().message();

  const auto& suggested_indices = result.value();
  // We expect the travel-related candidates (indices 5 and 9) to be suggested
  // The exact order may vary based on similarity scores, but we should get some
  // suggestions
  EXPECT_FALSE(suggested_indices.empty())
      << "Expected some tab suggestions but got none";

  // Verify all suggested indices are valid candidate indices
  for (int suggested_index : suggested_indices) {
    bool found = std::find(expected_candidate_indices.begin(),
                           expected_candidate_indices.end(),
                           suggested_index) != expected_candidate_indices.end();
    EXPECT_TRUE(found) << "Suggested index " << suggested_index
                       << " is not a valid candidate";
  }
}

TEST_F(TextEmbedderUnitTest, VerifySuggestGroupForTab) {
  // Create a financial candidate tab that should match the finance group
  local_ai::TextEmbedder::CandidateTab candidate_tab = {
      10,
      {u"Compare savings accounts interest rates", GURL("https://bankrate.com"),
       "", ""}};

  // Create groups with TabGroupIds
  tab_groups::TabGroupId travel_group_id = CreateMockTabGroupId();
  tab_groups::TabGroupId weather_group_id = CreateMockTabGroupId();
  tab_groups::TabGroupId finance_group_id = CreateMockTabGroupId();
  tab_groups::TabGroupId sports_group_id = CreateMockTabGroupId();

  std::map<tab_groups::TabGroupId, std::vector<local_ai::TextEmbedder::TabInfo>>
      group_tabs;

  // Travel group
  group_tabs[travel_group_id] = {{u"Top 10 places to visit in Italy",
                                  GURL("https://travelblog.com"), "", ""},
                                 {u"Flight comparison: Rome vs Venice",
                                  GURL("https://skyscanner.com"), "", ""},
                                 {u"Train travel tips across Europe",
                                  GURL("https://eurotripadvisor.net"), "", ""},
                                 {u"Travel insurance for international trips",
                                  GURL("https://safetravel.com"), "", ""},
                                 {u"Visa requirements for Schengen countries",
                                  GURL("https://visaguide.world"), "", ""}};

  // Weather group
  group_tabs[weather_group_id] = {
      {u"Woking, Surrey, United Kingdom Current Weather | AccuWeather",
       GURL("https://accuweather.com"), "", ""},
      {u"London - BBC Weather", GURL("https://bbc.co.uk"), "", ""}};

  // Finance group - this should be the best match
  group_tabs[finance_group_id] = {
      {u"Personal Savings Allowance", GURL("https://moneysavingexpert.com"), "",
       ""},
      {u"What is the Personal Savings Allowance? | Barclays",
       GURL("https://barclays.co.uk"), "", ""}};

  // Sports group
  group_tabs[sports_group_id] = {
      {u"Football Scores & Fixtures - Today's Schedule of Football",
       GURL("https://skysports.com"), "", ""}};

  auto result =
      VerifySuggestGroupForTab(std::move(candidate_tab), std::move(group_tabs));

  EXPECT_TRUE(result.ok()) << "Expected success but got error: "
                           << result.status().message();

  // If successful, verify we got a valid TabGroupId (non-empty)
  if (result.ok()) {
    // TabGroupId should be valid (we can't easily check the specific group
    // without maintaining the mapping, but we can verify the API worked)
    EXPECT_FALSE(result.value().is_empty());
  }
}

// Test that SuggestTabsForGroup fails gracefully when TextEmbedder is not
// initialized
TEST_F(TextEmbedderUnitTest, SuggestTabsForGroupNotInitialized) {
  // Create an uninitialized embedder
  auto uninitialized_embedder = TextEmbedder::Create(
      model_dir_.AppendASCII(kUniversalQAModelName), embedder_task_runner_);
  ASSERT_TRUE(uninitialized_embedder);
  EXPECT_FALSE(uninitialized_embedder->IsInitialized());

  // Test that SuggestTabsForGroup returns proper error when not initialized
  base::test::TestFuture<absl::StatusOr<std::vector<int>>> future;
  std::vector<local_ai::TextEmbedder::TabInfo> group_tabs = {
      {u"Test group tab", GURL(), "", ""}};
  std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs = {
      {0, {u"Test candidate tab", GURL(), "", ""}}};
  uninitialized_embedder->SuggestTabsForGroup(
      std::move(group_tabs), std::move(candidate_tabs), future.GetCallback());

  auto result = future.Get();
  EXPECT_FALSE(result.ok());
  EXPECT_TRUE(absl::IsFailedPrecondition(result.status()));
  EXPECT_EQ(result.status().message(),
            "TextEmbedder is not initialized. Call Initialize() first.");
}

// Test that SuggestGroupForTab fails gracefully when TextEmbedder is not
// initialized
TEST_F(TextEmbedderUnitTest, SuggestGroupForTabNotInitialized) {
  // Create an uninitialized embedder
  auto uninitialized_embedder = TextEmbedder::Create(
      model_dir_.AppendASCII(kUniversalQAModelName), embedder_task_runner_);
  ASSERT_TRUE(uninitialized_embedder);
  EXPECT_FALSE(uninitialized_embedder->IsInitialized());

  // Test that SuggestGroupForTab returns proper error when not initialized
  base::test::TestFuture<absl::StatusOr<tab_groups::TabGroupId>> future;
  local_ai::TextEmbedder::CandidateTab candidate = {
      0, {u"Test candidate tab", GURL(), "", ""}};
  std::map<tab_groups::TabGroupId, std::vector<local_ai::TextEmbedder::TabInfo>>
      groups;
  groups[CreateMockTabGroupId()] = {{u"Test group tab 1", GURL(), "", ""}};
  groups[CreateMockTabGroupId()] = {{u"Test group tab 2", GURL(), "", ""}};
  uninitialized_embedder->SuggestGroupForTab(
      std::move(candidate), std::move(groups), future.GetCallback());

  auto result = future.Get();
  EXPECT_FALSE(result.ok());
  EXPECT_TRUE(absl::IsFailedPrecondition(result.status()));
  EXPECT_EQ(result.status().message(),
            "TextEmbedder is not initialized. Call Initialize() first.");
}

// Test that methods work correctly after proper initialization
TEST_F(TextEmbedderUnitTest, InitializationWorkflow) {
  // Create an uninitialized embedder
  auto embedder = TextEmbedder::Create(
      model_dir_.AppendASCII(kUniversalQAModelName), embedder_task_runner_);
  ASSERT_TRUE(embedder);
  EXPECT_FALSE(embedder->IsInitialized());

  // Verify that methods fail before initialization
  base::test::TestFuture<absl::StatusOr<std::vector<int>>> tabs_future;
  std::vector<local_ai::TextEmbedder::TabInfo> group_tabs1 = {
      {u"Test group tab", GURL(), "", ""}};
  std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs1 = {
      {0, {u"Test candidate tab", GURL(), "", ""}}};
  embedder->SuggestTabsForGroup(std::move(group_tabs1),
                                std::move(candidate_tabs1),
                                tabs_future.GetCallback());
  auto tabs_result = tabs_future.Get();
  EXPECT_FALSE(tabs_result.ok());
  EXPECT_TRUE(absl::IsFailedPrecondition(tabs_result.status()));

  base::test::TestFuture<absl::StatusOr<tab_groups::TabGroupId>> group_future;
  local_ai::TextEmbedder::CandidateTab candidate1 = {
      0, {u"Test candidate tab", GURL(), "", ""}};
  std::map<tab_groups::TabGroupId, std::vector<local_ai::TextEmbedder::TabInfo>>
      groups1;
  groups1[CreateMockTabGroupId()] = {{u"Test group tab", GURL(), "", ""}};
  embedder->SuggestGroupForTab(std::move(candidate1), std::move(groups1),
                               group_future.GetCallback());
  auto group_result = group_future.Get();
  EXPECT_FALSE(group_result.ok());
  EXPECT_TRUE(absl::IsFailedPrecondition(group_result.status()));

  // Initialize the embedder
  base::test::TestFuture<bool> init_future;
  embedder->Initialize(init_future.GetCallback());
  EXPECT_TRUE(init_future.Get());
  EXPECT_TRUE(embedder->IsInitialized());

  // Now the methods should work with properly designed test data
  base::test::TestFuture<absl::StatusOr<std::vector<int>>> tabs_future2;
  std::vector<local_ai::TextEmbedder::TabInfo> group_tabs2 = {
      {u"Best travel destinations in Europe", GURL("https://travelblog.com"),
       "", ""},
      {u"Top places to visit in Italy", GURL("https://lonelyplanet.com"), "",
       ""}};
  std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs2 = {
      {0,
       {u"Amazing travel spots in France", GURL("https://travelguide.com"), "",
        ""}}};
  embedder->SuggestTabsForGroup(std::move(group_tabs2),
                                std::move(candidate_tabs2),
                                tabs_future2.GetCallback());
  auto tabs_result2 = tabs_future2.Get();
  EXPECT_TRUE(tabs_result2.ok());  // Should succeed with similar travel content

  base::test::TestFuture<absl::StatusOr<tab_groups::TabGroupId>> group_future2;
  local_ai::TextEmbedder::CandidateTab candidate2 = {
      0,
      {u"European travel guide recommendations", GURL("https://eurotravel.com"),
       "", ""}};
  std::map<tab_groups::TabGroupId, std::vector<local_ai::TextEmbedder::TabInfo>>
      groups2;
  groups2[CreateMockTabGroupId()] = {
      {u"Best travel destinations in Europe", GURL("https://travelblog.com"),
       "", ""},
      {u"Top places to visit in Italy", GURL("https://lonelyplanet.com"), "",
       ""}};
  embedder->SuggestGroupForTab(std::move(candidate2), std::move(groups2),
                               group_future2.GetCallback());
  auto group_result2 = group_future2.Get();
  // Should succeed with semantically similar travel content
  ASSERT_TRUE(group_result2.ok())
      << "Expected success with similar travel content: "
      << group_result2.status().message();
  EXPECT_FALSE(group_result2.value().is_empty());
}

// Test that host extraction works correctly in SerializeTabInfo
TEST_F(TextEmbedderUnitTest, HostExtractionTest) {
  // Test with valid HTTPS URL - should extract host
  local_ai::TextEmbedder::TabInfo tab1 = {
      u"Test Page", GURL("https://example.com/path?query=1"), "", ""};

  // Test with subdomain - should extract full host
  local_ai::TextEmbedder::TabInfo tab2 = {
      u"News Article", GURL("https://news.google.com/article/123"), "", ""};

  // Test with invalid URL - should use "unknown"
  local_ai::TextEmbedder::TabInfo tab3 = {u"Invalid URL", GURL("not-a-url"), "",
                                          ""};

  // Test with file URL (no host) - should use full spec
  local_ai::TextEmbedder::TabInfo tab4 = {
      u"Local File", GURL("file:///path/to/file.html"), "", ""};

  // Test SerializeTabInfo directly using the helper method
  EXPECT_THAT(CallSerializeTabInfo(tab1), testing::HasSubstr("Test Page"));
  EXPECT_THAT(CallSerializeTabInfo(tab1), testing::HasSubstr("example.com"));

  EXPECT_THAT(CallSerializeTabInfo(tab2), testing::HasSubstr("News Article"));
  EXPECT_THAT(CallSerializeTabInfo(tab2),
              testing::HasSubstr("news.google.com"));

  EXPECT_THAT(CallSerializeTabInfo(tab3), testing::HasSubstr("Invalid URL"));
  EXPECT_THAT(CallSerializeTabInfo(tab3), testing::HasSubstr("unknown"));

  EXPECT_THAT(CallSerializeTabInfo(tab4), testing::HasSubstr("Local File"));
  EXPECT_THAT(CallSerializeTabInfo(tab4),
              testing::HasSubstr("file:///path/to/file.html"));

  // Test with tab_content field present - should include content keywords
  local_ai::TextEmbedder::TabInfo tab_with_content = {
      u"Article Title", GURL("https://example.com/article"),
      "This is the main article content about travel tips and destinations.",
      ""};
  std::string result = CallSerializeTabInfo(tab_with_content);
  EXPECT_THAT(result, testing::HasSubstr("Article Title"));
  EXPECT_THAT(result, testing::HasSubstr("example.com"));
  // Keywords are currently disabled, should NOT contain keywords section
  EXPECT_THAT(result, testing::Not(testing::HasSubstr("[keywords:")));
}

// Test YAKE keyword extraction functionality (currently disabled in
// SerializeTabInfo) This test verifies the YakeKeywordExtractor still works
// independently
TEST_F(TextEmbedderUnitTest, YakeKeywordExtraction) {
  YakeKeywordExtractor extractor;

  // Test basic keyword extraction
  std::string text =
      "Machine learning and artificial intelligence are transforming "
      "technology industry";
  auto keywords = extractor.ExtractKeywords(text, 5, 2);

  EXPECT_FALSE(keywords.empty());
  EXPECT_LE(keywords.size(), 5u);

  // Check that extracted keywords are from the original text
  for (const auto& keyword : keywords) {
    EXPECT_FALSE(keyword.keyword.empty());
    EXPECT_GT(keyword.score, 0.0);
  }

  // Test with travel content similar to our tab examples
  std::string travel_text =
      "Best travel destinations in Europe include Italy, France, and Spain. "
      "Visit Rome, Paris, and Barcelona for amazing cultural experiences.";
  auto travel_keywords = extractor.ExtractKeywords(travel_text, 6, 2);

  EXPECT_FALSE(travel_keywords.empty());
  EXPECT_LE(travel_keywords.size(), 6u);

  // Test with empty text
  auto empty_keywords = extractor.ExtractKeywords("", 5, 2);
  EXPECT_TRUE(empty_keywords.empty());

  // Test with single word
  auto single_keywords = extractor.ExtractKeywords("technology", 3, 2);
  EXPECT_FALSE(single_keywords.empty());
  EXPECT_EQ(single_keywords[0].keyword, "technology");
}

// Test SerializeTabInfo format (keywords currently disabled)
TEST_F(TextEmbedderUnitTest, SerializeTabInfoFormat) {
  // Test with rich content - keywords are disabled so should only show title |
  // host
  local_ai::TextEmbedder::TabInfo tab_with_rich_content = {
      u"Best Travel Destinations in Europe",
      GURL("https://travelblog.com/europe-guide"),
      "Europe offers amazing travel destinations including Italy with its "
      "beautiful cities like Rome and Venice. "
      "France provides cultural experiences in Paris and Lyon. Spain features "
      "Barcelona and Madrid. "
      "These destinations offer rich history, excellent cuisine, and "
      "unforgettable experiences for travelers.",
      ""};

  std::string serialized = CallSerializeTabInfo(tab_with_rich_content);

  // Should contain the title
  EXPECT_THAT(serialized,
              testing::HasSubstr("Best Travel Destinations in Europe"));

  // Should contain the host
  EXPECT_THAT(serialized, testing::HasSubstr("travelblog.com"));

  // Keywords are currently disabled - should NOT contain keyword section
  EXPECT_THAT(serialized, testing::Not(testing::HasSubstr("[keywords:")));

  // Current format should be "title | host"
  EXPECT_THAT(serialized, testing::ContainsRegex(".* \\| travelblog\\.com"));

  // Test with title-only content (no tab_content)
  local_ai::TextEmbedder::TabInfo tab_title_only = {
      u"Machine Learning Tutorial for Beginners",
      GURL("https://ai-tutorial.com/ml-basics"), "", ""};

  std::string title_only_result = CallSerializeTabInfo(tab_title_only);
  EXPECT_THAT(title_only_result,
              testing::HasSubstr("Machine Learning Tutorial for Beginners"));
  EXPECT_THAT(title_only_result, testing::HasSubstr("ai-tutorial.com"));
  // Should NOT contain keywords section (keywords are disabled)
  EXPECT_THAT(title_only_result,
              testing::Not(testing::HasSubstr("[keywords:")));

  // Current format should be "title | host"
  EXPECT_THAT(title_only_result,
              testing::ContainsRegex(".* \\| ai-tutorial\\.com"));
}

// Test ScoreGroupKNN function directly
TEST_F(TextEmbedderUnitTest, ScoreGroupKNNBasicFunctionality) {
  // Create test embeddings by generating them from text
  SetTabs(
      embedder_.get(),
      {"Travel destinations in Europe", "Visit Italy for amazing experiences",
       "France travel guide recommendations", "Stock market analysis report"});
  EXPECT_TRUE(EmbedTabs(embedder_.get()).ok());
  EXPECT_EQ(embeddings_size(), 4u);

  // Get candidate embedding (travel-related)
  auto candidate = GetEmbedding(0);  // "Travel destinations in Europe"

  // Create group with similar travel content
  std::vector<tflite::task::processor::EmbeddingResult> travel_group = {
      GetEmbedding(1),  // "Visit Italy for amazing experiences"
      GetEmbedding(2)   // "France travel guide recommendations"
  };

  // Create group with different content
  std::vector<tflite::task::processor::EmbeddingResult> finance_group = {
      GetEmbedding(3)  // "Stock market analysis report"
  };

  // Test KNN scoring
  double travel_score = CallScoreGroupKNN(candidate, travel_group);
  double finance_score = CallScoreGroupKNN(candidate, finance_group);

  // Travel group should have higher similarity score than finance group
  EXPECT_GT(travel_score, finance_score);
  EXPECT_GT(travel_score, 0.0);   // Should be positive similarity
  EXPECT_GT(finance_score, 0.0);  // Should still be positive but lower

  // Test empty group
  std::vector<tflite::task::processor::EmbeddingResult> empty_group;
  double empty_score = CallScoreGroupKNN(candidate, empty_group);
  EXPECT_EQ(empty_score, -1.0);  // Should return -1.0 for empty groups
}

// Test KNN method vs Centroid method comparison
TEST_F(TextEmbedderUnitTest, ClusteringMethodComparison) {
  // Create group tabs (travel-related content)
  std::vector<local_ai::TextEmbedder::TabInfo> group_tabs = {
      {u"Top travel destinations in Europe", GURL("https://travelblog.com"), "",
       ""},
      {u"Visit Italy for amazing cultural experiences",
       GURL("https://lonelyplanet.com"), "", ""},
      {u"France travel guide and recommendations",
       GURL("https://frenchguide.com"), "", ""}};

  // Create candidate tabs with varying similarity
  std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs = {
      {0,
       {u"Stock market analysis and trading tips", GURL("https://finance.com"),
        "", ""}},  // Low similarity
      {1,
       {u"Amazing travel experiences in Spain",
        GURL("https://spanishtravel.com"), "", ""}},  // High similarity
      {2,
       {u"Weather forecast for tomorrow", GURL("https://weather.com"), "",
        ""}},  // Low similarity
      {3,
       {u"Best European cities to visit this summer",
        GURL("https://euroguide.com"), "", ""}}  // High similarity
  };

  // Test with Centroid method
  auto centroid_result = VerifySuggestTabsForGroupWithMethod(
      group_tabs, candidate_tabs, local_ai::ClusteringMethod::kCentroid);
  ASSERT_TRUE(centroid_result.ok())
      << "Centroid method failed: " << centroid_result.status().message();

  // Test with KNN method
  auto knn_result = VerifySuggestTabsForGroupWithMethod(
      std::move(group_tabs), std::move(candidate_tabs),
      local_ai::ClusteringMethod::kKNN);
  ASSERT_TRUE(knn_result.ok())
      << "KNN method failed: " << knn_result.status().message();

  // Both methods should return some suggestions
  EXPECT_FALSE(centroid_result.value().empty());
  EXPECT_FALSE(knn_result.value().empty());

  // Both should identify the travel-related tabs (indices 1 and 3) as good
  // candidates
  const auto& centroid_indices = centroid_result.value();
  const auto& knn_indices = knn_result.value();

  // Verify that travel-related candidates are suggested by both methods
  bool centroid_found_travel =
      std::find(centroid_indices.begin(), centroid_indices.end(), 1) !=
          centroid_indices.end() ||
      std::find(centroid_indices.begin(), centroid_indices.end(), 3) !=
          centroid_indices.end();
  bool knn_found_travel =
      std::find(knn_indices.begin(), knn_indices.end(), 1) !=
          knn_indices.end() ||
      std::find(knn_indices.begin(), knn_indices.end(), 3) != knn_indices.end();

  EXPECT_TRUE(centroid_found_travel)
      << "Centroid method should find travel-related candidates";
  EXPECT_TRUE(knn_found_travel)
      << "KNN method should find travel-related candidates";
}

// Test SuggestGroupForTab with both clustering methods
TEST_F(TextEmbedderUnitTest, SuggestGroupForTabClusteringMethods) {
  // Create a travel-related candidate
  local_ai::TextEmbedder::CandidateTab candidate_tab = {
      10,
      {u"Best places to visit in Italy this summer",
       GURL("https://italyguide.com"), "", ""}};

  // Create groups with different themes
  tab_groups::TabGroupId travel_group_id = CreateMockTabGroupId();
  tab_groups::TabGroupId finance_group_id = CreateMockTabGroupId();
  tab_groups::TabGroupId weather_group_id = CreateMockTabGroupId();

  std::map<tab_groups::TabGroupId, std::vector<local_ai::TextEmbedder::TabInfo>>
      group_tabs;

  // Travel group - should be the best match
  group_tabs[travel_group_id] = {{u"Top European travel destinations",
                                  GURL("https://europeguide.com"), "", ""},
                                 {u"France travel guide and tips",
                                  GURL("https://francetravel.com"), "", ""}};

  // Finance group - should be low match
  group_tabs[finance_group_id] = {
      {u"Stock market trading strategies", GURL("https://trading.com"), "", ""},
      {u"Investment portfolio management", GURL("https://invest.com"), "", ""}};

  // Weather group - should be low match
  group_tabs[weather_group_id] = {
      {u"Daily weather forecast", GURL("https://weather.com"), "", ""},
      {u"Climate change reports", GURL("https://climate.org"), "", ""}};

  // Test with Centroid method
  auto centroid_result = VerifySuggestGroupForTabWithMethod(
      candidate_tab, group_tabs, local_ai::ClusteringMethod::kCentroid);
  EXPECT_TRUE(centroid_result.ok())
      << "Centroid method failed: " << centroid_result.status().message();

  // Test with KNN method
  auto knn_result = VerifySuggestGroupForTabWithMethod(
      std::move(candidate_tab), std::move(group_tabs),
      local_ai::ClusteringMethod::kKNN);
  EXPECT_TRUE(knn_result.ok())
      << "KNN method failed: " << knn_result.status().message();

  // Both methods should return valid group IDs
  if (centroid_result.ok()) {
    EXPECT_FALSE(centroid_result.value().is_empty());
  }
  if (knn_result.ok()) {
    EXPECT_FALSE(knn_result.value().is_empty());
  }
}

// Test default clustering method behavior
TEST_F(TextEmbedderUnitTest, DefaultClusteringMethod) {
  // Test that default behavior uses Centroid method
  std::vector<local_ai::TextEmbedder::TabInfo> group_tabs = {
      {u"European travel destinations", GURL("https://travel.com"), "", ""}};
  std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs = {
      {0, {u"Italy travel guide", GURL("https://italy.com"), "", ""}}};

  // Call without explicit method (should use default kCentroid)
  auto default_result = VerifySuggestTabsForGroup(group_tabs, candidate_tabs);

  // Call with explicit kCentroid method
  auto explicit_result = VerifySuggestTabsForGroupWithMethod(
      std::move(group_tabs), std::move(candidate_tabs),
      local_ai::ClusteringMethod::kCentroid);

  // Both should work identically
  EXPECT_EQ(default_result.ok(), explicit_result.ok());
  if (default_result.ok() && explicit_result.ok()) {
    EXPECT_EQ(default_result.value().size(), explicit_result.value().size());
  }
}

// Test KNN with various group sizes
TEST_F(TextEmbedderUnitTest, KNNWithVariousGroupSizes) {
  // Test with group size smaller than MAX_NN_GROUPED_TABS (4)
  SetTabs(embedder_.get(), {
                               "Travel in Europe", "Visit Italy",
                               "France guide"  // Only 2 items in group
                           });
  EXPECT_TRUE(EmbedTabs(embedder_.get()).ok());

  auto candidate = GetEmbedding(0);
  std::vector<tflite::task::processor::EmbeddingResult> small_group = {
      GetEmbedding(1), GetEmbedding(2)};

  double small_score = CallScoreGroupKNN(candidate, small_group);
  EXPECT_GT(small_score, 0.0);
  EXPECT_LT(small_score, 1.0);  // Should be normalized similarity

  // Test with group size larger than MAX_NN_GROUPED_TABS (4)
  SetTabs(embedder_.get(),
          {
              "Travel candidate", "Europe travel 1", "Europe travel 2",
              "Europe travel 3", "Europe travel 4", "Europe travel 5",
              "Europe travel 6"  // 6 items in group
          });
  EXPECT_TRUE(EmbedTabs(embedder_.get()).ok());

  candidate = GetEmbedding(0);
  std::vector<tflite::task::processor::EmbeddingResult> large_group;
  for (size_t i = 1; i < embeddings_size(); ++i) {
    large_group.push_back(GetEmbedding(i));
  }

  double large_score = CallScoreGroupKNN(candidate, large_group);
  EXPECT_GT(large_score, 0.0);
  EXPECT_LT(large_score, 1.0);

  // The score should be reasonable (KNN should use top 4 out of 6)
  EXPECT_GT(large_score, 0.3);  // Should have decent similarity
}

}  // namespace local_ai
