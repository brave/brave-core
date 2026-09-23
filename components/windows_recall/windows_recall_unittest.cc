// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/windows_recall/windows_recall.h"

#include "base/test/scoped_os_info_override_win.h"
#include "build/build_config.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace window_recall {

TEST(WindowsRecallTest, IsWindowsRecallAvailable) {
  {
    base::test::ScopedOSInfoOverride win_version(
        base::test::ScopedOSInfoOverride::Type::kWin10Home);
    EXPECT_FALSE(windows_recall::IsWindowsRecallAvailable());
  }
  {
    base::test::ScopedOSInfoOverride win_version(
        base::test::ScopedOSInfoOverride::Type::kWin10Pro);
    EXPECT_FALSE(windows_recall::IsWindowsRecallAvailable());
  }
  {
    base::test::ScopedOSInfoOverride win_version(
        base::test::ScopedOSInfoOverride::Type::kWin10Pro21H1);
    EXPECT_FALSE(windows_recall::IsWindowsRecallAvailable());
  }
  {
    base::test::ScopedOSInfoOverride win_version(
        base::test::ScopedOSInfoOverride::Type::kWinServer2016);
    EXPECT_FALSE(windows_recall::IsWindowsRecallAvailable());
  }
  {
    base::test::ScopedOSInfoOverride win_version(
        base::test::ScopedOSInfoOverride::Type::kWinServer2022);
    EXPECT_FALSE(windows_recall::IsWindowsRecallAvailable());
  }
  {
    base::test::ScopedOSInfoOverride win_version(
        base::test::ScopedOSInfoOverride::Type::kWin11HomeN);
    EXPECT_TRUE(windows_recall::IsWindowsRecallAvailable());
  }
  {
    base::test::ScopedOSInfoOverride win_version(
        base::test::ScopedOSInfoOverride::Type::kWin11Home);
    EXPECT_TRUE(windows_recall::IsWindowsRecallAvailable());
  }
  {
    base::test::ScopedOSInfoOverride win_version(
        base::test::ScopedOSInfoOverride::Type::kWin11Pro);
    EXPECT_TRUE(windows_recall::IsWindowsRecallAvailable());
  }
}

TEST(WindowsRecallTest, IsWindowsRecallDisabled) {
  {
    base::test::ScopedOSInfoOverride win_version(
        base::test::ScopedOSInfoOverride::Type::kWin10Home);
    TestingPrefServiceSimple prefs;
    windows_recall::RegisterLocalStatePrefs(prefs.registry());
    EXPECT_FALSE(
        prefs.FindPreference(windows_recall::prefs::kWindowsRecallDisabled));
    EXPECT_FALSE(windows_recall::IsWindowsRecallDisabled(&prefs));
  }
  {
    base::test::ScopedOSInfoOverride win_version(
        base::test::ScopedOSInfoOverride::Type::kWin11Home);
    TestingPrefServiceSimple prefs;
    windows_recall::RegisterLocalStatePrefs(prefs.registry());
    ASSERT_TRUE(
        prefs.GetBoolean(windows_recall::prefs::kWindowsRecallDisabled));
    EXPECT_TRUE(windows_recall::IsWindowsRecallDisabled(&prefs));
    prefs.SetBoolean(windows_recall::prefs::kWindowsRecallDisabled, false);
    // value is cached
    EXPECT_TRUE(windows_recall::IsWindowsRecallDisabled(&prefs));

    // reset the value to match the current pref
    windows_recall::test::ScopedWindowsRecallDisabledOverride override(
        prefs.GetBoolean(windows_recall::prefs::kWindowsRecallDisabled));
    EXPECT_FALSE(windows_recall::IsWindowsRecallDisabled(&prefs));
  }
}

}  // namespace window_recall
