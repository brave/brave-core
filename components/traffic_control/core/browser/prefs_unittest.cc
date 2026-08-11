// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/prefs.h"

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "brave/components/traffic_control/core/browser/pref_names.h"
#include "brave/components/traffic_control/core/browser/prefs_registration.h"
#include "brave/components/traffic_control/core/common/features.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace traffic_control {

class TrafficControlPrefsTest : public testing::Test {
 public:
  TrafficControlPrefsTest() {
    feature_list_.InitAndEnableFeature(features::kTrafficControl);
    RegisterProfilePrefs(prefs_.registry());
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
};

TEST_F(TrafficControlPrefsTest, RoundTrip) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(mojom::TrafficRule::New("id-1", true, "mail.example.com",
                                          mojom::Target::New("container-1")));

  SetRulesToPrefs(rules, prefs_);
  auto loaded = GetRulesFromPrefs(prefs_);
  ASSERT_EQ(1u, loaded.size());
  EXPECT_EQ("id-1", loaded[0]->id);
  EXPECT_TRUE(loaded[0]->enabled);
  EXPECT_EQ("mail.example.com", loaded[0]->url_filter);
  ASSERT_TRUE(loaded[0]->target->container_id.has_value());
  EXPECT_EQ("container-1", *loaded[0]->target->container_id);
}

TEST_F(TrafficControlPrefsTest, RoundTripOmitsUnsetContainerId) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(mojom::TrafficRule::New("id-1", true, "example.com",
                                          mojom::Target::New(std::nullopt)));

  SetRulesToPrefs(rules, prefs_);
  auto loaded = GetRulesFromPrefs(prefs_);
  ASSERT_EQ(1u, loaded.size());
  EXPECT_FALSE(loaded[0]->target->container_id.has_value());
}

TEST_F(TrafficControlPrefsTest, FullListReplacePreservesOrder) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(
      mojom::TrafficRule::New("a", true, "a.com", mojom::Target::New("c1")));
  rules.push_back(
      mojom::TrafficRule::New("b", false, "b.com", mojom::Target::New("c2")));
  SetRulesToPrefs(rules, prefs_);

  auto loaded = GetRulesFromPrefs(prefs_);
  ASSERT_EQ(2u, loaded.size());
  EXPECT_EQ("a", loaded[0]->id);
  EXPECT_EQ("b", loaded[1]->id);

  std::vector<mojom::TrafficRulePtr> reordered;
  reordered.push_back(loaded[1]->Clone());
  reordered.push_back(loaded[0]->Clone());
  SetRulesToPrefs(reordered, prefs_);

  loaded = GetRulesFromPrefs(prefs_);
  ASSERT_EQ(2u, loaded.size());
  EXPECT_EQ("b", loaded[0]->id);
  EXPECT_EQ("a", loaded[1]->id);
}

}  // namespace traffic_control
