// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_throttle.h"

#include <memory>
#include <optional>

#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/query_filter/browser/test_support/query_filter_test_helper.h"
#include "brave/components/query_filter/common/features.h"
#include "brave/components/query_filter/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "net/base/isolation_info.h"
#include "net/cookies/site_for_cookies.h"
#include "services/network/public/cpp/resource_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace query_filter {

namespace {

// A tracking parameter from the default rules, plus one to preserve.
constexpr char kUrlWithTrackers[] = "https://example.com/?fbclid=abc&keep=1";
constexpr char kFilteredURL[] = "https://example.com/?keep=1";
// A cross-site page that embeds `kUrlWithTrackers` in an iframe.
constexpr char kOtherSite[] = "https://other.com/";

}  // namespace

class QueryFilterThrottleTest : public testing::Test {
 public:
  QueryFilterThrottleTest() : feature_list_(features::kQueryFilterComponent) {}

  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());
    // Toggling Brave Shields records a P3A usage metric into local state.
    brave_shields::RegisterShieldsP3ALocalPrefs(local_state_.registry());
    prefs_.registry()->RegisterBooleanPref(
        kTrackingQueryParametersFilteringEnabled, true);

    host_content_settings_map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &prefs_, /*is_off_the_record=*/false, /*store_last_modified=*/false,
        /*restore_session=*/false, /*should_record_metrics=*/false);
    shields_settings_service_ =
        std::make_unique<brave_shields::BraveShieldsSettingsService>(
            *host_content_settings_map_, &local_state_, &prefs_);
    rules_.emplace();
  }

  void TearDown() override {
    shields_settings_service_.reset();
    host_content_settings_map_->ShutdownOnUIThread();
    rules_.reset();
  }

  // Builds a request the way content does for an outermost main-frame
  // navigation.
  static network::ResourceRequest MainFrameRequest(const GURL& url) {
    network::ResourceRequest request;
    request.url = url;
    request.method = "GET";
    request.is_outermost_main_frame = true;
    const url::Origin origin = url::Origin::Create(url);
    request.trusted_params = network::ResourceRequest::TrustedParams();
    request.trusted_params->isolation_info = net::IsolationInfo::Create(
        net::IsolationInfo::RequestType::kMainFrame, origin, origin,
        net::SiteForCookies::FromOrigin(origin));
    return request;
  }

  // Builds a request the way content does for a navigation of an iframe
  // embedded in `top_frame_url`.
  static network::ResourceRequest SubFrameRequest(const GURL& top_frame_url,
                                                  const GURL& url) {
    network::ResourceRequest request;
    request.url = url;
    request.method = "GET";
    request.is_outermost_main_frame = false;
    const url::Origin top_frame_origin = url::Origin::Create(top_frame_url);
    request.request_initiator = top_frame_origin;
    request.trusted_params = network::ResourceRequest::TrustedParams();
    request.trusted_params->isolation_info = net::IsolationInfo::Create(
        net::IsolationInfo::RequestType::kSubFrame, top_frame_origin,
        url::Origin::Create(url),
        net::SiteForCookies::FromOrigin(top_frame_origin));
    return request;
  }

  std::unique_ptr<blink::URLLoaderThrottle> CreateThrottle(
      const network::ResourceRequest& request) {
    return QueryFilterThrottle::MaybeCreateThrottle(
        shields_settings_service_.get(), request);
  }

  // Runs `WillStartRequest` and returns the URL the loader would start.
  GURL FilterRequest(network::ResourceRequest request) {
    auto throttle = CreateThrottle(request);
    if (!throttle) {
      ADD_FAILURE() << "Expected a throttle to be created for " << request.url;
      return request.url;
    }
    bool defer = false;
    throttle->WillStartRequest(&request, &defer);
    return request.url;
  }

  brave_shields::BraveShieldsSettingsService* shields_settings_service() {
    return shields_settings_service_.get();
  }

  sync_preferences::TestingPrefServiceSyncable* prefs() { return &prefs_; }

 private:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  std::unique_ptr<brave_shields::BraveShieldsSettingsService>
      shields_settings_service_;
  std::optional<test::ScopedTestingQueryFilterRules> rules_;
};

TEST_F(QueryFilterThrottleTest, NoThrottleWithoutShieldsSettingsService) {
  EXPECT_FALSE(QueryFilterThrottle::MaybeCreateThrottle(
      /*shields_settings_service=*/nullptr,
      MainFrameRequest(GURL(kUrlWithTrackers))));
}

TEST_F(QueryFilterThrottleTest, StripsTrackersFromRequestURL) {
  EXPECT_EQ(GURL(kFilteredURL),
            FilterRequest(MainFrameRequest(GURL(kUrlWithTrackers))));
}

TEST_F(QueryFilterThrottleTest, LeavesURLWithoutTrackersUnchanged) {
  EXPECT_EQ(GURL(kFilteredURL),
            FilterRequest(MainFrameRequest(GURL(kFilteredURL))));
}

TEST_F(QueryFilterThrottleTest, MainFrameUsesDestinationShieldsSetting) {
  const GURL url(kUrlWithTrackers);
  shields_settings_service()->SetBraveShieldsEnabled(false, GURL(kOtherSite));
  EXPECT_TRUE(CreateThrottle(MainFrameRequest(url)));

  shields_settings_service()->SetBraveShieldsEnabled(false, url);
  EXPECT_FALSE(CreateThrottle(MainFrameRequest(url)));
}

// Shields down on the embedding page opts its iframes out, even when Shields
// are up on the iframe's own site.
TEST_F(QueryFilterThrottleTest, SubFrameUsesTopFrameShieldsSettingWhenDown) {
  shields_settings_service()->SetBraveShieldsEnabled(false, GURL(kOtherSite));
  EXPECT_FALSE(CreateThrottle(
      SubFrameRequest(GURL(kOtherSite), GURL(kUrlWithTrackers))));
}

// Shields up on the embedding page filters its iframes, even when Shields are
// down on the iframe's own site.
TEST_F(QueryFilterThrottleTest, SubFrameUsesTopFrameShieldsSettingWhenUp) {
  shields_settings_service()->SetBraveShieldsEnabled(false,
                                                     GURL(kUrlWithTrackers));
  EXPECT_EQ(GURL(kFilteredURL), FilterRequest(SubFrameRequest(
                                    GURL(kOtherSite), GURL(kUrlWithTrackers))));
}

// Without IsolationInfo the initiator stands in for the top-level site, as in
// the network delegate.
TEST_F(QueryFilterThrottleTest, SubFrameFallsBackToInitiatorShieldsSetting) {
  network::ResourceRequest request =
      SubFrameRequest(GURL(kOtherSite), GURL(kUrlWithTrackers));
  request.trusted_params.reset();

  EXPECT_TRUE(CreateThrottle(request));
  shields_settings_service()->SetBraveShieldsEnabled(false, GURL(kOtherSite));
  EXPECT_FALSE(CreateThrottle(request));
}
}  // namespace query_filter
