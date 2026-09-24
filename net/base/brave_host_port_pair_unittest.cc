// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>

#include "net/base/host_port_pair.h"
#include "net/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

std::vector<HostPortPair> GetTestCasesInAccedingOrder() {
  std::vector<HostPortPair> v;
  v.emplace_back("a.com", 10);
  v.emplace_back("user1", "pass1", "a.com", 10);
  v.emplace_back("user1", "pass2", "a.com", 10);
  v.emplace_back("user2", "pass2", "a.com", 10);
  v.emplace_back("user2", "pass2", "b.com", 10);
  v.emplace_back("user2", "pass2", "a.com", 11);
  v.emplace_back("user2", "pass3", "a.com", 11);
  v.emplace_back("c.com", 11);
  return v;
}

}  // namespace

TEST(BraveHostPortPairTest, Parsing) {
  HostPortPair foo("user", "pass", "foo.com", 10);
  std::string foo_str = foo.ToString();
  EXPECT_EQ("user:pass@foo.com:10", foo_str);
  HostPortPair bar = HostPortPair::FromString(foo_str);
  EXPECT_EQ(bar.host(), "foo.com");
  EXPECT_EQ(bar.port(), 10);
  EXPECT_EQ(bar.username(), "user");
  EXPECT_EQ(bar.password(), "pass");

  EXPECT_TRUE(foo == bar) << bar.ToString();
}

TEST(BraveHostPortPairTest, Compare) {
  const auto v = GetTestCasesInAccedingOrder();
  // Compare each pair using < and == operators and verify the result.
  for (size_t i = 0; i < v.size(); i++) {
    for (size_t j = 0; j < v.size(); j++) {
      const auto& a = v[i];
      const auto& b = v[j];
      SCOPED_TRACE(testing::Message()
                   << "case " << a.ToString() << " vs " << b.ToString());
      if (i == j) {
        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a.Equals(b));
      } else {
        EXPECT_FALSE(a == b);
        EXPECT_FALSE(a.Equals(b));

        const bool expected_less = i < j;
        EXPECT_EQ(a < b, expected_less);
        EXPECT_EQ(b < a, !expected_less);
      }
    }
  }
}

TEST(BraveHostPortPairTest, Equals) {
  const auto v = GetTestCasesInAccedingOrder();
  HostPortPair second_item("user1", "pass1", "a.com", 10);
  for (size_t i = 0; i < v.size(); i++) {
    EXPECT_EQ(v[i] == second_item, i == 1);
  }
}

}  // namespace net
