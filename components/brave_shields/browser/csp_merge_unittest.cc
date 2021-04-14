/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

const base::Optional<std::string> NO_POLICY = base::nullopt;

const auto POLICY1 =
    base::Optional<std::string>("script-src 'self' 'unsafe-inline'");
const auto POLICY2 =
    base::Optional<std::string>("media-src 'self' https://example.com");

TEST(CspMergeTest, MergeTwoEmptyPolicies) {
  const auto a = NO_POLICY;
  auto b = NO_POLICY;

  MergeCspDirectiveInto(a, &b);

  ASSERT_EQ(b, NO_POLICY);
}

TEST(CspMergeTest, MergeEmptyIntoNonEmpty) {
  const auto a = POLICY1;
  auto b = NO_POLICY;

  MergeCspDirectiveInto(a, &b);

  ASSERT_EQ(b, POLICY1);
}

TEST(CspMergeTest, MergeNonEmptyIntoEmpty) {
  const auto a = NO_POLICY;
  auto b = POLICY1;

  MergeCspDirectiveInto(a, &b);

  ASSERT_EQ(b, POLICY1);
}

TEST(CspMergeTest, MergeNonEmptyIntoNonEmpty) {
  const auto a = POLICY1;
  auto b = POLICY2;

  const std::string expected =
      "script-src 'self' 'unsafe-inline', media-src 'self' https://example.com";

  MergeCspDirectiveInto(a, &b);

  ASSERT_TRUE(b);
  ASSERT_EQ(*b, expected);
}

}  // namespace brave_shields
