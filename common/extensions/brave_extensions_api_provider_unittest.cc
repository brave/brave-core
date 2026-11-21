// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "extensions/common/features/feature_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

TEST(BraveExtensionsAPIProviderTest, RegistersBraveAPIFeatures) {
  EXPECT_TRUE(FeatureProvider::GetAPIFeature("braveTheme"));
  EXPECT_TRUE(FeatureProvider::GetAPIFeature("webDiscovery"));
}

}  // namespace extensions
