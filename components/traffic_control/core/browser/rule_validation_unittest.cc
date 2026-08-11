// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/rule_validation.h"

#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace traffic_control {

TEST(RuleValidationTest, IsValidUrlFilterAcceptsCommonFilters) {
  EXPECT_TRUE(IsValidUrlFilter("example.com"));
  EXPECT_TRUE(IsValidUrlFilter(".example.com"));
  EXPECT_TRUE(IsValidUrlFilter("https://example.com"));
  EXPECT_TRUE(IsValidUrlFilter("https://example.com/path"));
  EXPECT_TRUE(IsValidUrlFilter("*"));
}

TEST(RuleValidationTest, IsValidUrlFilterRejectsInvalidFilters) {
  EXPECT_FALSE(IsValidUrlFilter(""));
  EXPECT_FALSE(IsValidUrlFilter("http://"));
}

TEST(RuleValidationTest, ValidateRule) {
  EXPECT_EQ(ValidateRule(mojom::TrafficRule::New("", true, "example.com",
                                                 mojom::Target::New("c1")),
                         /*require_empty_id=*/true),
            std::nullopt);

  EXPECT_EQ(ValidateRule(mojom::TrafficRule::New("id", true, "example.com",
                                                 mojom::Target::New("c1")),
                         /*require_empty_id=*/true),
            mojom::RuleOperationError::kIdShouldBeEmpty);

  EXPECT_EQ(ValidateRule(mojom::TrafficRule::New("", true, "example.com",
                                                 mojom::Target::New("c1")),
                         /*require_empty_id=*/false),
            mojom::RuleOperationError::kIdShouldBeSet);

  EXPECT_EQ(ValidateRule(mojom::TrafficRule::New("id", true, "",
                                                 mojom::Target::New("c1")),
                         /*require_empty_id=*/false),
            mojom::RuleOperationError::kInvalidUrlFilter);

  EXPECT_EQ(ValidateRule(mojom::TrafficRule::New("id", true, "example.com",
                                                 mojom::Target::New("c1")),
                         /*require_empty_id=*/false),
            std::nullopt);

  EXPECT_EQ(
      ValidateRule(mojom::TrafficRule::New("id", true, "example.com",
                                           mojom::Target::New(std::nullopt)),
                   /*require_empty_id=*/false),
      std::nullopt);

  EXPECT_EQ(ValidateRule(nullptr, /*require_empty_id=*/true),
            mojom::RuleOperationError::kInvalidTarget);
}

}  // namespace traffic_control
