// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/rule_validation.h"

#include <optional>
#include <string>
#include <string_view>

#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
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

TEST(RuleValidationTest, IsValidUrlFilterAcceptsCommonFilters) {
  EXPECT_TRUE(IsValidUrlFilter("example.com"));
  EXPECT_TRUE(IsValidUrlFilter(".example.com"));
  EXPECT_TRUE(IsValidUrlFilter("https://example.com"));
  EXPECT_TRUE(IsValidUrlFilter("https://example.com/path"));
  EXPECT_TRUE(IsValidUrlFilter("*"));
}

TEST(RuleValidationTest, IsValidUrlFilterAcceptsMultilineWithComments) {
  EXPECT_TRUE(IsValidUrlFilter(
      "# work sites\nexample.com\n\n.corp.example.com\n# end\n"));
  EXPECT_TRUE(IsValidUrlFilter("  example.com  \n  other.com  "));
}

TEST(RuleValidationTest, IsValidUrlFilterRejectsInvalidFilters) {
  EXPECT_FALSE(IsValidUrlFilter(""));
  EXPECT_FALSE(IsValidUrlFilter("   \n# only comments\n"));
  EXPECT_FALSE(IsValidUrlFilter("http://"));
  EXPECT_FALSE(IsValidUrlFilter("example.com\nhttp://"));
}

TEST(RuleValidationTest, ValidateRule) {
  EXPECT_EQ(ValidateRule(MakeRule("", true, "example.com", "c1"),
                         /*require_empty_id=*/true),
            std::nullopt);

  EXPECT_EQ(ValidateRule(MakeRule("id", true, "example.com", "c1"),
                         /*require_empty_id=*/true),
            mojom::RuleOperationError::kIdShouldBeEmpty);

  EXPECT_EQ(ValidateRule(MakeRule("", true, "example.com", "c1"),
                         /*require_empty_id=*/false),
            mojom::RuleOperationError::kIdShouldBeSet);

  auto missing_condition = MakeRule("id", true, "example.com", "c1");
  missing_condition->condition = nullptr;
  EXPECT_EQ(ValidateRule(missing_condition, /*require_empty_id=*/false),
            mojom::RuleOperationError::kInvalidCondition);

  EXPECT_EQ(ValidateRule(MakeRule("id", true, "", "c1"),
                         /*require_empty_id=*/false),
            mojom::RuleOperationError::kInvalidUrlFilter);

  EXPECT_EQ(ValidateRule(MakeRule("id", true, std::nullopt, "c1"),
                         /*require_empty_id=*/false),
            mojom::RuleOperationError::kInvalidUrlFilter);

  EXPECT_EQ(ValidateRule(MakeRule("id", true, "example.com", "c1"),
                         /*require_empty_id=*/false),
            std::nullopt);

  EXPECT_EQ(ValidateRule(MakeRule("id", true, "example.com", std::nullopt),
                         /*require_empty_id=*/false),
            std::nullopt);

  EXPECT_EQ(ValidateRule(MakeRule("id", true, "example.com", std::nullopt,
                                  /*temporary_container=*/true),
                         /*require_empty_id=*/false),
            std::nullopt);

  EXPECT_EQ(ValidateRule(MakeRule("id", true, "example.com", "c1",
                                  /*temporary_container=*/true),
                         /*require_empty_id=*/false),
            mojom::RuleOperationError::kInvalidTarget);

  EXPECT_EQ(ValidateRule(nullptr, /*require_empty_id=*/true),
            mojom::RuleOperationError::kInvalidTarget);
}

}  // namespace traffic_control
