// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/content/query_filter_throttle.h"

#include <memory>
#include <optional>

#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/query_filter/browser/test_support/query_filter_test_helper.h"
#include "brave/components/query_filter/common/features.h"
#include "brave/components/query_filter/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/resource_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace query_filter {

namespace {

// A tracking parameter from the default rules, plus one to preserve.
constexpr char kUrlWithTrackers[] = "https://example.com/?fbclid=abc&keep=1";
constexpr char kFilteredURL[] = "https://example.com/?keep=1";

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

  std::unique_ptr<blink::URLLoaderThrottle> CreateThrottle(const GURL& url) {
    return QueryFilterThrottle::MaybeCreateThrottle(
        &prefs_, shields_settings_service_.get(), url);
  }

  // Runs `WillStartRequest` and returns the URL the loader would start.
  GURL FilterRequest(const GURL& url) {
    auto throttle = CreateThrottle(url);
    CHECK(throttle);
    network::ResourceRequest request;
    request.url = url;
    request.method = "GET";
    bool defer = false;
    throttle->WillStartRequest(&request, &defer);
    return request.url;
  }

  brave_shields::BraveShieldsSettingsService* shields_settings_service() {
    return shields_settings_service_.get();
  }

  sync_preferences::TestingPrefServiceSyncable* prefs() { return &prefs_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  std::unique_ptr<brave_shields::BraveShieldsSettingsService>
      shields_settings_service_;
  std::optional<test::ScopedTestingQueryFilterRules> rules_;
};

TEST_F(QueryFilterThrottleTest, NoThrottleWhenFeatureDisabled) {
  base::test::ScopedFeatureList disabled;
  disabled.InitAndDisableFeature(features::kQueryFilterComponent);

  EXPECT_FALSE(CreateThrottle(GURL(kUrlWithTrackers)));
}

TEST_F(QueryFilterThrottleTest, NoThrottleWhenDisabledByPolicy) {
  prefs()->SetManagedPref(kTrackingQueryParametersFilteringEnabled,
                          base::Value(false));

  EXPECT_FALSE(CreateThrottle(GURL(kUrlWithTrackers)));
}

TEST_F(QueryFilterThrottleTest, NoThrottleWhenBraveShieldsDisabled) {
  shields_settings_service()->SetBraveShieldsEnabled(false,
                                                     GURL(kUrlWithTrackers));

  EXPECT_FALSE(CreateThrottle(GURL(kUrlWithTrackers)));
}

TEST_F(QueryFilterThrottleTest, NoThrottleWithoutShieldsSettingsService) {
  EXPECT_FALSE(QueryFilterThrottle::MaybeCreateThrottle(
      prefs(), /*shields_settings_service=*/nullptr, GURL(kUrlWithTrackers)));
}

TEST_F(QueryFilterThrottleTest, StripsTrackersFromRequestURL) {
  EXPECT_EQ(GURL(kFilteredURL), FilterRequest(GURL(kUrlWithTrackers)));
}

TEST_F(QueryFilterThrottleTest, LeavesURLWithoutTrackersUnchanged) {
  EXPECT_EQ(GURL(kFilteredURL), FilterRequest(GURL(kFilteredURL)));
}
}  // namespace query_filter
