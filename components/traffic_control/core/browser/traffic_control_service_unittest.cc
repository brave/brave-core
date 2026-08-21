// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_control_service.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/traffic_control/core/browser/pref_names.h"
#include "brave/components/traffic_control/core/browser/prefs.h"
#include "brave/components/traffic_control/core/browser/prefs_registration.h"
#include "brave/components/traffic_control/core/common/features.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace traffic_control {

class TrafficControlServiceTest : public testing::Test {
 public:
  TrafficControlServiceTest() {
    feature_list_.InitAndEnableFeature(features::kTrafficControl);
    RegisterProfilePrefs(prefs_.registry());
    service_ = std::make_unique<TrafficControlService>(&prefs_);
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<TrafficControlService> service_;
};

TEST_F(TrafficControlServiceTest, EnabledPref) {
  EXPECT_FALSE(service_->IsEnabled());
  prefs_.SetBoolean(prefs::kTrafficControlEnabled, true);
  EXPECT_TRUE(service_->IsEnabled());
}

TEST_F(TrafficControlServiceTest, FindMatchingRuleRequiresEnabledPref) {
  auto rule =
      mojom::TrafficRule::New("r1", true, mojom::Condition::New("example.com"),
                              mojom::Target::New(std::string("c1"), false));
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(std::move(rule));
  SetRulesToPrefs(rules, prefs_);

  EXPECT_FALSE(service_->FindMatchingRule(GURL("https://example.com/")));

  prefs_.SetBoolean(prefs::kTrafficControlEnabled, true);
  auto match = service_->FindMatchingRule(GURL("https://example.com/"));
  ASSERT_TRUE(match);
  EXPECT_EQ("r1", match->id);
}

TEST_F(TrafficControlServiceTest, PrefChangeRebuildsMatcher) {
  prefs_.SetBoolean(prefs::kTrafficControlEnabled, true);

  auto rule =
      mojom::TrafficRule::New("r1", true, mojom::Condition::New("example.com"),
                              mojom::Target::New(std::string("c1"), false));
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(std::move(rule));
  SetRulesToPrefs(rules, prefs_);
  EXPECT_TRUE(service_->FindMatchingRule(GURL("https://example.com/")));

  rules.clear();
  rules.push_back(
      mojom::TrafficRule::New("r2", true, mojom::Condition::New("other.com"),
                              mojom::Target::New(std::string("c2"), false)));
  SetRulesToPrefs(rules, prefs_);
  EXPECT_FALSE(service_->FindMatchingRule(GURL("https://example.com/")));
  EXPECT_TRUE(service_->FindMatchingRule(GURL("https://other.com/")));
}

}  // namespace traffic_control
