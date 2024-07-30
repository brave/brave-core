// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/suggestions_controller.h"

#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/background_history_querier.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-shared.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/url_row.h"
#include "content/public/test/browser_task_environment.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {
namespace {
Publishers MakePublishers(const std::vector<std::string>& publisher_urls) {
  Publishers result;
  size_t next_id = 1;
  for (const auto& url : publisher_urls) {
    auto publisher = mojom::Publisher::New();
    publisher->locales.push_back(
        mojom::LocaleInfo::New("en_US", 0, std::vector<std::string>{}));
    publisher->site_url = GURL(url);
    publisher->user_enabled_status = mojom::UserEnabled::NOT_MODIFIED;
    result[base::NumberToString(next_id++)] = std::move(publisher);
  }
  return result;
}

history::QueryResults MakeQueryResults(const std::vector<std::string>& urls) {
  history::QueryResults result;
  std::vector<history::URLResult> url_results;
  for (const auto& url : urls) {
    url_results.emplace_back(history::URLResult(GURL(url), base::Time::Now()));
  }

  result.SetURLResults(std::move(url_results));
  return result;
}
}  // namespace

class BraveNewsSuggestionsControllerTest : public testing::Test {
 public:
  BraveNewsSuggestionsControllerTest()
      : api_request_helper_(TRAFFIC_ANNOTATION_FOR_TESTS,
                            test_url_loader_factory_.GetSafeWeakWrapper()),
        publishers_controller_(&api_request_helper_),
        suggestions_controller_(&publishers_controller_,
                                &api_request_helper_,
                                querier_) {
    profile_.GetPrefs()->SetBoolean(brave_news::prefs::kBraveNewsOptedIn, true);
    profile_.GetPrefs()->SetBoolean(brave_news::prefs::kNewTabPageShowToday,
                                    true);
    SetLocale("en_US");
  }
  ~BraveNewsSuggestionsControllerTest() override = default;

 protected:
  std::vector<std::string> GetSuggestedPublisherIds(
      const Publishers& publishers,
      const history::QueryResults& history) {
    return suggestions_controller_.GetSuggestedPublisherIdsWithHistory(
        publishers, history);
  }

  void SetSimilarityMatrix(
      SuggestionsController::PublisherSimilarities similarities) {
    suggestions_controller_.similarities_ = std::move(similarities);
  }

  void SetLocale(const std::string& locale) {
    suggestions_controller_.locale_ = locale;
  }

  content::BrowserTaskEnvironment browser_task_environment_;
  TestingProfile profile_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  api_request_helper::APIRequestHelper api_request_helper_;

  BackgroundHistoryQuerier querier_ = base::DoNothing();
  PublishersController publishers_controller_;
  SuggestionsController suggestions_controller_;
};

TEST_F(BraveNewsSuggestionsControllerTest, VisitedSourcesAreSuggested) {
  const auto& publishers = MakePublishers({
      "https://example.com",
      "https://bar.com",
      "https://foo.com",
  });
  const auto& history = MakeQueryResults(
      {"https://example.com", "https://foo.com", "https://example.com"});
  auto suggestions = GetSuggestedPublisherIds(publishers, history);

  // Publisher 1 & publisher 3 have been visited. However, P1 was visited more
  // times, so we should suggest it first.
  EXPECT_EQ(2u, suggestions.size());
  EXPECT_EQ("1", suggestions[0]);
  EXPECT_EQ("3", suggestions[1]);
}

TEST_F(BraveNewsSuggestionsControllerTest,
       SubscribedVisitedSourcesAreNotSuggested) {
  const auto& publishers = MakePublishers({
      "https://example.com",
      "https://bar.com",
      "https://foo.com",
  });
  publishers.at("1")->user_enabled_status = mojom::UserEnabled::ENABLED;
  const auto& history = MakeQueryResults(
      {"https://example.com", "https://foo.com", "https://example.com"});
  auto suggestions = GetSuggestedPublisherIds(publishers, history);

  // Publisher 1 is subscribed, so we shouldn't suggest it. However, we've
  // visited publisher 3, so we should suggest that.
  EXPECT_EQ(1u, suggestions.size());
  EXPECT_EQ("3", suggestions[0]);
}

TEST_F(BraveNewsSuggestionsControllerTest,
       DisabledVisitedSourcesAreNotSuggested) {
  const auto& publishers = MakePublishers({
      "https://example.com",
      "https://bar.com",
      "https://foo.com",
  });
  publishers.at("1")->user_enabled_status = mojom::UserEnabled::DISABLED;
  const auto& history = MakeQueryResults(
      {"https://example.com", "https://foo.com", "https://example.com"});
  auto suggestions = GetSuggestedPublisherIds(publishers, history);

  // P1 was disabled, so we shouldn't suggest it. P3 was visited so it should be
  // suggested.
  EXPECT_EQ(1u, suggestions.size());
  EXPECT_EQ("3", suggestions[0]);
}

TEST_F(BraveNewsSuggestionsControllerTest, SimilarSourcesAreSuggested) {
  const auto& publishers = MakePublishers({
      "https://example.com",
      "https://bar.com",
      "https://foo.com",
      "https://frob.com",
  });

  publishers.at("1")->user_enabled_status = mojom::UserEnabled::ENABLED;
  SetSimilarityMatrix({{"1",
                        {{.publisher_id = "2", .score = 0.8},
                         {.publisher_id = "4", .score = 0.9}}}});

  auto suggestions = GetSuggestedPublisherIds(publishers, {});

  // P1 is enabled so we should suggest sources similar to it. P4 is more
  // similar to it than P2, so we should suggest it first.
  EXPECT_EQ(2u, suggestions.size());
  EXPECT_EQ("4", suggestions[0]);
  EXPECT_EQ("2", suggestions[1]);
}

TEST_F(BraveNewsSuggestionsControllerTest,
       SimilarToVisitedSourcesAreSuggested) {
  const auto& publishers = MakePublishers({
      "https://example.com",
      "https://bar.com",
      "https://foo.com",
      "https://frob.com",
  });

  auto history = MakeQueryResults({"https://example.com"});
  SetSimilarityMatrix({{"1",
                        {{.publisher_id = "2", .score = 0.8},
                         {.publisher_id = "4", .score = 0.9}}}});

  auto suggestions = GetSuggestedPublisherIds(publishers, history);

  // P1 has been visited and is not subscribed, so we should suggest it first.
  // P4 and P2 are similar to P1 so they should be suggested too (P4 is more
  // similar to P1, so suggest it first.)
  EXPECT_EQ(3u, suggestions.size());
  EXPECT_EQ("1", suggestions[0]);
  EXPECT_EQ("4", suggestions[1]);
  EXPECT_EQ("2", suggestions[2]);
}

TEST_F(BraveNewsSuggestionsControllerTest,
       VisitWeightingAltersSimilarToVisitWeighting) {
  const auto& publishers = MakePublishers({
      "https://example.com",
      "https://bar.com",
      "https://foo.com",
      "https://frob.com",
  });

  auto history = MakeQueryResults({"https://example.com", "https://example.com",
                                   "https://example.com", "https://bar.com"});
  SetSimilarityMatrix({{"1", {{.publisher_id = "3", .score = 0.3}}},
                       {"2", {{.publisher_id = "4", .score = 0.4}}}});

  auto suggestions = GetSuggestedPublisherIds(publishers, history);

  // P1 has been visited many times, so sources similar to it should be ranked
  // higher than sources similar to P2.
  EXPECT_EQ(4u, suggestions.size());
  EXPECT_EQ("1", suggestions[0]);  // Visited many times
  EXPECT_EQ("2", suggestions[1]);  // Visited, but just once
  EXPECT_EQ("3",
            suggestions[2]);  // Similar to P1 (which was visited many times).
  EXPECT_EQ("4", suggestions[3]);  // Similar to P2 (which was visited once)
}

TEST_F(BraveNewsSuggestionsControllerTest,
       SuggestionsCanComeFromVistSimilarOrSimilarToVisit) {
  const auto& publishers = MakePublishers({
      "https://visited.com",
      "https://similar-to-visited.com",
      "https://subscribed.com",
      "https://similar-to-subscribed.com",
      "https://unrelated.com",
  });

  publishers.at("3")->user_enabled_status = mojom::UserEnabled::ENABLED;

  auto history = MakeQueryResults({"https://visited.com"});
  SetSimilarityMatrix({{"1", {{.publisher_id = "2", .score = 0.8}}},
                       {"3", {{.publisher_id = "4", .score = 0.8}}}});

  auto suggestions = GetSuggestedPublisherIds(publishers, history);
  EXPECT_EQ(3u, suggestions.size());
  // Note: Don't care about order here - we're going to be tweaking the weights
  // and we don't want the test to fail all the time.
  EXPECT_TRUE(base::Contains(suggestions, "1"));  // Visited
  EXPECT_TRUE(
      base::Contains(suggestions, "2"));  // Similar to P3 (which is subscribed)
  EXPECT_TRUE(
      base::Contains(suggestions, "4"));  // Similar to P1 (which was visited)
}

TEST_F(BraveNewsSuggestionsControllerTest,
       SourcesFromDifferentLocalesAreNotSuggested) {
  const auto& publishers = MakePublishers({
      "https://visited.com",
      "https://similar-to-visited.com",
      "https://subscribed.com",
      "https://similar-to-subscribed.com",
      "https://unrelated.com",
  });

  publishers.at("3")->user_enabled_status = mojom::UserEnabled::ENABLED;

  auto history = MakeQueryResults({"https://visited.com"});
  SetSimilarityMatrix({{"1", {{.publisher_id = "2", .score = 0.8}}},
                       {"3", {{.publisher_id = "4", .score = 0.8}}}});

  SetLocale("en_NZ");
  auto suggestions = GetSuggestedPublisherIds(publishers, history);
  EXPECT_EQ(0u, suggestions.size());
}

}  // namespace brave_news
