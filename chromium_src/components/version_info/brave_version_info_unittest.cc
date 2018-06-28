/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/version_info/version_info.h"

#include "testing/gtest/include/gtest/gtest.h"

using version_info::GetChannelString;
using version_info::Channel;

// We use |nightly| instead of |canary|.
TEST(BraveVersionInfoTest, ChannelStringTest) {
  EXPECT_EQ("stable", GetChannelString(Channel::STABLE));
  EXPECT_EQ("beta", GetChannelString(Channel::BETA));
  EXPECT_EQ("dev", GetChannelString(Channel::DEV));
  EXPECT_EQ("nightly", GetChannelString(Channel::CANARY));
}
