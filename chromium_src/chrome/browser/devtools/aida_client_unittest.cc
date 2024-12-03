/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/devtools/aida_client_unittest.cc"

TEST_F(AidaClientTest, NotAvailable) {
  auto availability = AidaClient::CanUseAida(profile_.get());
  EXPECT_FALSE(availability.available);
  EXPECT_TRUE(availability.blocked);
  EXPECT_TRUE(availability.blocked_by_age);
  EXPECT_TRUE(availability.blocked_by_enterprise_policy);
  EXPECT_TRUE(availability.blocked_by_geo);
  EXPECT_FALSE(availability.blocked_by_rollout);
  EXPECT_TRUE(availability.disallow_logging);
}
