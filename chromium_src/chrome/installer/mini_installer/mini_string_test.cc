/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <chrome/installer/mini_installer/mini_string_test.cc>

// Tests the case insensitive find support of the StackString class.
// This test used to be upstream and had to be restored in Brave to support
// delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937
TEST_F(MiniInstallerStringTest, StackStringFind) {
  static const wchar_t kTestStringSource[] = L"1234ABcD567890";
  static const wchar_t kTestStringFind[] = L"abcd";
  static const wchar_t kTestStringNotFound[] = L"80";

  StackString<MAX_PATH> str;
  EXPECT_TRUE(str.assign(kTestStringSource));
  EXPECT_EQ(str.get(),
            mini_installer::SearchStringI(str.get(), kTestStringSource));
  EXPECT_EQ(nullptr,
            mini_installer::SearchStringI(str.get(), kTestStringNotFound));
  const wchar_t* found =
      mini_installer::SearchStringI(str.get(), kTestStringFind);
  EXPECT_NE(nullptr, found);
  std::wstring check(found, _countof(kTestStringFind) - 1);
  EXPECT_EQ(0, lstrcmpi(check.c_str(), kTestStringFind));
}
