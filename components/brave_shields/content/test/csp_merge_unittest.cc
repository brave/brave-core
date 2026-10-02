// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>
#include <string>

#include "brave/components/brave_shields/core/browser/ad_block_service_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

const std::optional<std::string> kNoPolicy = std::nullopt;

constexpr char kPolicy1[] = "script-src 'self' 'unsafe-inline'";
constexpr char kPolicy2[] = "media-src 'self' https://example.com";

TEST(CspMergeTest, MergeTwoEmptyPolicies) {
  auto b = kNoPolicy;

  MergeCspDirectiveInto(kNoPolicy, &b);

  ASSERT_EQ(b, kNoPolicy);
}

TEST(CspMergeTest, MergeEmptyIntoNonEmpty) {
  auto b = kNoPolicy;

  MergeCspDirectiveInto(kPolicy1, &b);

  ASSERT_EQ(b, kPolicy1);
}

TEST(CspMergeTest, MergeNonEmptyIntoEmpty) {
  std::optional<std::string> b(kPolicy1);

  MergeCspDirectiveInto(kNoPolicy, &b);

  ASSERT_EQ(b, kPolicy1);
}

TEST(CspMergeTest, MergeNonEmptyIntoNonEmpty) {
  std::optional<std::string> b(kPolicy2);

  const std::string expected =
      "script-src 'self' 'unsafe-inline', media-src 'self' https://example.com";

  MergeCspDirectiveInto(kPolicy1, &b);

  ASSERT_TRUE(b);
  ASSERT_EQ(*b, expected);
}

}  // namespace brave_shields
