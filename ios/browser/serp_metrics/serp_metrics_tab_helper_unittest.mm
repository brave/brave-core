/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/serp_metrics/serp_metrics_tab_helper.h"

#include <memory>
#include <optional>
#include <string_view>

#include "base/memory/raw_ref.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/serp_metrics/test/fake_serp_metrics_time_period_store.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store_factory.h"
#include "brave/ios/browser/brave_stats/brave_stats_prefs.h"
#include "components/prefs/testing_pref_service.h"
#include "ios/chrome/test/testing_application_context.h"
#include "ios/web/public/test/fakes/fake_navigation_context.h"
#include "ios/web/public/test/fakes/fake_web_state.h"
#include "net/http/http_response_headers.h"
#include "testing/platform_test.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace serp_metrics {

namespace {

class NavigationBuilder final {
 public:
  NavigationBuilder(web::FakeWebState& web_state, const GURL& url)
      : web_state_(web_state), url_(url) {}

  NavigationBuilder(const NavigationBuilder&) = delete;
  NavigationBuilder& operator=(const NavigationBuilder&) = delete;

  ~NavigationBuilder() = default;

  NavigationBuilder& WithTransition(ui::PageTransition transition) {
    transition_ = transition;
    return *this;
  }

  NavigationBuilder& WithHasCommitted(bool has_committed) {
    has_committed_ = has_committed;
    return *this;
  }

  NavigationBuilder& WithResponseHeaders(
      scoped_refptr<net::HttpResponseHeaders> headers) {
    response_headers_ = std::move(headers);
    return *this;
  }

  NavigationBuilder& WithIsRendererInitiated(bool is_renderer_initiated) {
    is_renderer_initiated_ = is_renderer_initiated;
    return *this;
  }

  void Simulate() {
    web::FakeNavigationContext context;
    context.SetUrl(url_);
    context.SetHasCommitted(has_committed_);
    context.SetPageTransition(transition_);
    if (response_headers_) {
      context.SetResponseHeaders(std::move(response_headers_));
    }
    if (is_renderer_initiated_.has_value()) {
      context.SetIsRendererInitiated(*is_renderer_initiated_);
    }
    web_state_->OnNavigationFinished(&context);
  }

 private:
  raw_ref<web::FakeWebState> web_state_;
  GURL url_;
  ui::PageTransition transition_ = ui::PAGE_TRANSITION_TYPED;
  bool has_committed_ = true;
  scoped_refptr<net::HttpResponseHeaders> response_headers_;
  std::optional<bool> is_renderer_initiated_;
};

}  // namespace

class SerpMetricsTabHelperTest : public PlatformTest {
 public:
  SerpMetricsTabHelperTest() {
    brave_stats::RegisterLocalStatePrefs(local_state_.registry());
    TestingApplicationContext::GetGlobal()->SetLocalState(&local_state_);

    scoped_feature_list_.InitAndEnableFeature(kSerpMetricsFeature);

    serp_metrics_ = std::make_unique<SerpMetrics>(
        &local_state_, test::FakeSerpMetricsTimePeriodStoreFactory());

    SerpMetricsTabHelper::CreateForWebState(&web_state_, *serp_metrics_);
  }

  SerpMetricsTabHelperTest(const SerpMetricsTabHelperTest&) = delete;
  SerpMetricsTabHelperTest& operator=(const SerpMetricsTabHelperTest&) = delete;

  ~SerpMetricsTabHelperTest() override {
    TestingApplicationContext::GetGlobal()->SetLocalState(nullptr);
  }

  NavigationBuilder Navigation(const GURL& url) {
    return NavigationBuilder(web_state_, url);
  }

  PrefService& local_state() { return local_state_; }

  SerpMetrics* serp_metrics() { return serp_metrics_.get(); }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  TestingPrefServiceSimple local_state_;
  web::FakeWebState web_state_;
  std::unique_ptr<SerpMetrics> serp_metrics_;
};

TEST_F(SerpMetricsTabHelperTest, RecordBraveSearchEngineResultsPage) {
  Navigation(GURL("https://search.brave.com/search?q=test")).Simulate();

  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, RecordGoogleSearchEngineResultsPage) {
  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();

  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, RecordOtherSearchEngineResultsPage) {
  Navigation(GURL("https://duckduckgo.com/?q=test")).Simulate();

  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, RecordStartpageSearchEngineResultsPage) {
  Navigation(GURL("https://www.startpage.com/sp/search")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));

  // For Startpage, we cannot determine whether two URLs represent the same
  // search results page, so these pages are always classified.
  Navigation(GURL("https://www.startpage.com/sp/search")).Simulate();
  EXPECT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, RecordWhenNavigatingViaLinkClick) {
  Navigation(GURL("https://plugh.xyzzy.com/thud")).Simulate();
  ASSERT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));

  Navigation(GURL("https://search.brave.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_LINK)
      .Simulate();

  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTabHelperTest, RecordForHttp4xxResponse) {
  Navigation(GURL("https://search.brave.com/search?q=test"))
      .WithResponseHeaders(net::HttpResponseHeaders::TryToCreate(
          "HTTP/1.1 404 Not Found\r\n\r\n"))
      .Simulate();

  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTabHelperTest, RecordForHttp5xxResponse) {
  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithResponseHeaders(net::HttpResponseHeaders::TryToCreate(
          "HTTP/1.1 500 Internal Server Error\r\n\r\n"))
      .Simulate();

  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsTabHelperTest, DoNotRecordNonSearchUrl) {
  Navigation(GURL("https://plugh.xyzzy.com/thud")).Simulate();

  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, DoNotRecordSameSearchWhenConsecutive) {
  Navigation(GURL("https://duckduckgo.com/?q=test")).Simulate();
  Navigation(GURL("https://duckduckgo.com/?q=test")).Simulate();

  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, DoNotRecordPagingThroughSameSearchResults) {
  Navigation(GURL("https://search.brave.com/search?q=test")).Simulate();
  Navigation(GURL("https://search.brave.com/search?q=test&page=2")).Simulate();

  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTabHelperTest,
       RecordSameSearchAfterNavigatingAwayAndReturning) {
  Navigation(GURL("https://duckduckgo.com/?q=test")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));

  Navigation(GURL("https://plugh.xyzzy.com/thud")).Simulate();
  Navigation(GURL("https://duckduckgo.com/?q=test")).Simulate();

  EXPECT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, RecordSameSearchWithDifferentSerpInBetween) {
  Navigation(GURL("https://duckduckgo.com/?q=test")).Simulate();
  Navigation(GURL("https://duckduckgo.com/?q=other")).Simulate();
  Navigation(GURL("https://duckduckgo.com/?q=test")).Simulate();

  EXPECT_EQ(3U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, RecordReloadNavigation) {
  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();
  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_RELOAD)
      .Simulate();
  EXPECT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsTabHelperTest, RecordAfterMultipleReloadNavigations) {
  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_RELOAD)
      .Simulate();
  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_RELOAD)
      .Simulate();

  EXPECT_EQ(3U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsTabHelperTest,
       RecordSameSearchAfterReloadNavigationForNewNavigation) {
  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_RELOAD)
      .Simulate();
  ASSERT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();
  EXPECT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsTabHelperTest,
       RecordSameSearchAfterReloadNavigationAndLinkClick) {
  Navigation(GURL("https://plugh.xyzzy.com/thud")).Simulate();
  Navigation(GURL("https://search.brave.com/search?q=test")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));

  Navigation(GURL("https://search.brave.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_RELOAD)
      .Simulate();
  ASSERT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));

  Navigation(GURL("https://plugh.xyzzy.com/thud"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  Navigation(GURL("https://search.brave.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_LINK)
      .Simulate();

  EXPECT_EQ(3U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTabHelperTest,
       RecordDifferentSearchAfterReloadNavigationForNewNavigation) {
  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_RELOAD)
      .Simulate();
  ASSERT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://search.brave.com/search?q=test")).Simulate();

  EXPECT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTabHelperTest, RecordBackForwardNavigation) {
  Navigation(GURL("https://plugh.xyzzy.com/thud")).Simulate();
  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://plugh.xyzzy.com/thud"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();

  EXPECT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsTabHelperTest, RecordBackForwardNavigationForSameUrl) {
  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://www.google.com/search?q=test&t=web")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  Navigation(GURL("https://www.google.com/search?q=test&t=web"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();

  EXPECT_EQ(3U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsTabHelperTest, RecordAfterMultipleBackForwardNavigations) {
  Navigation(GURL("https://plugh.xyzzy.com/thud")).Simulate();
  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://plugh.xyzzy.com/thud"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  Navigation(GURL("https://plugh.xyzzy.com/thud"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();

  EXPECT_EQ(3U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsTabHelperTest,
       DoNotRecordSameSearchAfterBackForwardNavigationForNewNavigation) {
  Navigation(GURL("https://plugh.xyzzy.com/thud")).Simulate();
  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://plugh.xyzzy.com/thud"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  ASSERT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();

  EXPECT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsTabHelperTest,
       RecordSameSearchAfterBackForwardNavigationAndLinkClick) {
  Navigation(GURL("https://plugh.xyzzy.com/thud")).Simulate();
  Navigation(GURL("https://search.brave.com/search?q=test")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));

  Navigation(GURL("https://plugh.xyzzy.com/thud"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  Navigation(GURL("https://search.brave.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  ASSERT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));

  Navigation(GURL("https://plugh.xyzzy.com/thud"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  Navigation(GURL("https://search.brave.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_LINK)
      .Simulate();

  EXPECT_EQ(3U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTabHelperTest,
       RecordDifferentSearchAfterBackForwardNavigationForNewNavigation) {
  Navigation(GURL("https://plugh.xyzzy.com/thud")).Simulate();
  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();
  ASSERT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://plugh.xyzzy.com/thud"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  ASSERT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Navigation(GURL("https://search.brave.com/search?q=test")).Simulate();

  EXPECT_EQ(2U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTabHelperTest, RecordWithoutUserGesture) {
  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithTransition(ui::PAGE_TRANSITION_LINK)
      .WithIsRendererInitiated(true)
      .Simulate();

  EXPECT_EQ(1U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsTabHelperTest, DoNotRecordIfStatsReportingIsDisabled) {
  local_state().SetBoolean(kStatsReportingEnabled, false);

  Navigation(GURL("https://search.brave.com/search?q=test")).Simulate();
  Navigation(GURL("https://www.google.com/search?q=test")).Simulate();
  Navigation(GURL("https://duckduckgo.com/?q=test")).Simulate();

  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTabHelperTest, DoNotRecordIfStatsForUncommittedNavigation) {
  Navigation(GURL("https://search.brave.com/search?q=test"))
      .WithHasCommitted(false)
      .Simulate();
  Navigation(GURL("https://www.google.com/search?q=test"))
      .WithHasCommitted(false)
      .Simulate();
  Navigation(GURL("https://duckduckgo.com/?q=test"))
      .WithHasCommitted(false)
      .Simulate();

  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

}  // namespace serp_metrics
