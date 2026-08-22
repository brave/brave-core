// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_control_settings_handler.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/traffic_control/core/browser/prefs_registration.h"
#include "brave/components/traffic_control/core/common/features.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/receiver.h"
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

class MockTrafficControlSettingsObserver
    : public mojom::TrafficControlSettingsUI {
 public:
  MockTrafficControlSettingsObserver() = default;
  ~MockTrafficControlSettingsObserver() override = default;

  mojo::PendingRemote<mojom::TrafficControlSettingsUI> BindAndGetRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  MockTrafficControlSettingsObserver* operator->() {
    FlushForTesting();
    return this;
  }

  void FlushForTesting() {
    if (receiver_.is_bound()) {
      receiver_.FlushForTesting();
    }
  }

  void OnRulesChanged(std::vector<mojom::TrafficRulePtr> rules) override {
    last_rules_ = std::move(rules);
    rules_changed_count_++;
  }

  const std::vector<mojom::TrafficRulePtr>& last_rules() const {
    return last_rules_;
  }

  int rules_changed_count() const { return rules_changed_count_; }

 private:
  mojo::Receiver<mojom::TrafficControlSettingsUI> receiver_{this};
  std::vector<mojom::TrafficRulePtr> last_rules_;
  int rules_changed_count_ = 0;
};

}  // namespace

class TrafficControlSettingsHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kTrafficControl);
    RegisterProfilePrefs(prefs_.registry());
    handler_ = std::make_unique<TrafficControlSettingsHandler>(&prefs_);
    handler_->BindUI(mock_observer_.BindAndGetRemote());
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  MockTrafficControlSettingsObserver mock_observer_;
  std::unique_ptr<TrafficControlSettingsHandler> handler_;
};

TEST_F(TrafficControlSettingsHandlerTest, AddUpdateRemove) {
  base::test::TestFuture<std::optional<mojom::RuleOperationError>> error_future;
  auto rule = MakeRule("", true, "example.com", "c1");
  handler_->AddRule(rule->Clone(), error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());

  base::test::TestFuture<std::vector<mojom::TrafficRulePtr>> future;
  handler_->GetRules(future.GetCallback());
  auto rules = future.Take();
  ASSERT_EQ(1u, rules.size());
  EXPECT_FALSE(rules[0]->id.empty());

  auto updated = rules[0]->Clone();
  updated->condition->url_filter = "other.example.com";
  handler_->UpdateRule(updated->Clone(), error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());

  handler_->GetRules(future.GetCallback());
  rules = future.Take();
  ASSERT_EQ(1u, rules.size());
  ASSERT_TRUE(rules[0]->condition->url_filter.has_value());
  EXPECT_EQ("other.example.com", *rules[0]->condition->url_filter);

  handler_->RemoveRule(rules[0]->id, error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());

  handler_->GetRules(future.GetCallback());
  rules = future.Take();
  EXPECT_TRUE(rules.empty());
}

TEST_F(TrafficControlSettingsHandlerTest, AddAppendsInListOrder) {
  base::test::TestFuture<std::optional<mojom::RuleOperationError>> error_future;
  handler_->AddRule(MakeRule("", true, "a.com", "c1"),
                    error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());
  handler_->AddRule(MakeRule("", true, "b.com", "c2"),
                    error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());

  base::test::TestFuture<std::vector<mojom::TrafficRulePtr>> future;
  handler_->GetRules(future.GetCallback());
  auto rules = future.Take();
  ASSERT_EQ(2u, rules.size());
  ASSERT_TRUE(rules[0]->condition->url_filter.has_value());
  ASSERT_TRUE(rules[1]->condition->url_filter.has_value());
  EXPECT_EQ("a.com", *rules[0]->condition->url_filter);
  EXPECT_EQ("b.com", *rules[1]->condition->url_filter);
}

TEST_F(TrafficControlSettingsHandlerTest, RejectsInvalidUrlFilter) {
  base::test::TestFuture<std::optional<mojom::RuleOperationError>> error_future;
  handler_->AddRule(MakeRule("", true, "", "c1"), error_future.GetCallback());
  EXPECT_EQ(mojom::RuleOperationError::kInvalidUrlFilter, error_future.Take());

  handler_->AddRule(MakeRule("", true, std::nullopt, "c1"),
                    error_future.GetCallback());
  EXPECT_EQ(mojom::RuleOperationError::kInvalidUrlFilter, error_future.Take());
}

TEST_F(TrafficControlSettingsHandlerTest, AcceptsUnsetContainerId) {
  base::test::TestFuture<std::optional<mojom::RuleOperationError>> error_future;
  handler_->AddRule(MakeRule("", true, "example.com", std::nullopt),
                    error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());

  base::test::TestFuture<std::vector<mojom::TrafficRulePtr>> future;
  handler_->GetRules(future.GetCallback());
  auto rules = future.Take();
  ASSERT_EQ(1u, rules.size());
  EXPECT_FALSE(rules[0]->target->container_id.has_value());
}

TEST_F(TrafficControlSettingsHandlerTest, AcceptsEmptyContainerId) {
  base::test::TestFuture<std::optional<mojom::RuleOperationError>> error_future;
  handler_->AddRule(MakeRule("", true, "example.com", std::string()),
                    error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());

  base::test::TestFuture<std::vector<mojom::TrafficRulePtr>> future;
  handler_->GetRules(future.GetCallback());
  auto rules = future.Take();
  ASSERT_EQ(1u, rules.size());
  ASSERT_TRUE(rules[0]->target->container_id.has_value());
  EXPECT_TRUE(rules[0]->target->container_id->empty());
  EXPECT_FALSE(rules[0]->target->temporary_container);
}

TEST_F(TrafficControlSettingsHandlerTest, AcceptsTemporaryContainer) {
  base::test::TestFuture<std::optional<mojom::RuleOperationError>> error_future;
  handler_->AddRule(MakeRule("", true, "example.com", std::nullopt,
                             /*temporary_container=*/true),
                    error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());

  base::test::TestFuture<std::vector<mojom::TrafficRulePtr>> future;
  handler_->GetRules(future.GetCallback());
  auto rules = future.Take();
  ASSERT_EQ(1u, rules.size());
  EXPECT_FALSE(rules[0]->target->container_id.has_value());
  EXPECT_TRUE(rules[0]->target->temporary_container);
}

TEST_F(TrafficControlSettingsHandlerTest, RejectsTemporaryWithContainerId) {
  base::test::TestFuture<std::optional<mojom::RuleOperationError>> error_future;
  handler_->AddRule(MakeRule("", true, "example.com", "c1",
                             /*temporary_container=*/true),
                    error_future.GetCallback());
  EXPECT_EQ(mojom::RuleOperationError::kInvalidTarget, error_future.Take());
}

}  // namespace traffic_control
