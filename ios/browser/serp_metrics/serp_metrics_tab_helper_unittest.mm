/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/serp_metrics/serp_metrics_tab_helper.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/time_period_storage/time_period_store.h"
#include "brave/components/time_period_storage/time_period_store_factory.h"
#include "brave/ios/browser/brave_stats/brave_stats_prefs.h"
#include "components/prefs/testing_pref_service.h"
#include "ios/chrome/test/testing_application_context.h"
#include "ios/web/public/test/fakes/fake_navigation_context.h"
#include "ios/web/public/test/fakes/fake_web_state.h"
#include "testing/platform_test.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace serp_metrics {

namespace {

constexpr std::string_view kBraveSearchUrl =
    "https://search.brave.com/search?q=foo";
constexpr std::string_view kGoogleSearchUrl =
    "https://www.google.com/search?q=foo";
constexpr std::string_view kDuckDuckGoSearchUrl =
    "https://duckduckgo.com/?q=foo";
constexpr std::string_view kNonSearchUrl = "https://plugh.xyzzy.com/thud";

class FakeTimePeriodStore : public TimePeriodStore {
 public:
  FakeTimePeriodStore() = default;

  FakeTimePeriodStore(const FakeTimePeriodStore&) = delete;
  FakeTimePeriodStore& operator=(const FakeTimePeriodStore&) = delete;

  ~FakeTimePeriodStore() override = default;

  const base::ListValue* Get() override { return &list_; }

  void Set(base::ListValue list) override { list_ = std::move(list); }

  void Clear() override { list_.clear(); }

 private:
  base::ListValue list_;
};

class FakeTimePeriodStoreFactory : public TimePeriodStoreFactory {
 public:
  FakeTimePeriodStoreFactory() = default;

  FakeTimePeriodStoreFactory(const FakeTimePeriodStoreFactory&) = delete;
  FakeTimePeriodStoreFactory& operator=(const FakeTimePeriodStoreFactory&) =
      delete;

  ~FakeTimePeriodStoreFactory() override = default;

  std::unique_ptr<TimePeriodStore> Build(
      const char* metric_name) const override {
    return std::make_unique<FakeTimePeriodStore>();
  }
};

}  // namespace

class SerpMetricsTabHelperTest : public PlatformTest {
 protected:
  SerpMetricsTabHelperTest() {
    brave_stats::RegisterLocalStatePrefs(local_state_.registry());
    TestingApplicationContext::GetGlobal()->SetLocalState(&local_state_);

    scoped_feature_list_.InitAndEnableFeature(kSerpMetricsFeature);

    serp_metrics_ = std::make_unique<SerpMetrics>(&local_state_,
                                                  FakeTimePeriodStoreFactory());

    SerpMetricsTabHelper::CreateForWebState(&web_state_, *serp_metrics_);
  }

  ~SerpMetricsTabHelperTest() override {
    TestingApplicationContext::GetGlobal()->SetLocalState(nullptr);
  }

  void SimulateNavigation(
      const GURL& url,
      ui::PageTransition transition = ui::PAGE_TRANSITION_TYPED,
      bool has_committed = true) {
    web::FakeNavigationContext context;
    context.SetUrl(url);
    context.SetHasCommitted(has_committed);
    context.SetPageTransition(transition);
    web_state_.OnNavigationFinished(&context);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
  TestingPrefServiceSimple local_state_;
  web::FakeWebState web_state_;
  std::unique_ptr<SerpMetrics> serp_metrics_;
};

TEST_F(SerpMetricsTabHelperTest, RecordBraveSearchEngineResultsPage) {
  SimulateNavigation(GURL(kBraveSearchUrl));

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, RecordGoogleSearchEngineResultsPage) {
  SimulateNavigation(GURL(kGoogleSearchUrl));

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, RecordOtherSearchEngineResultsPage) {
  SimulateNavigation(GURL(kDuckDuckGoSearchUrl));

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, DoNotRecordNonSearchUrl) {
  SimulateNavigation(GURL(kNonSearchUrl));

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, DoNotRecordSameSearchWhenConsecutive) {
  SimulateNavigation(GURL(kDuckDuckGoSearchUrl));
  SimulateNavigation(GURL(kDuckDuckGoSearchUrl));

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, RecordBackForwardNavigationForSameUrl) {
  SimulateNavigation(GURL(kBraveSearchUrl));
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));

  SimulateNavigation(GURL(kBraveSearchUrl), ui::PAGE_TRANSITION_FORWARD_BACK);

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTabHelperTest, DoNotRecordIfStatsReportingIsDisabled) {
  local_state_.SetBoolean(kStatsReportingEnabled, false);

  SimulateNavigation(GURL(kBraveSearchUrl));
  SimulateNavigation(GURL(kGoogleSearchUrl));
  SimulateNavigation(GURL(kDuckDuckGoSearchUrl));

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, DoNotRecordIfStatsForUncommittedNavigation) {
  SimulateNavigation(GURL(kBraveSearchUrl), ui::PAGE_TRANSITION_TYPED,
                     /*has_committed=*/false);
  SimulateNavigation(GURL(kGoogleSearchUrl), ui::PAGE_TRANSITION_TYPED,
                     /*has_committed=*/false);
  SimulateNavigation(GURL(kDuckDuckGoSearchUrl), ui::PAGE_TRANSITION_TYPED,
                     /*has_committed=*/false);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, RecordReloadNavigation) {
  SimulateNavigation(GURL(kGoogleSearchUrl));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));

  SimulateNavigation(GURL(kGoogleSearchUrl), ui::PAGE_TRANSITION_RELOAD);
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsTabHelperTest, RecordSearchWithDifferentQueries) {
  SimulateNavigation(GURL("https://search.brave.com/search?q=foo"));
  SimulateNavigation(GURL("https://search.brave.com/search?q=bar"));

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTabHelperTest,
       RecordSameSearchAfterNavigatingAwayAndReturning) {
  SimulateNavigation(GURL(kBraveSearchUrl));
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));

  SimulateNavigation(GURL(kNonSearchUrl));
  SimulateNavigation(GURL(kBraveSearchUrl));

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
}

}  // namespace serp_metrics
