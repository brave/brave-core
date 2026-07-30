/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/containers/cookie_store_id.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/containers/core/common/features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

class CookieStoreIdTest : public testing::Test {
 public:
  CookieStoreIdTest() {
    feature_list_.InitAndEnableFeature(features::kContainers);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(CookieStoreIdTest, GetContainerStoreId) {
  EXPECT_EQ("containers:abc-123", GetContainerStoreId("abc-123"));
}

TEST_F(CookieStoreIdTest, ParseContainerStoreId) {
  EXPECT_EQ("abc-123", ParseContainerStoreId("containers:abc-123"));
  EXPECT_FALSE(ParseContainerStoreId("0"));
  EXPECT_FALSE(ParseContainerStoreId("1"));
  EXPECT_FALSE(ParseContainerStoreId("containers:"));
  EXPECT_FALSE(ParseContainerStoreId("container:abc-123"));
  EXPECT_FALSE(ParseContainerStoreId("containers:bad_id!"));
}

TEST_F(CookieStoreIdTest, IsContainerStoreId) {
  EXPECT_TRUE(IsContainerStoreId("containers:work"));
  EXPECT_FALSE(IsContainerStoreId("0"));
  EXPECT_FALSE(IsContainerStoreId("firefox-container-1"));
}

TEST_F(CookieStoreIdTest, RoundTrip) {
  const std::string id = "550e8400-e29b-41d4-a716-446655440000";
  EXPECT_EQ(id, ParseContainerStoreId(GetContainerStoreId(id)));
}

}  // namespace containers
