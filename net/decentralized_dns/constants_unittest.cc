/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/decentralized_dns/constants.h"

#include "base/strings/string_split.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Optional;

namespace decentralized_dns {

TEST(DecentralisedDnsConstantsTest, UnstoppableDomainsSuffixLookup) {
  std::string full_list = GetUnstoppableDomainSuffixFullList();
  EXPECT_FALSE(full_list.empty());

  // Split the full list into individual TLDs
  std::vector<std::string> domains = base::SplitString(
      full_list, ", ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  for (const auto& domain : domains) {
    std::string url = "https://example" + domain;
    EXPECT_THAT(GetUnstoppableDomainSuffix(url),
                Optional(std::string_view(domain)))
        << "Failed to recognize domain: " << domain;
  }

  // Test invalid cases
  EXPECT_FALSE(
      GetUnstoppableDomainSuffix("https://foo.bar.crypto.unknown").has_value());
  EXPECT_FALSE(GetUnstoppableDomainSuffix("https://unstoppable").has_value());
}

}  // namespace decentralized_dns
