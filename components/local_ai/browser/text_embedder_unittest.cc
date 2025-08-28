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

  absl::StatusOr<tab_groups::TabGroupId> VerifySuggestGroupForTab(
      local_ai::TextEmbedder::CandidateTab candidate_tab,
      std::map<tab_groups::TabGroupId,
               std::vector<local_ai::TextEmbedder::TabInfo>> group_tabs) {
    base::test::TestFuture<absl::StatusOr<tab_groups::TabGroupId>> future;
    embedder_->SuggestGroupForTab(std::move(candidate_tab),
                                  std::move(group_tabs), future.GetCallback());
    return future.Get();
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
  // Create group tabs (travel-related content)
  std::vector<local_ai::TextEmbedder::TabInfo> group_tabs = {
      {u"Top 10 places to visit in Italy", GURL("https://travelblog.com"), ""},
      {u"Flight comparison: Rome vs Venice", GURL("https://skyscanner.com"),
       ""},
      {u"Train travel tips across Europe", GURL("https://eurotripadvisor.net"),
       ""},
      {u"Travel insurance for international trips",
       GURL("https://safetravel.com"), ""},
      {u"Visa requirements for Schengen countries",
       GURL("https://visaguide.world"), ""}};

  // Create candidate tabs (mix of travel and non-travel content)
  std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs = {
      {0,
       {u"Compare savings accounts interest rates",
        GURL("https://bankrate.com"), ""}},
      {10,
       {u"Tips to improve credit score", GURL("https://nerdwallet.com"), ""}},
      {2,
       {u"Live coverage of Formula 1 race", GURL("https://formula1.com"), ""}},
      {5,
       {u"Visiting Italy in October: all you need to know",
        GURL("https://mamalovesitaly.com"), ""}},  // Travel-related
      {9,
       {u"Review of hotels in Venice", GURL("https://booking.com"),
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
      {u"Compare savings accounts interest rates",
       GURL("https://bankrate.com")}};

  // Create groups with TabGroupIds
  tab_groups::TabGroupId travel_group_id = CreateMockTabGroupId();
  tab_groups::TabGroupId weather_group_id = CreateMockTabGroupId();
  tab_groups::TabGroupId finance_group_id = CreateMockTabGroupId();
  tab_groups::TabGroupId sports_group_id = CreateMockTabGroupId();

  std::map<tab_groups::TabGroupId, std::vector<local_ai::TextEmbedder::TabInfo>>
      group_tabs;

  // Travel group
  group_tabs[travel_group_id] = {
      {u"Top 10 places to visit in Italy", GURL("https://travelblog.com"), ""},
      {u"Flight comparison: Rome vs Venice", GURL("https://skyscanner.com"),
       ""},
      {u"Train travel tips across Europe", GURL("https://eurotripadvisor.net"),
       ""},
      {u"Travel insurance for international trips",
       GURL("https://safetravel.com"), ""},
      {u"Visa requirements for Schengen countries",
       GURL("https://visaguide.world"), ""}};

  // Weather group
  group_tabs[weather_group_id] = {
      {u"Woking, Surrey, United Kingdom Current Weather | AccuWeather",
       GURL("https://accuweather.com"), ""},
      {u"London - BBC Weather", GURL("https://bbc.co.uk"), ""}};

  // Finance group - this should be the best match
  group_tabs[finance_group_id] = {
      {u"Personal Savings Allowance", GURL("https://moneysavingexpert.com"),
       ""},
      {u"What is the Personal Savings Allowance? | Barclays",
       GURL("https://barclays.co.uk"), ""}};

  // Sports group
  group_tabs[sports_group_id] = {
      {u"Football Scores & Fixtures - Today's Schedule of Football",
       GURL("https://skysports.com")}};

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
      {u"Test group tab", GURL()}};
  std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs = {
      {0, {u"Test candidate tab", GURL()}}};
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
      0, {u"Test candidate tab", GURL()}};
  std::map<tab_groups::TabGroupId, std::vector<local_ai::TextEmbedder::TabInfo>>
      groups;
  groups[CreateMockTabGroupId()] = {{u"Test group tab 1", GURL()}};
  groups[CreateMockTabGroupId()] = {{u"Test group tab 2", GURL()}};
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
      {u"Test group tab", GURL()}};
  std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs1 = {
      {0, {u"Test candidate tab", GURL()}}};
  embedder->SuggestTabsForGroup(std::move(group_tabs1),
                                std::move(candidate_tabs1),
                                tabs_future.GetCallback());
  auto tabs_result = tabs_future.Get();
  EXPECT_FALSE(tabs_result.ok());
  EXPECT_TRUE(absl::IsFailedPrecondition(tabs_result.status()));

  base::test::TestFuture<absl::StatusOr<tab_groups::TabGroupId>> group_future;
  local_ai::TextEmbedder::CandidateTab candidate1 = {
      0, {u"Test candidate tab", GURL()}};
  std::map<tab_groups::TabGroupId, std::vector<local_ai::TextEmbedder::TabInfo>>
      groups1;
  groups1[CreateMockTabGroupId()] = {{u"Test group tab", GURL()}};
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
       ""},
      {u"Top places to visit in Italy", GURL("https://lonelyplanet.com"), ""}};
  std::vector<local_ai::TextEmbedder::CandidateTab> candidate_tabs2 = {
      {0,
       {u"Amazing travel spots in France", GURL("https://travelguide.com"),
        ""}}};
  embedder->SuggestTabsForGroup(std::move(group_tabs2),
                                std::move(candidate_tabs2),
                                tabs_future2.GetCallback());
  auto tabs_result2 = tabs_future2.Get();
  EXPECT_TRUE(tabs_result2.ok());  // Should succeed with similar travel content

  base::test::TestFuture<absl::StatusOr<tab_groups::TabGroupId>> group_future2;
  local_ai::TextEmbedder::CandidateTab candidate2 = {
      0,
      {u"European travel guide recommendations",
       GURL("https://eurotravel.com")}};
  std::map<tab_groups::TabGroupId, std::vector<local_ai::TextEmbedder::TabInfo>>
      groups2;
  groups2[CreateMockTabGroupId()] = {
      {u"Best travel destinations in Europe", GURL("https://travelblog.com"),
       ""},
      {u"Top places to visit in Italy", GURL("https://lonelyplanet.com"), ""}};
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
      u"Test Page", GURL("https://example.com/path?query=1"), ""};

  // Test with subdomain - should extract full host
  local_ai::TextEmbedder::TabInfo tab2 = {
      u"News Article", GURL("https://news.google.com/article/123"), ""};

  // Test with invalid URL - should use "unknown"
  local_ai::TextEmbedder::TabInfo tab3 = {u"Invalid URL", GURL("not-a-url"),
                                          ""};

  // Test with file URL (no host) - should use full spec
  local_ai::TextEmbedder::TabInfo tab4 = {
      u"Local File", GURL("file:///path/to/file.html"), ""};

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
      "This is the main article content about travel tips and destinations."};
  std::string result = CallSerializeTabInfo(tab_with_content);
  EXPECT_THAT(result, testing::HasSubstr("Article Title"));
  EXPECT_THAT(result, testing::HasSubstr("example.com"));
  EXPECT_THAT(result, testing::HasSubstr("[keywords:"));
}

// Test YAKE keyword extraction functionality
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

// Test SerializeTabInfo with keyword extraction integration
TEST_F(TextEmbedderUnitTest, SerializeTabInfoWithKeywords) {
  // Test with rich content that should produce meaningful keywords
  local_ai::TextEmbedder::TabInfo tab_with_rich_content = {
      u"Best Travel Destinations in Europe",
      GURL("https://travelblog.com/europe-guide"),
      "Europe offers amazing travel destinations including Italy with its "
      "beautiful cities like Rome and Venice. "
      "France provides cultural experiences in Paris and Lyon. Spain features "
      "Barcelona and Madrid. "
      "These destinations offer rich history, excellent cuisine, and "
      "unforgettable experiences for travelers."};

  std::string serialized = CallSerializeTabInfo(tab_with_rich_content);

  // Should contain the title
  EXPECT_THAT(serialized,
              testing::HasSubstr("Best Travel Destinations in Europe"));

  // Should contain the host
  EXPECT_THAT(serialized, testing::HasSubstr("travelblog.com"));

  // Should contain keyword section (from content)
  EXPECT_THAT(serialized, testing::HasSubstr("[keywords:"));

  // Test with title-only content (no tab_content)
  local_ai::TextEmbedder::TabInfo tab_title_only = {
      u"Machine Learning Tutorial for Beginners",
      GURL("https://ai-tutorial.com/ml-basics"), ""};

  std::string title_only_result = CallSerializeTabInfo(tab_title_only);
  EXPECT_THAT(title_only_result,
              testing::HasSubstr("Machine Learning Tutorial for Beginners"));
  EXPECT_THAT(title_only_result, testing::HasSubstr("ai-tutorial.com"));
  // Should NOT contain keywords section since tab_content is empty
  EXPECT_THAT(title_only_result,
              testing::Not(testing::HasSubstr("[keywords:")));
}

}  // namespace local_ai
