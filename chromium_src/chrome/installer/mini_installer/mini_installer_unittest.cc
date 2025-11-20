/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file contains code that used to be upstream and had to be restored in
// Brave to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937

#include <chrome/installer/mini_installer/mini_installer_unittest.cc>

namespace mini_installer {

namespace {

#define PREVIOUS_VERSION L"62.0.1234.0"
constexpr wchar_t kPreviousVersion[] = PREVIOUS_VERSION;

class FakeConfiguration : public Configuration {
 public:
  FakeConfiguration() { previous_version_ = kPreviousVersion; }
};

}  // namespace

// A test harness for GetPreviousSetupExePath.
class GetPreviousSetupExePathTest : public ::testing::Test {
 public:
  GetPreviousSetupExePathTest(const GetPreviousSetupExePathTest&) = delete;
  GetPreviousSetupExePathTest& operator=(const GetPreviousSetupExePathTest&) =
      delete;

 protected:
  GetPreviousSetupExePathTest() = default;
  ~GetPreviousSetupExePathTest() override = default;

  void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_CURRENT_USER));
  }

  const Configuration& configuration() const { return configuration_; }

  // Writes |path| to the registry in Chrome's ClientState...UninstallString
  // value.
  void SetPreviousSetup(const wchar_t* path) {
    base::win::RegKey key;
    const install_static::InstallDetails& details =
        install_static::InstallDetails::Get();
    ASSERT_EQ(
        key.Create(HKEY_CURRENT_USER, details.GetClientStateKeyPath().c_str(),
                   KEY_SET_VALUE | KEY_WOW64_32KEY),
        ERROR_SUCCESS);
    ASSERT_EQ(key.WriteValue(installer::kUninstallStringField, path),
              ERROR_SUCCESS);
  }

 private:
  registry_util::RegistryOverrideManager registry_override_manager_;
  FakeConfiguration configuration_;
};

// Tests that the path is returned.
TEST_F(GetPreviousSetupExePathTest, SimpleTest) {
  static constexpr wchar_t kSetupExePath[] =
      L"C:\\SomePath\\To\\" PREVIOUS_VERSION L"\\setup.exe";
  ASSERT_NO_FATAL_FAILURE(SetPreviousSetup(kSetupExePath));

  StackString<MAX_PATH> path;
  ProcessExitResult result =
      GetPreviousSetupExePath(configuration(), path.get(), path.capacity());
  ASSERT_TRUE(result.IsSuccess());
  EXPECT_STREQ(path.get(), kSetupExePath);
}

// Tests that quotes are removed, if present.
TEST_F(GetPreviousSetupExePathTest, QuoteStripping) {
  static constexpr wchar_t kSetupExePath[] =
      L"C:\\SomePath\\To\\" PREVIOUS_VERSION L"\\setup.exe";
  std::wstring quoted_path(L"\"");
  quoted_path += kSetupExePath;
  quoted_path += L"\"";
  ASSERT_NO_FATAL_FAILURE(SetPreviousSetup(quoted_path.c_str()));

  StackString<MAX_PATH> path;
  ProcessExitResult result =
      GetPreviousSetupExePath(configuration(), path.get(), path.capacity());
  ASSERT_TRUE(result.IsSuccess());
  EXPECT_STREQ(path.get(), kSetupExePath);
}

}  // namespace mini_installer
