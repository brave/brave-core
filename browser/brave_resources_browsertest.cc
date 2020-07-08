/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_theme_resources.h"
#include "brave/grit/brave_unscaled_resources.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/base/resource/resource_bundle.h"

using BraveResourcesBrowserTest = InProcessBrowserTest;

// Check brave's theme resources pacakges are properly added.
IN_PROC_BROWSER_TEST_F(BraveResourcesBrowserTest, ResourceExistanceTest) {
  gfx::Image test_image =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(
          IDR_PRODUCT_LOGO_32_DEV);
  EXPECT_FALSE(test_image.IsEmpty());

#if defined(OS_LINUX)
  test_image =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(
          IDR_PRODUCT_LOGO_128_BETA);
  EXPECT_FALSE(test_image.IsEmpty());
  test_image =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(
          IDR_PRODUCT_LOGO_128_DEV);
  EXPECT_FALSE(test_image.IsEmpty());
#endif
}
