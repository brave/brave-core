/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// The tests in this file ensure that our code satisfies some of upstream's
// implicit requirements.

#if defined(OFFICIAL_BUILD)
#error "This test should only be compiled in non-official builds"
#endif

#include "chrome/installer/mini_installer/mini_installer_constants.h"

#include "chrome/install_static/install_modes.h"
#include "chrome/installer/mini_installer/mini_string.h"
#include "testing/gtest/include/gtest/gtest.h"

using install_static::kProductPathName;

namespace mini_installer {

namespace {

const wchar_t* GetLastPathComponent(const wchar_t* path) {
  return GetNameFromPathExt(path, SafeStrLen(path, 1024));
}

}  // namespace

TEST(BraveMiniInstallerConstantsTest, ClientsKeyBaseEndsWithProductPathName) {
  EXPECT_STREQ(GetLastPathComponent(kClientsKeyBase), kProductPathName);
}

TEST(BraveMiniInstallerConstantsTest, ClientStateKeyBaseEndsWithProductPathName) {
  EXPECT_STREQ(GetLastPathComponent(kClientStateKeyBase), kProductPathName);
}

TEST(BraveMiniInstallerConstantsTest, CleanupRegistryKeyEndsWithProductPathName) {
  EXPECT_STREQ(GetLastPathComponent(kCleanupRegistryKey), kProductPathName);
}

}  // namespace mini_installer
