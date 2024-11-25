/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/decentralized_dns/constants.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Optional;

namespace decentralized_dns {

TEST(DecentralisedDnsConstantsTest, UnstoppableDomainsSuffixLookup) {
  EXPECT_THAT(GetUnstoppableDomainSuffix("https://foo.crypto"),
              Optional(std::string_view(".crypto")));
  EXPECT_THAT(GetUnstoppableDomainSuffix("https://foo.bar.crypto"),
              Optional(std::string_view(".crypto")));
  EXPECT_FALSE(
      GetUnstoppableDomainSuffix("https://foo.bar.crypto.unknown").has_value());
  EXPECT_THAT(GetUnstoppableDomainSuffix("https://foo.unstoppable"),
              Optional(std::string_view(".unstoppable")));
  EXPECT_FALSE(GetUnstoppableDomainSuffix("https://unstoppable").has_value());
}

}  // namespace decentralized_dns
