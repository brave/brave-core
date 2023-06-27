/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/base/trace_event/process_memory_dump_unittest.cc"

namespace base {
namespace trace_event {

TEST(ProcessMemoryDumpTest, IsMemoryAllocatorDumpNameInAllowlist) {
  // reset the test allow list leftover from other suites
  SetAllocatorDumpNameAllowlistForTesting(nullptr);
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

}  // namespace trace_event
}  // namespace base
