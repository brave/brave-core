/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/updater/win/win_constants.h"

#include <string_view>

#include "testing/gtest/include/gtest/gtest.h"

namespace updater {

// These values must match our Omaha 3 fork so that Omaha 4 can shut down, take
// over from, and clean up Omaha 3.
TEST(BraveWinConstantsTest, MatchOmaha3) {
  EXPECT_EQ(std::wstring_view(kShutdownEvent),
            L"{4613C8D6-D26E-4F10-B494-72CFF6F0BF0B}");
  EXPECT_EQ(std::wstring_view(kLegacyExeName), L"BraveUpdate.exe");
  EXPECT_EQ(std::wstring_view(kLegacyServiceNamePrefix), L"brave");
  EXPECT_EQ(std::wstring_view(kLegacyServiceDisplayNamePrefix),
            L"Brave Update Service");
  EXPECT_EQ(std::wstring_view(kLegacyRunValuePrefix), L"BraveSoftware Update");
  EXPECT_EQ(std::wstring_view(kLegacyTaskNamePrefixSystem),
            L"BraveSoftwareUpdateTaskMachine");
  EXPECT_EQ(std::wstring_view(kLegacyTaskNamePrefixUser),
            L"BraveSoftwareUpdateTaskUser");
  EXPECT_EQ(std::wstring_view(kLegacyGoogleUpdateAppID),
            L"{B131C935-9BE6-41DA-9599-1F776BEB8019}");
}

}  // namespace updater
