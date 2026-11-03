// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/request_otr/browser/request_otr_rule.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace request_otr {

// A host with any number of trailing dots must share its eTLD+1 with the bare
// host, or it misses the rules keyed on it.
TEST(RequestOTRRuleUnitTest, GetETLDForRequestOTRIgnoresTrailingDots) {
  EXPECT_EQ("example.com", RequestOTRRule::GetETLDForRequestOTR("example.com"));
  EXPECT_EQ("example.com",
            RequestOTRRule::GetETLDForRequestOTR("example.com."));
  EXPECT_EQ("example.com",
            RequestOTRRule::GetETLDForRequestOTR("example.com.."));
  EXPECT_EQ("example.com",
            RequestOTRRule::GetETLDForRequestOTR("www.example.com..."));
}

}  // namespace request_otr
