/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/v2/browser/brave_vpn_service_impl.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {
namespace v2 {

TEST(BraveVpnServiceImplTest, CompilesAndIsConnectedReturnsFalse) {
  // This test just verifies that the test fixture compiles and links correctly
  // with the real BraveVpnServiceImpl implementation.
  BraveVpnServiceImpl service;

  // Service is not connected by default.
  EXPECT_FALSE(service.IsConnected());
}

}  // namespace v2
}  // namespace brave_vpn
