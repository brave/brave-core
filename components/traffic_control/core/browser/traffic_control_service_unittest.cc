// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_control_service.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "brave/components/traffic_control/core/browser/prefs_registration.h"
#include "brave/components/traffic_control/core/common/features.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

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

TEST_F(TrafficControlServiceTest, AddUpdateRemove) {
  auto rule = mojom::TrafficRule::New("", true, "example.com",
                                      mojom::Target::New("c1"));
  EXPECT_FALSE(service_->AddRule(rule->Clone()).has_value());

  auto rules = service_->GetRules();
  ASSERT_EQ(1u, rules.size());
  EXPECT_FALSE(rules[0]->id.empty());

  auto updated = rules[0]->Clone();
  updated->url_filter = "other.example.com";
  EXPECT_FALSE(service_->UpdateRule(updated->Clone()).has_value());
  rules = service_->GetRules();
  ASSERT_EQ(1u, rules.size());
  EXPECT_EQ("other.example.com", rules[0]->url_filter);

  EXPECT_FALSE(service_->RemoveRule(rules[0]->id).has_value());
  EXPECT_TRUE(service_->GetRules().empty());
}

TEST_F(TrafficControlServiceTest, AddAppendsInListOrder) {
  EXPECT_FALSE(service_
                   ->AddRule(mojom::TrafficRule::New("", true, "a.com",
                                                     mojom::Target::New("c1")))
                   .has_value());
  EXPECT_FALSE(service_
                   ->AddRule(mojom::TrafficRule::New("", true, "b.com",
                                                     mojom::Target::New("c2")))
                   .has_value());

  auto rules = service_->GetRules();
  ASSERT_EQ(2u, rules.size());
  EXPECT_EQ("a.com", rules[0]->url_filter);
  EXPECT_EQ("b.com", rules[1]->url_filter);
}

TEST_F(TrafficControlServiceTest, RejectsInvalidUrlFilter) {
  auto rule = mojom::TrafficRule::New("", true, "", mojom::Target::New("c1"));
  EXPECT_EQ(mojom::RuleOperationError::kInvalidUrlFilter,
            service_->AddRule(std::move(rule)));
}

TEST_F(TrafficControlServiceTest, AcceptsUnsetContainerId) {
  auto rule = mojom::TrafficRule::New("", true, "example.com",
                                      mojom::Target::New(std::nullopt));
  EXPECT_FALSE(service_->AddRule(std::move(rule)).has_value());

  auto rules = service_->GetRules();
  ASSERT_EQ(1u, rules.size());
  EXPECT_FALSE(rules[0]->target->container_id.has_value());
}

TEST_F(TrafficControlServiceTest, EnabledPref) {
  EXPECT_FALSE(service_->IsEnabled());
  service_->SetEnabled(true);
  EXPECT_TRUE(service_->IsEnabled());
}

}  // namespace traffic_control
