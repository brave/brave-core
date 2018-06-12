/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/common/channel_info.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/version_info/channel.h"

using BraveChannelInfoBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveChannelInfoBrowserTest, DefaultChannelTest) {
#if defined(OFFICIAL_BUILD)
  EXPECT_NE(version_info::Channel::UNKNOWN, chrome::GetChannel());
#else
  EXPECT_EQ(version_info::Channel::UNKNOWN, chrome::GetChannel());
#endif
}
