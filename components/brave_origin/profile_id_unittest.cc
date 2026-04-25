/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/profile_id.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_origin {

TEST(ProfileIdTest, GetProfileId_KnownValue) {
  base::FilePath profile_path = base::FilePath::FromUTF8Unsafe(
      "/Users/test/BraveSoftware/Brave-Browser/Default");
  std::string expected_profile_id = "RGVmYXVsdA";

  std::string actual_profile_id = GetProfileId(profile_path);

  EXPECT_EQ(expected_profile_id, actual_profile_id);
}

TEST(ProfileIdTest, GetProfileId_EmptyString) {
  base::FilePath profile_path = base::FilePath::FromUTF8Unsafe("");
  std::string expected_profile_id =
      "RGVmYXVsdA";  // "Default" base64url encoded

  std::string actual_profile_id = GetProfileId(profile_path);

  EXPECT_EQ(expected_profile_id, actual_profile_id);
}

TEST(ProfileIdTest, GetProfileId_SpecialCharacters) {
  base::FilePath profile_path = base::FilePath::FromUTF8Unsafe(
      "/Users/test/BraveSoftware/Brave-Browser/Profile-1_test");
  std::string expected_profile_id = "UHJvZmlsZS0xX3Rlc3Q";

  std::string actual_profile_id = GetProfileId(profile_path);

  EXPECT_EQ(expected_profile_id, actual_profile_id);
}

TEST(ProfileIdTest, GetProfileId_UnicodeCharacters) {
  base::FilePath profile_path = base::FilePath::FromUTF8Unsafe(
      "/Users/test/BraveSoftware/Brave-Browser/Profilé");
  std::string expected_profile_id = "UHJvZmlsw6k";

  std::string actual_profile_id = GetProfileId(profile_path);

  EXPECT_EQ(expected_profile_id, actual_profile_id);
}

}  // namespace brave_origin
