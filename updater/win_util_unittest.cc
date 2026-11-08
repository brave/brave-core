/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/updater/util/win_util.h"

#include <string>

#include "base/functional/callback_helpers.h"
#include "base/win/scoped_handle.h"
#include "chrome/updater/updater_scope.h"
#include "chrome/updater/win/user_info.h"
#include "chrome/updater/win/win_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace updater {

// Mirrors GetNamedObjectAttributes in omaha/base/utils.cc of our Omaha 3 fork.
TEST(WinUtil, LegacyShutdownEventNameMatchesOmaha3) {
  const std::wstring guid = L"{4613C8D6-D26E-4F10-B494-72CFF6F0BF0B}";
  EXPECT_EQ(GetLegacyShutdownEventName(UpdaterScope::kSystem),
            L"Global\\BraveSoftware" + guid);

  std::wstring sid;
  ASSERT_HRESULT_SUCCEEDED(GetProcessUser(nullptr, nullptr, &sid));
  ASSERT_FALSE(sid.empty());
  EXPECT_EQ(GetLegacyShutdownEventName(UpdaterScope::kUser),
            L"Global\\BraveSoftware" + sid + guid);
}

// Opens the event like GoopdateImpl::IsShutdownEventSet in our Omaha 3 fork.
TEST(WinUtil, SignalShutdownEventReachesOmaha3) {
  const std::wstring name = GetLegacyShutdownEventName(UpdaterScope::kUser);
  {
    const base::ScopedClosureRunner reset_shutdown_event(
        SignalShutdownEvent(UpdaterScope::kUser));
    base::win::ScopedHandle event(
        ::OpenEvent(SYNCHRONIZE, false, name.c_str()));
    ASSERT_TRUE(event.is_valid());
    EXPECT_EQ(::WaitForSingleObject(event.get(), 0), WAIT_OBJECT_0);
    EXPECT_TRUE(IsShutdownEventSignaled(UpdaterScope::kUser));
  }
  EXPECT_FALSE(IsShutdownEventSignaled(UpdaterScope::kUser));
}

}  // namespace updater
