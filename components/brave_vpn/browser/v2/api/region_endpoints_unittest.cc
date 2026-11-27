/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/api/region_endpoints.h"

#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2::endpoints {
namespace {
constexpr char kTestRegion[] = "us-east";
constexpr char kTestRegionPrecision[] = "city-by-country";
}  // namespace

TEST(RegionEndpointsTest, GetHostnamesForRegionRequestBodyToValue) {
  const GetHostnamesForRegionRequestBody body{
      .region = kTestRegion, .region_precision = kTestRegionPrecision};
  EXPECT_EQ(body.ToValue(), base::test::ParseJsonDict(R"({
    "region": "us-east",
    "region-precision": "city-by-country"
  })"));
}

}  // namespace brave_vpn::v2::endpoints
