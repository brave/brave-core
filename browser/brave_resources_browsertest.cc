/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_theme_resources.h"
#include "brave/grit/brave_unscaled_resources.h"
#include "build/build_config.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/base/resource/resource_bundle.h"

using BraveResourcesBrowserTest = PlatformBrowserTest;

// Check brave's theme resources pacakges are properly added.
IN_PROC_BROWSER_TEST_F(PlatformBrowserTest, ResourceExistanceTest) {
  gfx::Image test_image =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(
          IDR_PRODUCT_LOGO_32_DEV);
  EXPECT_FALSE(test_image.IsEmpty());

#if BUILDFLAG(IS_LINUX)
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
