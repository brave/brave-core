// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/prefs.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/traffic_control/core/browser/pref_names.h"
#include "brave/components/traffic_control/core/browser/prefs_registration.h"
#include "brave/components/traffic_control/core/common/features.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace traffic_control {

namespace {

mojom::TrafficRulePtr MakeRule(std::string_view id,
                               bool enabled,
                               std::optional<std::string> url_filter,
                               std::optional<std::string> container_id,
                               bool temporary_container = false) {
  return mojom::TrafficRule::New(
      std::string(id), enabled, mojom::Condition::New(std::move(url_filter)),
      mojom::Target::New(std::move(container_id), temporary_container));
}

}  // namespace

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
  rules.push_back(MakeRule("id-1", true, "mail.example.com", "container-1"));

  SetRulesToPrefs(rules, prefs_);
  auto loaded = GetRulesFromPrefs(prefs_);
  ASSERT_EQ(1u, loaded.size());
  EXPECT_EQ("id-1", loaded[0]->id);
  EXPECT_TRUE(loaded[0]->enabled);
  ASSERT_TRUE(loaded[0]->condition->url_filter.has_value());
  EXPECT_EQ("mail.example.com", *loaded[0]->condition->url_filter);
  ASSERT_TRUE(loaded[0]->target->container_id.has_value());
  EXPECT_EQ("container-1", *loaded[0]->target->container_id);
  EXPECT_FALSE(loaded[0]->target->temporary_container);
}

TEST_F(TrafficControlPrefsTest, RoundTripOmitsUnsetOptionalFields) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("id-1", true, std::nullopt, std::nullopt));

  SetRulesToPrefs(rules, prefs_);
  const base::ListValue& stored = prefs_.GetList(prefs::kTrafficControlList);
  ASSERT_EQ(1u, stored.size());
  const base::DictValue& dict = stored[0].GetDict();
  EXPECT_FALSE(dict.FindDict("condition")->contains("url_filter"));
  EXPECT_FALSE(dict.FindDict("target")->contains("container_id"));
  EXPECT_FALSE(dict.FindDict("target")->contains("temporary_container"));

  auto loaded = GetRulesFromPrefs(prefs_);
  ASSERT_EQ(1u, loaded.size());
  EXPECT_FALSE(loaded[0]->condition->url_filter.has_value());
  EXPECT_FALSE(loaded[0]->target->container_id.has_value());
  EXPECT_FALSE(loaded[0]->target->temporary_container);
}

TEST_F(TrafficControlPrefsTest, RoundTripPreservesEmptyContainerId) {
  // Empty container_id means "open in a non-contained tab" and must round-trip
  // as an explicit empty string, not as an omitted/unset field.
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("id-1", true, "example.com", std::string()));

  SetRulesToPrefs(rules, prefs_);
  const base::ListValue& stored = prefs_.GetList(prefs::kTrafficControlList);
  ASSERT_EQ(1u, stored.size());
  const base::DictValue* target = stored[0].GetDict().FindDict("target");
  ASSERT_TRUE(target);
  const std::string* container_id = target->FindString("container_id");
  ASSERT_TRUE(container_id);
  EXPECT_TRUE(container_id->empty());

  auto loaded = GetRulesFromPrefs(prefs_);
  ASSERT_EQ(1u, loaded.size());
  ASSERT_TRUE(loaded[0]->target->container_id.has_value());
  EXPECT_TRUE(loaded[0]->target->container_id->empty());
  EXPECT_FALSE(loaded[0]->target->temporary_container);
}

TEST_F(TrafficControlPrefsTest, RoundTripTemporaryContainer) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("id-1", true, "example.com", std::nullopt,
                           /*temporary_container=*/true));

  SetRulesToPrefs(rules, prefs_);
  const base::ListValue& stored = prefs_.GetList(prefs::kTrafficControlList);
  ASSERT_EQ(1u, stored.size());
  const base::DictValue* target = stored[0].GetDict().FindDict("target");
  ASSERT_TRUE(target);
  EXPECT_FALSE(target->contains("container_id"));
  EXPECT_EQ(true, target->FindBool("temporary_container"));

  auto loaded = GetRulesFromPrefs(prefs_);
  ASSERT_EQ(1u, loaded.size());
  EXPECT_FALSE(loaded[0]->target->container_id.has_value());
  EXPECT_TRUE(loaded[0]->target->temporary_container);
}

TEST_F(TrafficControlPrefsTest, FullListReplacePreservesOrder) {
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("a", true, "a.com", "c1"));
  rules.push_back(MakeRule("b", false, "b.com", "c2"));
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
                  .Set("id", "bad-url-filter")
                  .Set("enabled", true)
                  .Set("condition", base::DictValue().Set("url_filter", 42))
                  .Set("target", base::DictValue()));  // Non-string url_filter.

  list.Append(
      base::DictValue()
          .Set("id", "bad-container-id")
          .Set("enabled", true)
          .Set("condition", base::DictValue().Set("url_filter", "bad.com"))
          .Set("target", base::DictValue().Set(
                             "container_id", 42)));  // Non-string container_id.

  list.Append(
      base::DictValue()
          .Set("id", "bad-new-temporary")
          .Set("enabled", true)
          .Set("condition", base::DictValue().Set("url_filter", "bad.com"))
          .Set("target",
               base::DictValue().Set("temporary_container",
                                     "yes")));  // Non-bool temporary_container.

  list.Append(
      base::DictValue()
          .Set("id", "valid-id")
          .Set("enabled", true)
          .Set("condition", base::DictValue().Set("url_filter", "example.com"))
          .Set("target", base::DictValue().Set("container_id", "container-1")));

  // Empty condition/target dicts are valid: optional fields are simply unset.
  list.Append(base::DictValue()
                  .Set("id", "valid-empty-optionals")
                  .Set("enabled", false)
                  .Set("condition", base::DictValue())
                  .Set("target", base::DictValue()));

  prefs_.SetList(prefs::kTrafficControlList, std::move(list));

  auto loaded = GetRulesFromPrefs(prefs_);
  ASSERT_EQ(2u, loaded.size());
  EXPECT_EQ("valid-id", loaded[0]->id);
  EXPECT_TRUE(loaded[0]->enabled);
  ASSERT_TRUE(loaded[0]->condition->url_filter.has_value());
  EXPECT_EQ("example.com", *loaded[0]->condition->url_filter);
  ASSERT_TRUE(loaded[0]->target->container_id.has_value());
  EXPECT_EQ("container-1", *loaded[0]->target->container_id);

  EXPECT_EQ("valid-empty-optionals", loaded[1]->id);
  EXPECT_FALSE(loaded[1]->enabled);
  EXPECT_FALSE(loaded[1]->condition->url_filter.has_value());
  EXPECT_FALSE(loaded[1]->target->container_id.has_value());
  EXPECT_FALSE(loaded[1]->target->temporary_container);
}

}  // namespace traffic_control
