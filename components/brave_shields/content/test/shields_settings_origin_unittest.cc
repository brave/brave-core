/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/content/browser/shields_settings_origin.h"

#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/test/navigation_simulator.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_shields {
namespace {

using ShieldsSettingsOriginTest = ChromeRenderViewHostTestHarness;

TEST_F(ShieldsSettingsOriginTest, GetForFrameWithoutValueUsesCommittedOrigin) {
  // No Brave observer is attached to this WebContents.
  const GURL page("https://a.test/page");
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             page);
  EXPECT_EQ(GURL("https://a.test/"),
            ShieldsSettingsOrigin::ForFrame(*main_rfh()).GetURL());
}

}  // namespace
}  // namespace brave_shields
