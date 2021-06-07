/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/environment.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/channel.h"
#include "content/public/test/browser_test.h"

#if defined(OS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

using BraveChannelInfoBrowserTest = PlatformBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveChannelInfoBrowserTest, DefaultChannelTest) {
#if defined(OFFICIAL_BUILD)
#if defined(OS_LINUX)
  // Set channel info explicitly to test. Linux uses this env vars to determine
  // which channel is used.
  auto env = base::Environment::Create();
  env->SetVar("CHROME_VERSION_EXTRA", "stable");
#endif
  EXPECT_NE(version_info::Channel::UNKNOWN, chrome::GetChannel());
#else
  EXPECT_EQ(version_info::Channel::UNKNOWN, chrome::GetChannel());
#endif
}
