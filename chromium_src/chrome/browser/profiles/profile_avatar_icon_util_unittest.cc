// Copyright 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/profiles/profile_avatar_icon_util.h"

#include "base/values.h"
#include "chrome/grit/theme_resources.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(ProfileUtilTest, KeepChromiumChoice) {
  // Test that a legacy chromium avatar choice will
  // still display the chromium avatar
  size_t chromium_avatar_index = 42;
  int expected = IDR_PROFILE_AVATAR_42;
  int actual = profiles::GetDefaultAvatarIconResourceIDAtIndex(
      chromium_avatar_index);
  EXPECT_EQ(actual, expected);
}

TEST(ProfileUtilTest, BraveAvatarIconChoices) {
  // Test that the avatar icon choices presented to the user are brave's.
  std::unique_ptr<base::ListValue> avatars(
      profiles::GetCustomProfileAvatarIconsAndLabels(0));

  const size_t expected_selectable_avatar_count =
      profiles::kBraveDefaultAvatarIconsCount;
  const size_t actual_selectable_avatar_count = avatars->GetSize();

  // Avatars are Brave's, not Chromium's
  EXPECT_EQ(actual_selectable_avatar_count,
      expected_selectable_avatar_count);
}

TEST(ProfileUtilTest, RandomIconNeverFirstIcon) {
  // Test that for Brave, a call to get a random
  // avatar icon will *never* get the placeholder icon.

  // Unfortunately this uses an implementation detail that may change,
  // and if so this test will need updating.
  // It checks that the 'ModernAvatarIconStartIndex' is greater than
  // the placeholder index (which for brave is always the first item).
  size_t placeholder_index = profiles::GetPlaceholderAvatarIndex();
  size_t random_start_index = profiles::GetModernAvatarIconStartIndex();
  EXPECT_GT(random_start_index, placeholder_index);
}

}  // namespace

