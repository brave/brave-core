/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/version_info/version_info.h"
#include "chrome/common/chrome_constants.h"
#include "components/version_info/version_info.h"
#include "testing/gtest/include/gtest/gtest.h"

using version_info::Channel;
using version_info::GetChannelString;

// We use |nightly| instead of |canary|.
TEST(BraveVersionInfoTest, ChannelStringTest) {
  EXPECT_EQ("stable", GetChannelString(Channel::STABLE));
  EXPECT_EQ("beta", GetChannelString(Channel::BETA));
  EXPECT_EQ("dev", GetChannelString(Channel::DEV));
  EXPECT_EQ("nightly", GetChannelString(Channel::CANARY));
}

TEST(BraveVersionInfoTest, VersionInfoTest) {
  // version_info::GetVersionNumber() and
  // version_info::GetBraveChromiumVersionNumber() should give different string.
  // Ex, 113.1.52.74(brave version) vs. 113.0.5672.53(chromium version).
  EXPECT_NE(version_info::GetVersionNumber(),
            version_info::GetBraveChromiumVersionNumber());

  EXPECT_EQ(std::string(chrome::kChromeVersion),
            version_info::GetVersionNumber());
}
