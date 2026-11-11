// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/content/query_filter_navigation_throttle.h"

#include <memory>
#include <optional>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/query_filter/browser/test_support/query_filter_test_helper.h"
#include "brave/components/query_filter/common/features.h"
#include "brave/components/query_filter/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/mock_navigation_throttle_registry.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace query_filter {

namespace {

// A tracking parameter from the default rules, plus one to preserve.
constexpr char kUrlWithTrackers[] = "https://example.com/?fbclid=abc&keep=1";
constexpr char kFilteredURL[] = "https://example.com/?keep=1";

// `GetRequestMethod()` is a mock with an empty default, and only GET
// navigations are filtered.
class GetNavigationHandle : public content::MockNavigationHandle {
 public:
  using MockNavigationHandle::MockNavigationHandle;

  std::string GetRequestMethod() override { return "GET"; }
};

}  // namespace

class QueryFilterNavigationThrottleTest : public testing::Test {
 public:
  QueryFilterNavigationThrottleTest()
      : feature_list_(features::kQueryFilterComponent) {}

  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());
    // Toggling Brave Shields records a P3A usage metric into local state.
    brave_shields::RegisterShieldsP3ALocalPrefs(local_state_.registry());
    prefs_.registry()->RegisterBooleanPref(
        kTrackingQueryParametersFilteringEnabled, true);
    user_prefs::UserPrefs::Set(&browser_context_, &prefs_);

    host_content_settings_map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &prefs_, /*is_off_the_record=*/false, /*store_last_modified=*/false,
        /*restore_session=*/false, /*should_record_metrics=*/false);
    shields_settings_service_ =
        std::make_unique<brave_shields::BraveShieldsSettingsService>(
            *host_content_settings_map_, &local_state_, &prefs_);

    web_contents_ = content::WebContentsTester::CreateTestWebContents(
        &browser_context_, /*site_instance=*/nullptr);
    rules_.emplace();
  }

  void TearDown() override {
    registry_.reset();
    handle_.reset();
    web_contents_.reset();
    shields_settings_service_.reset();
    host_content_settings_map_->ShutdownOnUIThread();
    rules_.reset();
  }

  // Whether the throttle attaches to a direct navigation to `url`, which is the
  // decision made by `MaybeCreateAndAdd`.
  bool ThrottleAttaches(const GURL& url) { return BuildThrottle(url); }

  // Runs the throttle attached to a direct navigation to `url` and returns its
  // action. `url` must attach a throttle.
  content::NavigationThrottle::ThrottleAction Intercept(const GURL& url) {
    redirected_url_.reset();
    auto* throttle = BuildThrottle(url);
    CHECK(throttle);
    return throttle->WillStartRequest().action();
  }

  const std::optional<GURL>& redirected_url() const { return redirected_url_; }

  brave_shields::BraveShieldsSettingsService* shields_settings_service() {
    return shields_settings_service_.get();
  }

  sync_preferences::TestingPrefServiceSyncable* prefs() { return &prefs_; }

 private:
  // Attaches the throttle for a direct (no initiator) navigation to `url`, the
  // case where tracking parameters are stripped. Returns null if the throttle
  // declined to attach. Keeps the handle and registry alive so the returned
  // pointer stays valid.
  QueryFilterNavigationThrottle* BuildThrottle(const GURL& url) {
    handle_ = std::make_unique<GetNavigationHandle>(web_contents_.get());
    handle_->set_url(url);
    registry_ = std::make_unique<content::MockNavigationThrottleRegistry>(
        handle_.get(),
        content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
    QueryFilterNavigationThrottle::MaybeCreateAndAdd(
        *registry_, shields_settings_service_.get());
    if (registry_->throttles().empty()) {
      return nullptr;
    }
    auto* throttle = static_cast<QueryFilterNavigationThrottle*>(
        registry_->throttles().back().get());
    throttle->SetRedirectCallbackForTesting(base::BindLambdaForTesting(
        [this](const GURL& redirected) { redirected_url_ = redirected; }));
    return throttle;
  }

  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  content::TestBrowserContext browser_context_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  std::unique_ptr<brave_shields::BraveShieldsSettingsService>
      shields_settings_service_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<GetNavigationHandle> handle_;
  std::unique_ptr<content::MockNavigationThrottleRegistry> registry_;
  std::optional<test::ScopedTestingQueryFilterRules> rules_;
  std::optional<GURL> redirected_url_;
};

// Throttle creation specific tests
TEST_F(QueryFilterNavigationThrottleTest, NoThrottlerWhenFeatureDisabled) {
  base::test::ScopedFeatureList disabled;
  disabled.InitAndDisableFeature(features::kQueryFilterComponent);

  EXPECT_FALSE(ThrottleAttaches(GURL(kUrlWithTrackers)));
}

TEST_F(QueryFilterNavigationThrottleTest, NoThrottlerWhenDisabledByPolicy) {
  prefs()->SetManagedPref(kTrackingQueryParametersFilteringEnabled,
                          base::Value(false));

  EXPECT_FALSE(ThrottleAttaches(GURL(kUrlWithTrackers)));
}

// Throttler exists in these tests and we check the interception logic and a
// vanilla check on whether redirect is fired in below tests.
TEST_F(QueryFilterNavigationThrottleTest, CancelsNavigationWithTrackers) {
  // Navigation with trackers gets cancelled after interception.
  EXPECT_EQ(content::NavigationThrottle::CANCEL,
            Intercept(GURL(kUrlWithTrackers)));
  ASSERT_TRUE(redirected_url().has_value());
  EXPECT_EQ(GURL(kFilteredURL), *redirected_url());
}

TEST_F(QueryFilterNavigationThrottleTest, ProceedsNavigationWithoutTrackers) {
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            Intercept(GURL(kFilteredURL)));
  EXPECT_FALSE(redirected_url().has_value());
}

TEST_F(QueryFilterNavigationThrottleTest, ProceedsWhenBraveShieldsDisabled) {
  shields_settings_service()->SetBraveShieldsEnabled(false,
                                                     GURL(kUrlWithTrackers));

  // The throttle still attaches, but leaves the tracked navigation untouched.
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            Intercept(GURL(kUrlWithTrackers)));
  EXPECT_FALSE(redirected_url().has_value());
}

}  // namespace query_filter
