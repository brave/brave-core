/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_system_request_handler.h"

#include <string>

#include "brave/components/constants/network_constants.h"
#include "services/network/public/cpp/resource_request.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

TEST(BraveSystemRequestHandlerTest, AddBraveServiceKeyHeaderForBrave) {
  GURL url("https://demo.brave.com");
  network::ResourceRequest request;

  request.url = url;
  brave::AddBraveServicesKeyHeader(&request);
  auto key = request.headers.GetHeader(kBraveServicesKeyHeader);
  ASSERT_TRUE(key);
  EXPECT_EQ(*key, BraveServicesKeyForTesting());
}

TEST(BraveSystemRequestHandlerTest, AddBraveServiceKeyHeaderForBraveSoftware) {
  GURL url("https://demo.bravesoftware.com");
  network::ResourceRequest request;

  request.url = url;
  brave::AddBraveServicesKeyHeader(&request);
  auto key = request.headers.GetHeader(kBraveServicesKeyHeader);
  ASSERT_TRUE(key);
  EXPECT_EQ(*key, BraveServicesKeyForTesting());
}

TEST(BraveSystemRequestHandlerTest, DontAddBraveServiceKeyHeader) {
  GURL url("https://demo.example.com");
  network::ResourceRequest request;

  request.url = url;
  brave::AddBraveServicesKeyHeader(&request);
  auto key = request.headers.GetHeader(kBraveServicesKeyHeader);
  ASSERT_FALSE(key);
}

}  // namespace brave
