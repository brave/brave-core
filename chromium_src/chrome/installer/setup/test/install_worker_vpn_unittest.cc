/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/installer/setup/install_worker.h"

#include <memory>

#include "base/test/test_reg_util_win.h"
#include "base/version.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/chrome_elf/nt_registry/nt_registry.h"
#include "chrome/installer/util/set_reg_value_work_item.h"
#include "chrome/installer/util/work_item_list.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::NiceMock;

namespace installer {

class BraveSetupInstallWorkerTest : public testing::Test {
 public:
  BraveSetupInstallWorkerTest()
      : example_version_(base::Version("1.0.0.0")),
        example_path_(FILE_PATH_LITERAL("elevation_service.exe")) {}

 protected:
  base::Version example_version_;
  base::FilePath example_path_;
};

// TODO(bsclifton): move this to a shared file. This was copied from:
// chromium_src/chrome/install_static/brave_user_data_dir_win_unittest.cc
class ScopedNTRegistryTestingOverride {
 public:
  ScopedNTRegistryTestingOverride(nt::ROOT_KEY root, const std::wstring& path)
      : root_(root) {
    EXPECT_TRUE(nt::SetTestingOverride(root_, path));
  }
  ~ScopedNTRegistryTestingOverride() {
    nt::SetTestingOverride(root_, std::wstring());
  }

 private:
  nt::ROOT_KEY root_;
};

// based on chrome/installer/setup/install_worker_unittest.cc
class MockWorkItemList : public WorkItemList {
 public:
  MockWorkItemList() {}

  MOCK_METHOD5(AddCopyTreeWorkItem,
               WorkItem*(const base::FilePath&,
                         const base::FilePath&,
                         const base::FilePath&,
                         CopyOverWriteOption,
                         const base::FilePath&));
  MOCK_METHOD1(AddCreateDirWorkItem, WorkItem*(const base::FilePath&));
  MOCK_METHOD3(AddCreateRegKeyWorkItem,
               WorkItem*(HKEY, const std::wstring&, REGSAM));
  MOCK_METHOD3(AddDeleteRegKeyWorkItem,
               WorkItem*(HKEY, const std::wstring&, REGSAM));
  MOCK_METHOD4(
      AddDeleteRegValueWorkItem,
      WorkItem*(HKEY, const std::wstring&, REGSAM, const std::wstring&));
  MOCK_METHOD2(AddDeleteTreeWorkItem,
               WorkItem*(const base::FilePath&, const base::FilePath&));
  MOCK_METHOD4(AddMoveTreeWorkItem,
               WorkItem*(const base::FilePath&,
                         const base::FilePath&,
                         const base::FilePath&,
                         MoveTreeOption));
  // Workaround for gmock problems with disambiguating between string pointers
  // and DWORD.
  WorkItem* AddSetRegValueWorkItem(HKEY a1,
                                   const std::wstring& a2,
                                   REGSAM a3,
                                   const std::wstring& a4,
                                   const std::wstring& a5,
                                   bool a6) override {
    return AddSetRegStringValueWorkItem(a1, a2, a3, a4, a5, a6);
  }

  MOCK_METHOD6(AddSetRegStringValueWorkItem,
               WorkItem*(HKEY,
                         const std::wstring&,
                         REGSAM,
                         const std::wstring&,
                         const std::wstring&,
                         bool));
  MOCK_METHOD6(AddSetRegDwordValueWorkItem,
               WorkItem*(HKEY,
                         const std::wstring&,
                         REGSAM,
                         const std::wstring&,
                         DWORD,
                         bool));
};

// Test for when registry key exists and value is already `1`.
TEST_F(BraveSetupInstallWorkerTest, CleanupAlreadyRan) {
  registry_util::RegistryOverrideManager override_manager;
  std::wstring temp;
  ASSERT_NO_FATAL_FAILURE(
      override_manager.OverrideRegistry(HKEY_LOCAL_MACHINE, &temp));
  ScopedNTRegistryTestingOverride nt_override(nt::HKLM, temp);

  // Write out a value `1` (simulating already ran)
  base::win::RegKey key(HKEY_LOCAL_MACHINE,
                        brave_vpn::kBraveVpnOneTimeServiceCleanupStoragePath,
                        KEY_ALL_ACCESS);
  DWORD cleanup_ran = 1;
  key.WriteValue(brave_vpn::kBraveVpnOneTimeServiceCleanupValue, cleanup_ran);

  NiceMock<MockWorkItemList> work_item_list;
  EXPECT_FALSE(installer::OneTimeVpnServiceCleanup(
      example_path_, example_version_, &work_item_list, true));
}

// Test for when no registry key exists yet.
TEST_F(BraveSetupInstallWorkerTest, CleanupNotRanYetNoKey) {
  registry_util::RegistryOverrideManager override_manager;
  std::wstring temp;
  ASSERT_NO_FATAL_FAILURE(
      override_manager.OverrideRegistry(HKEY_LOCAL_MACHINE, &temp));
  ScopedNTRegistryTestingOverride nt_override(nt::HKLM, temp);

  NiceMock<MockWorkItemList> work_item_list;
  EXPECT_TRUE(installer::OneTimeVpnServiceCleanup(
      example_path_, example_version_, &work_item_list, true));

  // Ensure it set `ran` to `1`
  base::win::RegKey key(HKEY_LOCAL_MACHINE,
                        brave_vpn::kBraveVpnOneTimeServiceCleanupStoragePath,
                        KEY_ALL_ACCESS);
  DWORD cleanup_ran = 0;
  LONG rv = key.ReadValueDW(brave_vpn::kBraveVpnOneTimeServiceCleanupValue,
                            &cleanup_ran);
  ASSERT_EQ(rv, ERROR_SUCCESS);
  EXPECT_EQ(cleanup_ran, 1U);
}

// Test for when registry key exists and there is a value, but it's not `1`.
TEST_F(BraveSetupInstallWorkerTest, CleanupNotRanKeyExists) {
  registry_util::RegistryOverrideManager override_manager;
  std::wstring temp;
  ASSERT_NO_FATAL_FAILURE(
      override_manager.OverrideRegistry(HKEY_LOCAL_MACHINE, &temp));
  ScopedNTRegistryTestingOverride nt_override(nt::HKLM, temp);

  // Write out a value `0` (ex: not `1`)
  base::win::RegKey key(HKEY_LOCAL_MACHINE,
                        brave_vpn::kBraveVpnOneTimeServiceCleanupStoragePath,
                        KEY_ALL_ACCESS);
  DWORD cleanup_ran = 0;
  key.WriteValue(brave_vpn::kBraveVpnOneTimeServiceCleanupValue, cleanup_ran);

  NiceMock<MockWorkItemList> work_item_list;
  EXPECT_TRUE(installer::OneTimeVpnServiceCleanup(
      example_path_, example_version_, &work_item_list, true));

  // Ensure it set `ran` to `1`
  LONG rv = key.ReadValueDW(brave_vpn::kBraveVpnOneTimeServiceCleanupValue,
                            &cleanup_ran);
  ASSERT_EQ(rv, ERROR_SUCCESS);
  EXPECT_EQ(cleanup_ran, 1U);
}

}  // namespace installer
