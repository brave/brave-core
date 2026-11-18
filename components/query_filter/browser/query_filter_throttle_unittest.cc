// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_throttle.h"

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/query_filter/common/features.h"
#include "brave/components/query_filter/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/resource_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace query_filter {

namespace {

constexpr char kUrlWithTrackers[] = "https://example.com/?fbclid=abc&keep=1";

network::ResourceRequest MakeRequest(const GURL& url) {
  network::ResourceRequest request;
  request.url = url;
  return request;
}

}  // namespace

class QueryFilterThrottleTest : public testing::Test {
 public:
  QueryFilterThrottleTest() {
    feature_list_.InitAndEnableFeature(
        features::kQueryFilterBrowserThrottleEnabled);
  }

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
  }

  void TearDown() override {
    shields_settings_service_.reset();
    host_content_settings_map_->ShutdownOnUIThread();
  }

  std::unique_ptr<blink::URLLoaderThrottle> CreateThrottle(
      const network::ResourceRequest& request) {
    return QueryFilterThrottle::MaybeCreateThrottle(
        shields_settings_service_.get(), request);
  }

  brave_shields::BraveShieldsSettingsService* shields_settings_service() {
    return shields_settings_service_.get();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  std::unique_ptr<brave_shields::BraveShieldsSettingsService>
      shields_settings_service_;
};

TEST_F(QueryFilterThrottleTest, NoThrottleWithoutShieldsSettingsService) {
  EXPECT_FALSE(QueryFilterThrottle::MaybeCreateThrottle(
      /*shields_settings_service=*/nullptr,
      MakeRequest(GURL(kUrlWithTrackers))));
}

TEST_F(QueryFilterThrottleTest, NoThrottleWhenBrowserThrottleFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(
      features::kQueryFilterBrowserThrottleEnabled);

  EXPECT_FALSE(CreateThrottle(MakeRequest(GURL(kUrlWithTrackers))));
}

TEST_F(QueryFilterThrottleTest, CheckThrottleCreated) {
  EXPECT_TRUE(CreateThrottle(MakeRequest(GURL(kUrlWithTrackers))));
}

}  // namespace query_filter
