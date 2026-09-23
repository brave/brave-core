/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/navigation_tracker/navigation_tracker.h"

#include <memory>
#include <optional>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_classifier.h"
#include "brave/components/serp_metrics/serp_classifier_utils.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/serp_metrics/test/fake_serp_metrics_time_period_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/regional_capabilities/regional_capabilities_utils.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// `SerpMetricsNavigationTracker` sees only a URL and whether the navigation was
// a new navigation. Platform tab helpers filter everything else before calling
// it: navigations outside the primary main frame, navigations that never
// committed (failed or cancelled), and navigations made while the user has
// opted out of usage pings. Those are covered by the desktop browser tests and
// the iOS tab helper unit tests.

namespace serp_metrics {

namespace {

constexpr char kBraveSerpUrl[] = "https://search.brave.com/search?q=foobar";
constexpr char kAskBraveSerpUrl[] = "https://search.brave.com/ask?q=foobar";
constexpr char kGoogleSerpUrl[] = "https://www.google.com/search?q=foobar";
constexpr char kDuckDuckGoSerpUrl[] = "https://duckduckgo.com/?q=foobar";
constexpr char kBingSerpUrl[] = "https://www.bing.com/search?q=foobar";
constexpr char kStartpageSerpUrl[] = "https://www.startpage.com/sp/search";
constexpr char kNonSerpUrl[] = "https://brave.com/";

SerpMetricType ToSerpMetricType(SearchEngineType search_engine_type) {
  switch (search_engine_type) {
    case SEARCH_ENGINE_BRAVE:
      return SerpMetricType::kBrave;
    case SEARCH_ENGINE_GOOGLE:
      return SerpMetricType::kGoogle;
    default:
      return SerpMetricType::kOther;
  }
}

}  // namespace

class SerpMetricsNavigationTrackerTest : public testing::Test {
 public:
  SerpMetricsNavigationTrackerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    // Register `kLastCheckYMD` pref (YYYY-MM-DD). This pref is part of the
    // daily usage ping and tracks the last reported day so we don't re-report
    // previously sent metrics.
    local_state_.registry()->RegisterStringPref(kLastCheckYMD,
                                                "");  // Never checked.
    local_state_.registry()->RegisterTimePref(prefs::kLastReportedAt,
                                              /* Never reported */
                                              base::Time());

    scoped_feature_list_.InitAndEnableFeature(kSerpMetricsFeature);

    Reset();
  }

  // Discards the tracker's recorded state, as if the user had opened a new tab.
  void Reset() {
    serp_metrics_ = std::make_unique<SerpMetrics>(
        &local_state_, test::FakeSerpMetricsTimePeriodStoreFactory());
    tracker_ = std::make_unique<SerpMetricsNavigationTracker>(*serp_metrics_);
  }

  void Navigate(std::string_view url, bool is_new_navigation = true) {
    tracker_->OnNavigationFinished(GURL(url), is_new_navigation);
  }

  size_t GetSearchCount(SerpMetricType type) const {
    return serp_metrics_->GetSearchCountForTesting(type);
  }

  size_t GetTotalSearchCount() const {
    return GetSearchCount(SerpMetricType::kBrave) +
           GetSearchCount(SerpMetricType::kGoogle) +
           GetSearchCount(SerpMetricType::kOther);
  }

  // Asserts that exactly `brave`, `google` and `other` searches were recorded.
  void ExpectSearchCounts(size_t brave, size_t google, size_t other) const {
    EXPECT_EQ(brave, GetSearchCount(SerpMetricType::kBrave));
    EXPECT_EQ(google, GetSearchCount(SerpMetricType::kGoogle));
    EXPECT_EQ(other, GetSearchCount(SerpMetricType::kOther));
  }

 protected:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedFeatureList scoped_feature_list_;

  TestingPrefServiceSimple local_state_;

  std::unique_ptr<SerpMetrics> serp_metrics_;
  std::unique_ptr<SerpMetricsNavigationTracker> tracker_;
};

///////////////////////////////////////////////////////////////////////////////
// Search Engine Classification

TEST_F(SerpMetricsNavigationTrackerTest, RecordBraveSearch) {
  Navigate(kBraveSerpUrl);

  ExpectSearchCounts(/*brave=*/1, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordAskBraveSearchAsBraveSearch) {
  Navigate(kAskBraveSerpUrl);

  ExpectSearchCounts(/*brave=*/1, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordAskBraveSearchWithoutSearchTerms) {
  Navigate(R"(https://search.brave.com/ask)");

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordAskPathOnAnotherHost) {
  Navigate(R"(https://foo.com/ask?q=bar)");

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordGoogleWebSearch) {
  Navigate(kGoogleSerpUrl);

  ExpectSearchCounts(/*brave=*/0, /*google=*/1, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordOtherSearchEngine) {
  Navigate(kDuckDuckGoSerpUrl);

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/1);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordBingAsOtherSearchEngine) {
  Navigate(kBingSerpUrl);

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/1);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordStartpageAsOtherSearchEngine) {
  // Startpage uses a path-based SERP URL that Chromium's query-based detection
  // does not support, so it is matched by host and path instead.
  Navigate(kStartpageSerpUrl);

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/1);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordSerpsForDifferentEngines) {
  Navigate(kBraveSerpUrl);
  Navigate(kGoogleSerpUrl);
  Navigate(kDuckDuckGoSerpUrl);

  ExpectSearchCounts(/*brave=*/1, /*google=*/1, /*other=*/1);
}

TEST_F(SerpMetricsNavigationTrackerTest, OnlyRecordAllowedSearchEngines) {
  const auto verify_prepopulated_engine =
      [&](const TemplateURLPrepopulateData::PrepopulatedEngine&
              prepopulated_engine) {
        const auto template_url_data =
            TemplateURLDataFromPrepopulatedEngine(prepopulated_engine);
        const TemplateURL template_url(*template_url_data);
        const GURL url =
            template_url.GenerateSearchURL(SearchTermsData(), u"test");
        ASSERT_TRUE(url.is_valid());

        Reset();
        Navigate(url.spec());

        const std::optional<SearchEngineType> search_engine_type =
            MaybeClassifySearchEngine(url);
        if (!search_engine_type) {
          EXPECT_EQ(0U, GetTotalSearchCount()) << url;
          return;
        }

        // Anything the tracker records must be an allowed search engine.
        EXPECT_TRUE(IsAllowedSearchEngine(*search_engine_type)) << url;
        EXPECT_EQ(1U, GetTotalSearchCount()) << url;
        EXPECT_EQ(1U, GetSearchCount(ToSerpMetricType(*search_engine_type)))
            << url;
      };

  for (const TemplateURLPrepopulateData::PrepopulatedEngine*
           prepopulated_engine :
       regional_capabilities::GetAllPrepopulatedEngines()) {
    verify_prepopulated_engine(*prepopulated_engine);
  }

  for (const auto& [_, prepopulated_engine] :
       TemplateURLPrepopulateData::kBraveEngines) {
    verify_prepopulated_engine(*prepopulated_engine);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Google vertical searches. Images, video, news, shopping and books are not
// web searches and must not count toward `kGoogle`

TEST_F(SerpMetricsNavigationTrackerTest, RecordGoogleWebSearchWithUdmZero) {
  // `udm=0` is the Web tab with AI Overviews.
  Navigate(R"(https://www.google.com/search?q=foobar&udm=0)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/1, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordGoogleWebSearchWithUdmFourteen) {
  // `udm=14` is the Web tab without AI Overviews.
  Navigate(R"(https://www.google.com/search?q=foobar&udm=14)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/1, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordGoogleWebSearchWithUdmWeb) {
  Navigate(R"(https://www.google.com/search?q=foobar&udm=web)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/1, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordGoogleImagesWithTbm) {
  Navigate(R"(https://www.google.com/search?q=foobar&tbm=isch)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordGoogleNewsWithTbm) {
  Navigate(R"(https://www.google.com/search?q=foobar&tbm=nws)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordGoogleVideoWithTbm) {
  Navigate(R"(https://www.google.com/search?q=foobar&tbm=vid)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordGoogleShoppingWithTbm) {
  Navigate(R"(https://www.google.com/search?q=foobar&tbm=shop)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordGoogleBooksWithTbm) {
  Navigate(R"(https://www.google.com/search?q=foobar&tbm=bks)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordGoogleImagesWithUdm) {
  Navigate(R"(https://www.google.com/search?q=foobar&udm=2)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordGoogleVideoWithUdm) {
  Navigate(R"(https://www.google.com/search?q=foobar&udm=7)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordGoogleNewsWithUdm) {
  Navigate(R"(https://www.google.com/search?q=foobar&udm=12)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordGoogleForumsWithUdm) {
  Navigate(R"(https://www.google.com/search?q=foobar&udm=18)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordGoogleShoppingWithUdm) {
  Navigate(R"(https://www.google.com/search?q=foobar&udm=28)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordGoogleShortVideosWithUdm) {
  Navigate(R"(https://www.google.com/search?q=foobar&udm=39)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordGoogleVerticalSearchWhenTbmOverridesWebUdm) {
  // `tbm` routes to a vertical search even alongside a web `udm`.
  Navigate(R"(https://www.google.com/search?q=foobar&tbm=vid&udm=14)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       RecordGoogleWebSearchAfterVerticalSearchForSameQuery) {
  // A rejected vertical search must not suppress the web search that follows
  // it, even for the same query.
  Navigate(R"(https://www.google.com/search?q=foobar&udm=2)");
  ASSERT_EQ(0U, GetTotalSearchCount());

  Navigate(kGoogleSerpUrl);
  ExpectSearchCounts(/*brave=*/0, /*google=*/1, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordVerticalSearchOnAnotherGoogleDomain) {
  Navigate(R"(https://www.google.co.uk/search?q=foobar&tbm=isch)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       RecordVerticalParamsForNonGoogleSearchEngine) {
  // `tbm` and `udm` are Google-specific and must not exclude other engines.
  Navigate(R"(https://duckduckgo.com/?q=foobar&tbm=isch)");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/1);
}

///////////////////////////////////////////////////////////////////////////////
// Non-SERP and malformed URLs.

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordNonSerp) {
  Navigate(kNonSerpUrl);
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordNonSearchEngine) {
  Navigate(R"(https://www.perplexity.ai/search/new/foo)");
  Navigate(R"(https://bar.com/baz)");
  Navigate(R"(https://qux.quux.com/corge)");

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordNonSerpPathsOnSearchEngineHosts) {
  // The right host is not enough; the path has to be a SERP.
  Navigate(R"(https://startpage.com/grault)");
  Navigate(R"(https://uk.search.yahoo.com/garply)");
  Navigate(R"(https://search.yahoo.com/waldo)");

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordSerpWithoutSearchTerms) {
  Navigate(R"(https://search.brave.com/search)");
  Navigate(R"(https://www.google.com/search)");

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordInvalidUrl) {
  Navigate("invalid");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordEmptyUrl) {
  Navigate("");
  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordNonHttpSchemes) {
  Navigate("about:blank");
  Navigate("chrome://settings");
  Navigate(R"(data:text/html,<p>foobar</p>)");

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

///////////////////////////////////////////////////////////////////////////////
// Deduplication of consecutive navigations to the same search results page.

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordConsecutiveNavigationsToSameSerp) {
  Navigate(kBraveSerpUrl);
  Navigate(kBraveSerpUrl);

  ExpectSearchCounts(/*brave=*/1, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordSameSerpWithExtraNonSearchParam) {
  Navigate(kBraveSerpUrl);
  Navigate(R"(https://search.brave.com/search?q=foobar&t=web)");

  ExpectSearchCounts(/*brave=*/1, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordSameSerpWithDifferentParamOrder) {
  Navigate(kBraveSerpUrl);
  Navigate(R"(https://search.brave.com/search?t=web&q=foobar)");

  ExpectSearchCounts(/*brave=*/1, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordPagingThroughSameSearchResults) {
  Navigate(kBraveSerpUrl);
  Navigate(R"(https://search.brave.com/search?q=foobar&page=2)");
  Navigate(R"(https://search.brave.com/search?q=foobar&page=3)");

  ExpectSearchCounts(/*brave=*/1, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordConsecutiveNavigationsToSameAskBraveSearch) {
  Navigate(kAskBraveSerpUrl);
  Navigate(R"(https://search.brave.com/ask?q=foobar&t=web)");

  ExpectSearchCounts(/*brave=*/1, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordSerpForDifferentSearchQuery) {
  Navigate(kBraveSerpUrl);
  Navigate(R"(https://search.brave.com/search?q=qux)");

  ExpectSearchCounts(/*brave=*/2, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       RecordAskBraveSearchForDifferentSearchQuery) {
  Navigate(kAskBraveSerpUrl);
  Navigate(R"(https://search.brave.com/ask?q=qux)");

  ExpectSearchCounts(/*brave=*/2, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordSameSerpAfterIntermediateUrl) {
  Navigate(kBraveSerpUrl);
  Navigate(kNonSerpUrl);
  Navigate(kBraveSerpUrl);

  ExpectSearchCounts(/*brave=*/2, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordSameSerpAfterIntermediateSerp) {
  Navigate(kBraveSerpUrl);
  Navigate(R"(https://search.brave.com/search?q=qux)");
  Navigate(kBraveSerpUrl);

  ExpectSearchCounts(/*brave=*/3, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       RecordSameQueryOnDifferentSearchEngines) {
  // Only the previous SERP is remembered, and the engines differ anyway.
  Navigate(kBraveSerpUrl);
  Navigate(kGoogleSerpUrl);
  Navigate(kBraveSerpUrl);

  ExpectSearchCounts(/*brave=*/2, /*google=*/1, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       AlwaysRecordConsecutiveStartpageSerps) {
  // Startpage's path-based SERP URL carries no search terms, so two Startpage
  // URLs can never be known to be the same search. They are always recorded.
  Navigate(kStartpageSerpUrl);
  Navigate(kStartpageSerpUrl);

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/2);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordConsecutiveNavigationsToSameSerpOnDifferentPorts) {
  // Ports are stripped before comparing, so test servers on random ports still
  // deduplicate.
  Navigate(R"(https://search.brave.com:8443/search?q=foobar)");
  Navigate(R"(https://search.brave.com:9443/search?q=foobar)");

  ExpectSearchCounts(/*brave=*/1, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordConsecutiveNavigationsToSameGoogleSerp) {
  Navigate(kGoogleSerpUrl);
  Navigate(R"(https://www.google.com/search?q=foobar&ie=UTF-8)");

  ExpectSearchCounts(/*brave=*/0, /*google=*/1, /*other=*/0);
}

///////////////////////////////////////////////////////////////////////////////
// Navigations that are not new navigations, i.e. reloads, back/forward
// navigations and session restores. These are recorded: the dedup cache only
// suppresses consecutive *new* navigations to the same SERP.

TEST_F(SerpMetricsNavigationTrackerTest, RecordReloadOfSameSerp) {
  Navigate(kGoogleSerpUrl);
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);

  ExpectSearchCounts(/*brave=*/0, /*google=*/2, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordMultipleReloadsOfSameSerp) {
  Navigate(kGoogleSerpUrl);
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);

  ExpectSearchCounts(/*brave=*/0, /*google=*/3, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordBackNavigationToSerp) {
  Navigate(kGoogleSerpUrl);
  Navigate(kNonSerpUrl);
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);

  ExpectSearchCounts(/*brave=*/0, /*google=*/2, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordNewNavigationToSameSerpAfterReload) {
  // The reload re-records and becomes the last recorded SERP, so the new
  // navigation that follows it is deduplicated.
  Navigate(kGoogleSerpUrl);
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);
  Navigate(kGoogleSerpUrl);

  ExpectSearchCounts(/*brave=*/0, /*google=*/2, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordReloadOfGoogleVerticalSearch) {
  Navigate(R"(https://www.google.com/search?q=foobar&tbm=isch)");
  Navigate(R"(https://www.google.com/search?q=foobar&tbm=isch)",
           /*is_new_navigation=*/false);

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, DoNotRecordReloadOfNonSerp) {
  Navigate(kNonSerpUrl);
  Navigate(kNonSerpUrl, /*is_new_navigation=*/false);

  ExpectSearchCounts(/*brave=*/0, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordRestoredSerp) {
  // A session restore is not a new navigation, and a restored tab gets a fresh
  // tracker with an empty dedup cache, so it is recorded.
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);

  ExpectSearchCounts(/*brave=*/0, /*google=*/1, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordDifferentSerpAfterReload) {
  Navigate(kGoogleSerpUrl);
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);
  Navigate(kBraveSerpUrl);

  ExpectSearchCounts(/*brave=*/1, /*google=*/2, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       RecordSameSerpAfterReloadAndReturningViaLinkClick) {
  Navigate(kNonSerpUrl);
  Navigate(kBraveSerpUrl);
  Navigate(kBraveSerpUrl, /*is_new_navigation=*/false);
  ASSERT_EQ(2U, GetSearchCount(SerpMetricType::kBrave));

  // Going back clears the last recorded SERP, so following a link back to it
  // records again.
  Navigate(kNonSerpUrl, /*is_new_navigation=*/false);
  Navigate(kBraveSerpUrl);

  ExpectSearchCounts(/*brave=*/3, /*google=*/0, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest, RecordBackAndForwardNavigation) {
  Navigate(kNonSerpUrl);
  Navigate(kGoogleSerpUrl);
  ASSERT_EQ(1U, GetSearchCount(SerpMetricType::kGoogle));

  // Back to the non-SERP, then forward to the SERP again.
  Navigate(kNonSerpUrl, /*is_new_navigation=*/false);
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);

  ExpectSearchCounts(/*brave=*/0, /*google=*/2, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       RecordBackAndForwardNavigationBetweenEquivalentSerpUrls) {
  // The two URLs are the same search, so the second new navigation is
  // deduplicated, but both back/forward navigations are recorded.
  Navigate(kGoogleSerpUrl);
  Navigate(R"(https://www.google.com/search?q=foobar&t=web)");
  ASSERT_EQ(1U, GetSearchCount(SerpMetricType::kGoogle));

  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);
  Navigate(R"(https://www.google.com/search?q=foobar&t=web)",
           /*is_new_navigation=*/false);

  ExpectSearchCounts(/*brave=*/0, /*google=*/3, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       RecordAfterMultipleBackAndForwardNavigations) {
  Navigate(kNonSerpUrl);
  Navigate(kGoogleSerpUrl);
  ASSERT_EQ(1U, GetSearchCount(SerpMetricType::kGoogle));

  Navigate(kNonSerpUrl, /*is_new_navigation=*/false);
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);
  Navigate(kNonSerpUrl, /*is_new_navigation=*/false);
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);

  ExpectSearchCounts(/*brave=*/0, /*google=*/3, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       DoNotRecordNewNavigationToSameSerpAfterBackAndForwardNavigation) {
  Navigate(kNonSerpUrl);
  Navigate(kGoogleSerpUrl);
  Navigate(kNonSerpUrl, /*is_new_navigation=*/false);
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);
  ASSERT_EQ(2U, GetSearchCount(SerpMetricType::kGoogle));

  Navigate(kGoogleSerpUrl);

  ExpectSearchCounts(/*brave=*/0, /*google=*/2, /*other=*/0);
}

TEST_F(SerpMetricsNavigationTrackerTest,
       RecordDifferentSerpAfterBackAndForwardNavigation) {
  Navigate(kNonSerpUrl);
  Navigate(kGoogleSerpUrl);
  Navigate(kNonSerpUrl, /*is_new_navigation=*/false);
  Navigate(kGoogleSerpUrl, /*is_new_navigation=*/false);
  ASSERT_EQ(2U, GetSearchCount(SerpMetricType::kGoogle));

  Navigate(kBraveSerpUrl);

  ExpectSearchCounts(/*brave=*/1, /*google=*/2, /*other=*/0);
}

///////////////////////////////////////////////////////////////////////////////
// Tracker state is per tab.

TEST_F(SerpMetricsNavigationTrackerTest, RecordSameSerpInNewTab) {
  Navigate(kBraveSerpUrl);
  ASSERT_EQ(1U, GetSearchCount(SerpMetricType::kBrave));

  // A new tab gets its own tracker, but shares the same `SerpMetrics`.
  SerpMetricsNavigationTracker other_tracker(*serp_metrics_);
  other_tracker.OnNavigationFinished(GURL(kBraveSerpUrl),
                                     /*is_new_navigation=*/true);

  ExpectSearchCounts(/*brave=*/2, /*google=*/0, /*other=*/0);
}

}  // namespace serp_metrics
