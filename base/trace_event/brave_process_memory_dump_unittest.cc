/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/trace_event/memory_infra_background_allowlist.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base::trace_event {

TEST(BraveProcessMemoryDumpTest, IsMemoryAllocatorDumpNameInAllowlist) {
  // reset the test allow list leftover from other suites
  SetAllocatorDumpNameAllowlistForTesting({});
  // This is to ensure we capture upstream name pattern changes in
  // value_store::LeveldbValueStore::OnMemoryDump
  ASSERT_TRUE(IsMemoryAllocatorDumpNameInAllowlist(
      "extensions/value_store/Extensions.Database.Open.Settings/0x1234"));
  ASSERT_TRUE(IsMemoryAllocatorDumpNameInAllowlist(
      "extensions/value_store/Extensions.Database.Open.Rules/0x1234"));
  ASSERT_TRUE(IsMemoryAllocatorDumpNameInAllowlist(
      "extensions/value_store/Extensions.Database.Open.State/0x1234"));
  ASSERT_TRUE(IsMemoryAllocatorDumpNameInAllowlist(
      "extensions/value_store/Extensions.Database.Open.Scripts/0x1234"));
  ASSERT_TRUE(IsMemoryAllocatorDumpNameInAllowlist(
      "extensions/value_store/Extensions.Database.Open.WebAppsLockScreen/"
      "0x1234"));
  ASSERT_TRUE(IsMemoryAllocatorDumpNameInAllowlist(
      "extensions/value_store/Extensions.Database.Open/0x1234"));

  EXPECT_TRUE(IsMemoryAllocatorDumpNameInAllowlist(
      "extensions/value_store/Extensions.Database.Open.BraveWallet/0x1234"));
}

}  // namespace base::trace_event
