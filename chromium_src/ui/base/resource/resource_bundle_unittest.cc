// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ui/base/resource/resource_bundle_unittest.cc>

namespace ui {

TEST_F(ResourceBundleTest, BlocksThemedLottieImage) {
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();

  const ui::ImageModel& image1 = rb.GetThemedLottieImageNamed(1001);
  const ui::ImageModel& image2 = rb.GetThemedLottieImageNamed(9999);

  // Both images should be empty
  EXPECT_TRUE(image1.IsEmpty());
  EXPECT_TRUE(image2.IsEmpty());

  // Image should remain empty across additional call
  const ui::ImageModel& image3 = rb.GetThemedLottieImageNamed(1001);
  EXPECT_TRUE(image3.IsEmpty());
}

}  // namespace ui
