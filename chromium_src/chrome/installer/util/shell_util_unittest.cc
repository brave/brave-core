/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Replace this upstream test with our own. The upstream test hardcodes
// "chromium" into registry path, but we need to use the value from
// kInstallModes.
#define RegisterChromeUriSchemeForChromium \
  DISABLED_RegisterChromeUriSchemeForChromium

#include <chrome/installer/util/shell_util_unittest.cc>
#undef RegisterChromeUriSchemeForChromium

// Tests that the brave's per-channel equivalent of google-chrome:// scheme is
// registered.
TEST_F(ShellUtilRegistryTest, RegisterBraveUriScheme) {
  std::unique_ptr<WorkItemList> work_item_list(WorkItem::CreateWorkItemList());
  ShellUtil::AddChromeUriSchemeWorkItems(chrome_exe(), std::wstring(),
                                         work_item_list.get());

  ASSERT_TRUE(work_item_list->Do());

  // Verify that registry entries were added for the stable channel.
  base::win::RegKey key;
  std::wstring value;
  const std::wstring expected_open_command =
      base::StrCat({L"\"", chrome_exe().value(), L"\" --single-argument %1"});
  const std::wstring scheme_path = base::StrCat(
      {L"Software\\Classes\\",
       base::ASCIIToWide(install_static::GetDirectLaunchUrlScheme()),
       L"\\shell\\open\\command"});

  ASSERT_EQ(ERROR_SUCCESS,
            key.Open(HKEY_CURRENT_USER, scheme_path.c_str(), KEY_READ));
  EXPECT_EQ(ERROR_SUCCESS, key.ReadValue(L"", &value));
  EXPECT_EQ(expected_open_command, value);
}
