/* This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/common/channel_info.h"

#include "base/files/file_path.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_paths_internal.h"
#include "components/version_info/channel.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveChannelInfoMacTest, ChannelTest) {
#if defined(OFFICIAL_BUILD)
  EXPECT_EQ(version_info::Channel::CANARY,
            chrome::GetChannelByName("nightly"));
#else
  EXPECT_EQ(version_info::Channel::UNKNOWN,
            chrome::GetChannelByName(""));
#endif
}

TEST(BraveChannelInfoMacTest, DefaultUserDataDirectoryTest) {
  base::FilePath path;
  EXPECT_TRUE(chrome::GetDefaultUserDataDirectory(&path));
#if defined(OFFICIAL_BUILD)
  EXPECT_EQ("Brave-Browser-Nightly", path.BaseName().AsUTF8Unsafe());
#else
  EXPECT_EQ("Brave-Browser-Development", path.BaseName().AsUTF8Unsafe());
#endif
}

TEST(BraveChannelInfoMacTest, ParentDirectoryOfUserDataDirectoryTest) {
  base::FilePath path;
  EXPECT_TRUE(chrome::GetDefaultUserDataDirectory(&path));
  EXPECT_EQ("BraveSoftware", path.DirName().BaseName().AsUTF8Unsafe());
}
