// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ui/base/resource/resource_bundle_unittest.cc>

namespace ui {

TEST_F(ResourceBundleTest, BlocksThemedLottieImage) {
  // Pick an arbitrary fake ID that won't collide with real ones
  const int kFakeId = 999999;

  // Set the blocked IDs
  base::flat_set<int> blocked = {kFakeId};
  SetBlockedThemedLottieImages(blocked);

  // Verify it's blocked
  const auto& img =
      ResourceBundle::GetSharedInstance().GetThemedLottieImageNamed(kFakeId);
  EXPECT_TRUE(img.IsEmpty());
}

}  // namespace ui
