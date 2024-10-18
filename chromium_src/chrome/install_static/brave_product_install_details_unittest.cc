/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "chrome/install_static/product_install_details.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/i18n/case_conversion.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/test/test_reg_util_win.h"
#include "base/win/registry.h"
#include "base/win/windows_version.h"
#include "chrome/chrome_elf/nt_registry/nt_registry.h"
#include "chrome/install_static/install_constants.h"
#include "chrome/install_static/install_details.h"
#include "chrome/install_static/install_modes.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Eq;
using ::testing::StrEq;

namespace install_static {

namespace {

TEST(ProductInstallDetailsTest, GetInstallSuffix) {
  const std::pair<const wchar_t*, const wchar_t*> kData1[] = {
      {L"\\Application", L""},
      {L"\\Application\\", L""},
      {L"-Blorf\\Application", L"-Blorf"},
      {L"-Blorf\\Application\\", L"-Blorf"},
  };
  for (const auto& data : kData1) {
    const std::wstring path = base::StrCat({kProductPathName, data.first});
    EXPECT_EQ(std::wstring(data.second), GetInstallSuffix(path)) << path;
  }

  const std::pair<const wchar_t*, const wchar_t*> kData2[] = {
      {L"\\Application", L""},
      {L"\\Application\\", L""},
      {L"-Blorf\\Application", L"-Blorf"},
      {L"-Blorf\\Application\\", L"-Blorf"},
  };
  for (const auto& data : kData2) {
    const std::wstring path =
        base::StrCat({L"\\", kProductPathName, data.first});
    EXPECT_EQ(std::wstring(data.second), GetInstallSuffix(path)) << path;
  }

  const std::pair<const wchar_t*, const wchar_t*> kData3[] = {
      {L"-Blorf\\Application\\foo.exe", L"-Blorf"},
      {L"\\Application\\foo.exe", L""},
  };
  for (const auto& data : kData3) {
    const std::wstring path =
        base::StrCat({L"C:\\foo\\", kProductPathName, data.first});
    EXPECT_EQ(std::wstring(data.second), GetInstallSuffix(path)) << path;
  }
}

struct TestData {
  const wchar_t* path;
  InstallConstantIndex index;
  bool system_level;
  const wchar_t* channel;
};

#if defined(OFFICIAL_BUILD)
constexpr TestData kTestData[] = {
    {
        L"C:\\Program Files (x86)\\BraveSoftware\\Brave-Browser\\Application"
        L"\\brave.exe",
        STABLE_INDEX, true, L"",
    },
    {
        L"C:\\Users\\user\\AppData\\Local\\BraveSoftware\\Brave-Browser"
        L"\\Application\\brave.exe",
        STABLE_INDEX, false, L"",
    },
    {
        L"C:\\Program Files (x86)\\BraveSoftware\\Brave-Browser-Beta"
        L"\\Application\\brave.exe",
        BETA_INDEX, true, L"beta",
    },
    {
        L"C:\\Users\\user\\AppData\\Local\\BraveSoftware\\Brave-Browser-Beta"
        L"\\Application\\brave.exe",
        BETA_INDEX, false, L"beta",
    },
    {
        L"C:\\Program Files (x86)\\BraveSoftware\\Brave-Browser-Dev"
        L"\\Application\\brave.exe",
        DEV_INDEX, true, L"dev",
    },
    {
        L"C:\\Users\\user\\AppData\\Local\\BraveSoftware\\Brave-Browser-Dev"
        L"\\Application\\brave.exe",
        DEV_INDEX, false, L"dev",
    },
    {
        L"C:\\Users\\user\\AppData\\Local\\BraveSoftware\\Brave-Browser-Nightly"
        L"\\Application\\brave.exe",
        NIGHTLY_INDEX, false, L"nightly",
    },
    {
        L"C:\\Users\\user\\AppData\\Local\\BraveSoftware\\Brave-Browser-Nightly"
        L"\\Application\\brave.exe",
        NIGHTLY_INDEX, false, L"nightly",
    },
};
#else   // OFFICIAL_BUILD
constexpr TestData kTestData[] = {
    {
        L"C:\\Program Files (x86)\\BraveSoftware\\Brave-Browser-Development"
        L"\\Application\\brave.exe",
        DEVELOPER_INDEX,
        true,
        L"",
    },
    {
        L"C:\\Users\\user\\AppData\\Local\\BraveSoftware\\Brave-Browser-"
        L"Development\\Application\\brave.exe",
        DEVELOPER_INDEX,
        false,
        L"",
    },
};
#endif  // !OFFICIAL_BUILD

}  // namespace

// Test that MakeProductDetails properly sniffs out an install's details.
class MakeProductDetailsTest : public testing::TestWithParam<TestData> {
 public:
  MakeProductDetailsTest(const MakeProductDetailsTest&) = delete;
  MakeProductDetailsTest& operator=(const MakeProductDetailsTest&) = delete;

 protected:
  MakeProductDetailsTest()
      : test_data_(GetParam()),
        root_key_(test_data_.system_level ? HKEY_LOCAL_MACHINE
                                          : HKEY_CURRENT_USER),
        nt_root_key_(test_data_.system_level ? nt::HKLM : nt::HKCU) {
  }

  ~MakeProductDetailsTest() {
    nt::SetTestingOverride(nt_root_key_, std::wstring());
  }

  void SetUp() override {
    std::wstring path;
    ASSERT_NO_FATAL_FAILURE(
        override_manager_.OverrideRegistry(root_key_, &path));
    nt::SetTestingOverride(nt_root_key_, path);
  }

  const TestData& test_data() const { return test_data_; }

  void SetAp(const wchar_t* value) {
    ASSERT_THAT(base::win::RegKey(root_key_, GetClientStateKeyPath().c_str(),
                                  KEY_WOW64_32KEY | KEY_SET_VALUE)
                    .WriteValue(L"ap", value),
                Eq(ERROR_SUCCESS));
  }

  void SetCohortName(const wchar_t* value) {
    ASSERT_THAT(
        base::win::RegKey(root_key_,
                          GetClientStateKeyPath().append(L"\\cohort").c_str(),
                          KEY_WOW64_32KEY | KEY_SET_VALUE)
            .WriteValue(L"name", value),
        Eq(ERROR_SUCCESS));
  }

 private:
  // Returns the registry path for the product's ClientState key.
  std::wstring GetClientStateKeyPath() {
    std::wstring result(L"Software\\");
#if defined(OFFICIAL_BUILD)
      result.append(L"BraveSoftware\\Update\\ClientState\\");
      result.append(kInstallModes[test_data().index].app_guid);
#else
      result.append(kProductPathName);
#endif
    return result;
  }

  registry_util::RegistryOverrideManager override_manager_;
  const TestData& test_data_;
  HKEY root_key_;
  nt::ROOT_KEY nt_root_key_;
};

// Test that the install mode is sniffed properly based on the path.
TEST_P(MakeProductDetailsTest, Index) {
  std::unique_ptr<PrimaryInstallDetails> details(
      MakeProductDetails(test_data().path));
  EXPECT_THAT(details->install_mode_index(), Eq(test_data().index));
}

// Test that user/system level is sniffed properly based on the path.
TEST_P(MakeProductDetailsTest, SystemLevel) {
  std::unique_ptr<PrimaryInstallDetails> details(
      MakeProductDetails(test_data().path));
  EXPECT_THAT(details->system_level(), Eq(test_data().system_level));
}

// Test that the default channel is sniffed properly based on the path.
TEST_P(MakeProductDetailsTest, DefaultChannel) {
  std::unique_ptr<PrimaryInstallDetails> details(
      MakeProductDetails(test_data().path));
  EXPECT_THAT(details->channel(), StrEq(test_data().channel));
}

// This test is only valid for brands that integrate with Google Update.
#if !defined(OFFICIAL_BUILD)
#define MAYBE_UpdateAp DISABLED_UpdateAp
#else
#define MAYBE_UpdateAp UpdateAp
#endif
// Test that the "ap" value is cached during initialization.
TEST_P(MakeProductDetailsTest, MAYBE_UpdateAp) {
  // With no value in the registry, the ap value should be empty.
  {
    std::unique_ptr<PrimaryInstallDetails> details(
        MakeProductDetails(test_data().path));
    EXPECT_THAT(details->update_ap(), StrEq(L""));
  }

  // And with a value, it should have ... the value.
  static constexpr wchar_t kCrookedMoon[] = L"CrookedMoon";
  SetAp(kCrookedMoon);
  {
    std::unique_ptr<PrimaryInstallDetails> details(
        MakeProductDetails(test_data().path));
    EXPECT_THAT(details->update_ap(), StrEq(kCrookedMoon));
  }
}

// Disable this test on development mode, as this test is only valid for
// branches that integrate Google Update.
#if !defined(OFFICIAL_BUILD)
#define MAYBE_UpdateCohortName DISABLED_UpdateCohortName
#else
#define MAYBE_UpdateCohortName UpdateCohortName
#endif
// Test that the cohort name is cached during initialization.
TEST_P(MakeProductDetailsTest, MAYBE_UpdateCohortName) {
  // With no value in the registry, the cohort name should be empty.
  {
    std::unique_ptr<PrimaryInstallDetails> details(
        MakeProductDetails(test_data().path));
    EXPECT_THAT(details->update_cohort_name(), StrEq(L""));
  }

  // And with a value, it should have ... the value.
  static constexpr wchar_t kPhony[] = L"Phony";
  SetCohortName(kPhony);
  {
    std::unique_ptr<PrimaryInstallDetails> details(
        MakeProductDetails(test_data().path));
    EXPECT_THAT(details->update_cohort_name(), StrEq(kPhony));
  }
}

INSTANTIATE_TEST_SUITE_P(All,
                         MakeProductDetailsTest,
                         testing::ValuesIn(kTestData));

}  // namespace install_static
