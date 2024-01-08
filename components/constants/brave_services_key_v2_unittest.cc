// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/constants/brave_services_key_v2.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace constants {

TEST(BraveServicesKeyV2Unittest, GetBraveServicesV2Headers) {
  // Should return a digest and authorization header for valid service
  auto result = GetBraveServicesV2Headers("{}", Service::kAIChat);
  ASSERT_TRUE(result);
  std::pair<std::string, std::string> auth_headers = result.value();
  EXPECT_EQ("SHA-256=RBNvo1WzZ4oRRq0W9+hknpT7T8If536DEMBg9hyq/4o=",
            auth_headers.first);
  EXPECT_EQ(
      "Signature "
      "keyId=\"\",algorithm=\"hs2019\",headers=\"digest\",signature="
      "\"2EfzngIam3KB9qOj0SQ6mc1PdUCGTgQ9/3J6ueZuAq4=\"",
      auth_headers.second);
}

}  // namespace constants
