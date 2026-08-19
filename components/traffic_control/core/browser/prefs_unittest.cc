// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/prefs.h"

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "base/values.h"
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

TEST_F(TrafficControlPrefsTest, GetRulesFromPrefsSkipsMalformedEntries) {
  base::ListValue list;
  list.Append(base::Value(42));  // Not a dictionary.

  list.Append(
      base::DictValue()
          .Set("enabled", true)
          .Set("condition", base::DictValue().Set("url_filter", "bad.com"))
          .Set("target",
               base::DictValue().Set("container_id", "c1")));  // Missing id.

  list.Append(
      base::DictValue()
          .Set("id", "missing-enabled")
          .Set("condition", base::DictValue().Set("url_filter", "bad.com"))
          .Set("target", base::DictValue()));  // Missing enabled.

  list.Append(base::DictValue()
                  .Set("id", "missing-condition")
                  .Set("enabled", true)
                  .Set("target", base::DictValue()));  // Missing condition.

  list.Append(
      base::DictValue()
          .Set("id", "missing-target")
          .Set("enabled", true)
          .Set("condition", base::DictValue().Set(
                                "url_filter", "bad.com")));  // Missing target.

  list.Append(base::DictValue()
                  .Set("id", "missing-url-filter")
                  .Set("enabled", true)
                  .Set("condition", base::DictValue())
                  .Set("target", base::DictValue()));  // Missing url_filter.

  list.Append(
      base::DictValue()
          .Set("id", "bad-container-id")
          .Set("enabled", true)
          .Set("condition", base::DictValue().Set("url_filter", "bad.com"))
          .Set("target", base::DictValue().Set(
                             "container_id", 42)));  // Non-string container_id.

  list.Append(
      base::DictValue()
          .Set("id", "valid-id")
          .Set("enabled", true)
          .Set("condition", base::DictValue().Set("url_filter", "example.com"))
          .Set("target", base::DictValue().Set("container_id", "container-1")));

  prefs_.SetList(prefs::kTrafficControlList, std::move(list));

  auto loaded = GetRulesFromPrefs(prefs_);
  ASSERT_EQ(1u, loaded.size());
  EXPECT_EQ("valid-id", loaded[0]->id);
  EXPECT_TRUE(loaded[0]->enabled);
  EXPECT_EQ("example.com", loaded[0]->url_filter);
  ASSERT_TRUE(loaded[0]->target->container_id.has_value());
  EXPECT_EQ("container-1", *loaded[0]->target->container_id);
}

}  // namespace traffic_control
