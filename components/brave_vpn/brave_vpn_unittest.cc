/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "brave/components/brave_vpn/features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

TEST(BraveVPNTest, Basic) {
  // Disable by default till we implement all brave vpn features.
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kBraveVPN));
}

}  // namespace brave_vpn
