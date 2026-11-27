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

class QueryFilterThrottleTest : public testing::Test {
 public:
  QueryFilterThrottleTest()
      : feature_list_(features::kQueryFilterBrowserThrottleEnabled) {}

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

  std::unique_ptr<blink::URLLoaderThrottle> CreateQueryFilterThrottle(
      const network::ResourceRequest& request) {
    return QueryFilterThrottle::MaybeCreateThrottle(
        shields_settings_service_.get(), request);
  }

  // Runs `WillStartRequest` and returns the URL the loader would start.
  GURL ThrottleRequest(network::ResourceRequest request) {
    auto throttle = CreateQueryFilterThrottle(request);
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

  // Request would be created like |url| is navigated in main frame.
  static network::ResourceRequest CreateMainFrameRequest(const GURL& url) {
    return CreateResourceRequestForUrl(std::nullopt, url, std::nullopt);
  }

  // Request would be created like |url| is navigated in sub frame within
  // top_frame_url.
  static network::ResourceRequest CreateSubFrameRequest(
      const GURL& top_frame_url,
      const GURL& url,
      std::optional<GURL> initiator_url = std::nullopt) {
    return CreateResourceRequestForUrl(top_frame_url, url, initiator_url);
  }

  sync_preferences::TestingPrefServiceSyncable* prefs() { return &prefs_; }

 protected:
  const GURL kExampleUrlWithTrackers =
      GURL("https://example.com/?fbclid=abc&keep=1");
  const GURL kExampleUrlWithoutTrackers = GURL("https://example.com/?keep=1");
  const GURL kCrossSiteUrl = GURL("https://other.com/");
  const GURL kCrossSiteUrl2 = GURL("https://other2.com/");

 private:
  // Build either a main frame or a sub frame request for |url|.
  static network::ResourceRequest CreateResourceRequestForUrl(
      std::optional<GURL> top_frame_url,
      const GURL& url,
      std::optional<GURL> initiator_url) {
    network::ResourceRequest request;
    request.url = url;
    request.method = "GET";
    request.trusted_params = network::ResourceRequest::TrustedParams();

    if (initiator_url.has_value()) {
      request.request_initiator = url::Origin::Create(*initiator_url);
    }

    net::IsolationInfo::RequestType request_type =
        top_frame_url.has_value() ? net::IsolationInfo::RequestType::kSubFrame
                                  : net::IsolationInfo::RequestType::kMainFrame;
    const url::Origin top_frame_origin =
        top_frame_url.has_value() ? url::Origin::Create(*top_frame_url)
                                  : url::Origin::Create(url);
    const url::Origin frame_origin = url::Origin::Create(url);

    request.is_outermost_main_frame = !top_frame_url.has_value();
    request.trusted_params = network::ResourceRequest::TrustedParams();
    request.trusted_params->isolation_info = net::IsolationInfo::Create(
        request_type, top_frame_origin, frame_origin,
        net::SiteForCookies::FromOrigin(
            top_frame_url.has_value() ? top_frame_origin : frame_origin));
    return request;
  }

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
      CreateMainFrameRequest(kExampleUrlWithTrackers)));
}

TEST_F(QueryFilterThrottleTest, NoThrottleWhenBrowserThrottleFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(
      features::kQueryFilterBrowserThrottleEnabled);

  EXPECT_FALSE(CreateQueryFilterThrottle(
      CreateMainFrameRequest(kExampleUrlWithTrackers)));
}

// Shield setting specific tests.
TEST_F(QueryFilterThrottleTest,
       CheckThrottle_MainFrameNavigation_WithShieldsSettingToggled) {
  const GURL top_frame_url = kExampleUrlWithTrackers;

  // Shields are off for |top_frame_url| so no throttle created.
  shields_settings_service()->SetBraveShieldsEnabled(false, top_frame_url);
  EXPECT_FALSE(
      CreateQueryFilterThrottle(CreateMainFrameRequest(top_frame_url)));

  // Shields are back on for |top_frame_url| so throttle should be created.
  shields_settings_service()->SetBraveShieldsEnabled(true, top_frame_url);
  EXPECT_TRUE(CreateQueryFilterThrottle(CreateMainFrameRequest(top_frame_url)));
}

TEST_F(QueryFilterThrottleTest,
       CheckThrottle_SubFrameNavigation_WithShieldsSettingToggled) {
  const GURL top_frame_url = kCrossSiteUrl;
  const GURL sub_frame_url = kExampleUrlWithTrackers;

  // Turn-off shields for |top_frame_url|
  shields_settings_service()->SetBraveShieldsEnabled(false, top_frame_url);
  // No throttler is created because |top_frame_url| was off.
  EXPECT_FALSE(CreateQueryFilterThrottle(
      CreateSubFrameRequest(top_frame_url, sub_frame_url)));

  // Turn-on shields for |top_frame_url|
  shields_settings_service()->SetBraveShieldsEnabled(true, top_frame_url);
  // No throttler is created because |top_frame_url| was off.
  EXPECT_TRUE(CreateQueryFilterThrottle(
      CreateSubFrameRequest(top_frame_url, sub_frame_url)));
}

// `about:blank` has an opaque origin, so it is not a Shields site.
TEST_F(QueryFilterThrottleTest, CheckThrottle_MainFrameNavigation_AboutBlank) {
  const GURL top_frame_url = GURL("about:blank");
  auto request = CreateMainFrameRequest(top_frame_url);
  ASSERT_TRUE(
      request.trusted_params->isolation_info.top_frame_origin()->opaque());
  EXPECT_FALSE(CreateQueryFilterThrottle(request));
}

TEST_F(QueryFilterThrottleTest, CheckThrottle_SubFrameNavigation_AboutBlank) {
  const GURL top_frame_url = GURL("about:blank");
  const GURL sub_frame_url = GURL(kExampleUrlWithTrackers);

  // 1. |top_frame_url| is converted to origin so should be opaque.
  // 2. No |initiator_url| was specified so |request| should fallback to
  // |sub_frame_url| which is the request URL.
  auto request = CreateSubFrameRequest(top_frame_url, sub_frame_url);
  // Not the actual test so assert.
  ASSERT_TRUE(
      request.trusted_params->isolation_info.top_frame_origin()->opaque());

  // We still throttle because shields is "on" for |sub_frame_url|.
  EXPECT_TRUE(CreateQueryFilterThrottle(request));
  // Disable shields for the |sub_frame_url| explicitly so no throttling should
  // happen.
  shields_settings_service()->SetBraveShieldsEnabled(false, sub_frame_url);
  EXPECT_FALSE(CreateQueryFilterThrottle(request));
}

// Without IsolationInfo the initiator stands in for the top-level site, as in
// the network delegate.
TEST_F(QueryFilterThrottleTest, SubFrame_InitiatorFallbackTest) {
  const GURL top_frame_url = kCrossSiteUrl;
  const GURL sub_frame_url = kExampleUrlWithTrackers;
  const GURL initiator_url = kCrossSiteUrl2;

  network::ResourceRequest request =
      CreateSubFrameRequest(top_frame_url, sub_frame_url, initiator_url);
  // No more knowledge about top-level site so |sub_frame_url| query filtering
  // decision should fallback to |initiator_url|.
  request.trusted_params.reset();
  // Disable shields for |top_frame_url| explicitly.
  shields_settings_service()->SetBraveShieldsEnabled(false, top_frame_url);

  // Expect that we still throttle because shields is on for |initiator_url|.
  EXPECT_TRUE(CreateQueryFilterThrottle(request));
  ASSERT_TRUE(shields_settings_service()->IsBraveShieldsEnabled(initiator_url));

  // Disable shields for |initiator_url| so no throttling should happen.
  shields_settings_service()->SetBraveShieldsEnabled(false, initiator_url);
  EXPECT_FALSE(CreateQueryFilterThrottle(request));
}

// Query filtering functionality specific tests.
TEST_F(QueryFilterThrottleTest, StripsTrackersFromRequestURL) {
  EXPECT_EQ(kExampleUrlWithoutTrackers,
            ThrottleRequest(CreateMainFrameRequest(kExampleUrlWithTrackers)));
}

TEST_F(QueryFilterThrottleTest, LeavesURLWithoutTrackersUnchanged) {
  EXPECT_EQ(
      kExampleUrlWithoutTrackers,
      ThrottleRequest(CreateMainFrameRequest(kExampleUrlWithoutTrackers)));
}

}  // namespace query_filter
