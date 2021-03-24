// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <objbase.h>

#include <tuple>

#include "base/macros.h"
#include "base/stl_util.h"
#include "base/test/test_reg_util_win.h"
#include "chrome/chrome_elf/nt_registry/nt_registry.h"
#include "chrome/install_static/install_details.h"
#include "chrome/install_static/install_modes.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/test/scoped_install_details.h"
#include "components/version_info/channel.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::ElementsAre;
using ::testing::StrCaseEq;
using version_info::Channel;

namespace install_static {

// Tests the MatchPattern function in the install_static library.
TEST(InstallStaticTest, MatchPattern) {
  EXPECT_TRUE(MatchPattern(L"", L""));
  EXPECT_TRUE(MatchPattern(L"", L"*"));
  EXPECT_FALSE(MatchPattern(L"", L"*a"));
  EXPECT_FALSE(MatchPattern(L"", L"abc"));
  EXPECT_TRUE(MatchPattern(L"Hello1234", L"He??o*1*"));
  EXPECT_TRUE(MatchPattern(L"Foo", L"F*?"));
  EXPECT_TRUE(MatchPattern(L"Foo", L"F*"));
  EXPECT_FALSE(MatchPattern(L"Foo", L"F*b"));
  EXPECT_TRUE(MatchPattern(L"abcd", L"*c*d"));
  EXPECT_TRUE(MatchPattern(L"abcd", L"*?c*d"));
  EXPECT_FALSE(MatchPattern(L"abcd", L"abcd*efgh"));
  EXPECT_TRUE(MatchPattern(L"foobarabc", L"*bar*"));
}

// Tests the install_static::GetSwitchValueFromCommandLine function.
TEST(InstallStaticTest, GetSwitchValueFromCommandLineTest) {
  // Simple case with one switch.
  std::wstring value =
      GetSwitchValueFromCommandLine(L"c:\\temp\\bleh.exe --type=bar", L"type");
  EXPECT_EQ(L"bar", value);

  // Multiple switches with trailing spaces between them.
  value = GetSwitchValueFromCommandLine(
      L"c:\\temp\\bleh.exe --type=bar  --abc=def bleh", L"abc");
  EXPECT_EQ(L"def", value);

  // Multiple switches with trailing spaces and tabs between them.
  value = GetSwitchValueFromCommandLine(
      L"c:\\temp\\bleh.exe --type=bar \t\t\t --abc=def bleh", L"abc");
  EXPECT_EQ(L"def", value);

  // Non existent switch.
  value = GetSwitchValueFromCommandLine(
      L"c:\\temp\\bleh.exe --foo=bar  --abc=def bleh", L"type");
  EXPECT_EQ(L"", value);

  // Non existent switch.
  value = GetSwitchValueFromCommandLine(L"c:\\temp\\bleh.exe", L"type");
  EXPECT_EQ(L"", value);

  // Non existent switch.
  value =
      GetSwitchValueFromCommandLine(L"c:\\temp\\bleh.exe type=bar", L"type");
  EXPECT_EQ(L"", value);

  // Trailing spaces after the switch.
  value = GetSwitchValueFromCommandLine(
      L"c:\\temp\\bleh.exe --type=bar      \t\t", L"type");
  EXPECT_EQ(L"bar", value);

  // Multiple switches with trailing spaces and tabs between them.
  value = GetSwitchValueFromCommandLine(
      L"c:\\temp\\bleh.exe --type=bar      \t\t --foo=bleh", L"foo");
  EXPECT_EQ(L"bleh", value);

  // Nothing after a switch.
  value = GetSwitchValueFromCommandLine(L"c:\\temp\\bleh.exe --type=", L"type");
  EXPECT_TRUE(value.empty());

  // Whitespace after a switch.
  value =
      GetSwitchValueFromCommandLine(L"c:\\temp\\bleh.exe --type= ", L"type");
  EXPECT_TRUE(value.empty());

  // Just tabs after a switch.
  value = GetSwitchValueFromCommandLine(L"c:\\temp\\bleh.exe --type=\t\t\t",
                                        L"type");
  EXPECT_TRUE(value.empty());
}

TEST(InstallStaticTest, SpacesAndQuotesInCommandLineArguments) {
  std::vector<std::wstring> tokenized;

  tokenized = TokenizeCommandLineToArray(L"\"C:\\a\\b.exe\"");
  ASSERT_EQ(1u, tokenized.size());
  EXPECT_EQ(L"C:\\a\\b.exe", tokenized[0]);

  tokenized = TokenizeCommandLineToArray(L"x.exe");
  ASSERT_EQ(1u, tokenized.size());
  EXPECT_EQ(L"x.exe", tokenized[0]);

  tokenized = TokenizeCommandLineToArray(L"\"c:\\with space\\something.exe\"");
  ASSERT_EQ(1u, tokenized.size());
  EXPECT_EQ(L"c:\\with space\\something.exe", tokenized[0]);

  tokenized = TokenizeCommandLineToArray(L"\"C:\\a\\b.exe\" arg");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"C:\\a\\b.exe", tokenized[0]);
  EXPECT_EQ(L"arg", tokenized[1]);

  tokenized = TokenizeCommandLineToArray(L"\"C:\\with space\\b.exe\" \"arg\"");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"C:\\with space\\b.exe", tokenized[0]);
  EXPECT_EQ(L"arg", tokenized[1]);

  tokenized = TokenizeCommandLineToArray(L"\"C:\\a\\b.exe\" c:\\tmp\\");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"C:\\a\\b.exe", tokenized[0]);
  EXPECT_EQ(L"c:\\tmp\\", tokenized[1]);

  tokenized =
      TokenizeCommandLineToArray(L"\"C:\\a\\b.exe\" \"c:\\some file path\\\"");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"C:\\a\\b.exe", tokenized[0]);
  EXPECT_EQ(L"c:\\some file path\"", tokenized[1]);

  tokenized = TokenizeCommandLineToArray(
      L"\"C:\\with space\\b.exe\" \\\\x\\\\ \\\\y\\\\");
  ASSERT_EQ(3u, tokenized.size());
  EXPECT_EQ(L"C:\\with space\\b.exe", tokenized[0]);
  EXPECT_EQ(L"\\\\x\\\\", tokenized[1]);
  EXPECT_EQ(L"\\\\y\\\\", tokenized[2]);

  tokenized = TokenizeCommandLineToArray(
      L"\"C:\\with space\\b.exe\" \"\\\\space quoted\\\\\"");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"C:\\with space\\b.exe", tokenized[0]);
  EXPECT_EQ(L"\\\\space quoted\\", tokenized[1]);

  tokenized = TokenizeCommandLineToArray(
      L"\"C:\\with space\\b.exe\" --stuff    -x -Y   \"c:\\some thing\\\"    "
      L"weewaa    ");
  ASSERT_EQ(5u, tokenized.size());
  EXPECT_EQ(L"C:\\with space\\b.exe", tokenized[0]);
  EXPECT_EQ(L"--stuff", tokenized[1]);
  EXPECT_EQ(L"-x", tokenized[2]);
  EXPECT_EQ(L"-Y", tokenized[3]);
  EXPECT_EQ(L"c:\\some thing\"    weewaa    ", tokenized[4]);

  tokenized = TokenizeCommandLineToArray(
      L"\"C:\\with space\\b.exe\" --stuff=\"d:\\stuff and things\"");
  EXPECT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"C:\\with space\\b.exe", tokenized[0]);
  EXPECT_EQ(L"--stuff=d:\\stuff and things", tokenized[1]);

  tokenized = TokenizeCommandLineToArray(
      L"\"C:\\with space\\b.exe\" \\\\\\\"\"");
  EXPECT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"C:\\with space\\b.exe", tokenized[0]);
  EXPECT_EQ(L"\\\"", tokenized[1]);
}

// Test cases from
// https://blogs.msdn.microsoft.com/oldnewthing/20100917-00/?p=12833.
TEST(InstallStaticTest, SpacesAndQuotesOldNewThing) {
  std::vector<std::wstring> tokenized;

  tokenized = TokenizeCommandLineToArray(L"program.exe \"hello there.txt\"");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"program.exe", tokenized[0]);
  EXPECT_EQ(L"hello there.txt", tokenized[1]);

  tokenized =
      TokenizeCommandLineToArray(L"program.exe \"C:\\Hello there.txt\"");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"program.exe", tokenized[0]);
  EXPECT_EQ(L"C:\\Hello there.txt", tokenized[1]);

  tokenized =
      TokenizeCommandLineToArray(L"program.exe \"hello\\\"there\"");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"program.exe", tokenized[0]);
  EXPECT_EQ(L"hello\"there", tokenized[1]);

  tokenized =
      TokenizeCommandLineToArray(L"program.exe \"hello\\\\\"");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"program.exe", tokenized[0]);
  EXPECT_EQ(L"hello\\", tokenized[1]);
}

// Test cases from
// http://www.windowsinspired.com/how-a-windows-programs-splits-its-command-line-into-individual-arguments/.
// These are mostly about the special handling of argv[0], which uses different
// quoting than the rest of the arguments.
TEST(InstallStaticTest, SpacesAndQuotesWindowsInspired) {
  std::vector<std::wstring> tokenized;

  tokenized = TokenizeCommandLineToArray(
      L"\"They said \"you can't do this!\", didn't they?\"");
  ASSERT_EQ(5u, tokenized.size());
  EXPECT_EQ(L"They said ", tokenized[0]);
  EXPECT_EQ(L"you", tokenized[1]);
  EXPECT_EQ(L"can't", tokenized[2]);
  EXPECT_EQ(L"do", tokenized[3]);
  EXPECT_EQ(L"this!, didn't they?", tokenized[4]);

  tokenized = TokenizeCommandLineToArray(
      L"test.exe \"c:\\Path With Spaces\\Ending In Backslash\\\" Arg2 Arg3");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"test.exe", tokenized[0]);
  EXPECT_EQ(L"c:\\Path With Spaces\\Ending In Backslash\" Arg2 Arg3",
            tokenized[1]);

  tokenized = TokenizeCommandLineToArray(
      L"FinalProgram.exe \"first second \"\"embedded quote\"\" third\"");
  ASSERT_EQ(4u, tokenized.size());
  EXPECT_EQ(L"FinalProgram.exe", tokenized[0]);
  EXPECT_EQ(L"first second \"embedded", tokenized[1]);
  EXPECT_EQ(L"quote", tokenized[2]);
  EXPECT_EQ(L"third", tokenized[3]);

  tokenized = TokenizeCommandLineToArray(
      L"\"F\"i\"r\"s\"t S\"e\"c\"o\"n\"d\" T\"h\"i\"r\"d\"");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"F", tokenized[0]);
  EXPECT_EQ(L"irst Second Third", tokenized[1]);

  tokenized = TokenizeCommandLineToArray(L"F\"\"ir\"s\"\"t \\\"Second Third\"");
  ASSERT_EQ(3u, tokenized.size());
  EXPECT_EQ(L"F\"\"ir\"s\"\"t", tokenized[0]);
  EXPECT_EQ(L"\"Second", tokenized[1]);
  EXPECT_EQ(L"Third", tokenized[2]);

  tokenized = TokenizeCommandLineToArray(L"  Something Else");
  ASSERT_EQ(3u, tokenized.size());
  EXPECT_EQ(L"", tokenized[0]);
  EXPECT_EQ(L"Something", tokenized[1]);
  EXPECT_EQ(L"Else", tokenized[2]);

  tokenized = TokenizeCommandLineToArray(L" Something Else");
  ASSERT_EQ(3u, tokenized.size());
  EXPECT_EQ(L"", tokenized[0]);
  EXPECT_EQ(L"Something", tokenized[1]);
  EXPECT_EQ(L"Else", tokenized[2]);

  tokenized = TokenizeCommandLineToArray(L"\"123 456\tabc\\def\"ghi");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"123 456\tabc\\def", tokenized[0]);
  EXPECT_EQ(L"ghi", tokenized[1]);

  tokenized = TokenizeCommandLineToArray(L"123\"456\"\tabc");
  ASSERT_EQ(2u, tokenized.size());
  EXPECT_EQ(L"123\"456\"", tokenized[0]);
  EXPECT_EQ(L"abc", tokenized[1]);
}

TEST(InstallStaticTest, BrowserProcessTest) {
  EXPECT_FALSE(IsProcessTypeInitialized());
  InitializeProcessType();
  EXPECT_TRUE(IsBrowserProcess());
}

class InstallStaticUtilTest
    : public ::testing::TestWithParam<
          std::tuple<InstallConstantIndex, const char*>> {
 protected:
  InstallStaticUtilTest()
      : system_level_(std::string(std::get<1>(GetParam())) != "user"),
        scoped_install_details_(system_level_, std::get<0>(GetParam())),
        mode_(&InstallDetails::Get().mode()),
        root_key_(system_level_ ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER),
        nt_root_key_(system_level_ ? nt::HKLM : nt::HKCU) {}

  void SetUp() override {
    ASSERT_TRUE(!system_level_ || mode_->supports_system_level);
    base::string16 path;
    ASSERT_NO_FATAL_FAILURE(
        override_manager_.OverrideRegistry(root_key_, &path));
    nt::SetTestingOverride(nt_root_key_, path);
  }

  void TearDown() override {
    nt::SetTestingOverride(nt_root_key_, base::string16());
  }

  bool system_level() const { return system_level_; }

  const wchar_t* default_channel() const { return mode_->default_channel_name; }

  void SetUsageStat(DWORD value, bool medium) {
    ASSERT_TRUE(!medium || system_level_);
    ASSERT_EQ(ERROR_SUCCESS,
              base::win::RegKey(root_key_, GetUsageStatsKeyPath(medium).c_str(),
                                KEY_SET_VALUE | KEY_WOW64_32KEY)
                  .WriteValue(L"usagestats", value));
  }

  void SetMetricsReportingPolicy(DWORD value) {
#if defined(OFFICIAL_BUILD)
    static constexpr wchar_t kPolicyKey[] =
        L"Software\\Policies\\BraveSoftware\\Brave-Browser";
#else
    static constexpr wchar_t kPolicyKey[] =
        L"Software\\Policies\\BraveSoftware\\Brave-Browser-Development";
#endif

    ASSERT_EQ(ERROR_SUCCESS,
              base::win::RegKey(root_key_, kPolicyKey, KEY_SET_VALUE)
                  .WriteValue(L"MetricsReportingEnabled", value));
  }

 private:
  // Returns the registry path for the key holding the product's usagestats
  // value. |medium| = true returns the path for ClientStateMedium.
  std::wstring GetUsageStatsKeyPath(bool medium) {
    EXPECT_TRUE(!medium || system_level_);

    std::wstring result(L"Software\\");
#if defined(OFFICIAL_BUILD)
      result.append(L"BraveSoftware\\Update\\ClientState");
      if (medium)
        result.append(L"Medium");
      result.push_back(L'\\');
      result.append(mode_->app_guid);
#else
      result.append(kProductPathName);
#endif
    return result;
  }

  const bool system_level_;
  const ScopedInstallDetails scoped_install_details_;
  const InstallConstants* mode_;
  const HKEY root_key_;
  const nt::ROOT_KEY nt_root_key_;
  registry_util::RegistryOverrideManager override_manager_;

  DISALLOW_COPY_AND_ASSIGN(InstallStaticUtilTest);
};

TEST_P(InstallStaticUtilTest, GetChromeInstallSubDirectory) {
#if defined(OFFICIAL_BUILD)
  // The directory strings for the brand's install modes; parallel to
  // kInstallModes.
  static constexpr const wchar_t* kInstallDirs[] = {
      L"BraveSoftware\\Brave-Browser",
      L"BraveSoftware\\Brave-Browser-Beta",
      L"BraveSoftware\\Brave-Browser-Dev",
      L"BraveSoftware\\Brave-Browser-Nightly",
  };
#else
  // The directory strings for the brand's install modes; parallel to
  // kInstallModes.
  static constexpr const wchar_t* kInstallDirs[] = {
      L"BraveSoftware\\Brave-Browser-Development",
  };
#endif
  static_assert(base::size(kInstallDirs) == NUM_INSTALL_MODES,
                "kInstallDirs out of date.");
  EXPECT_THAT(GetChromeInstallSubDirectory(),
              StrCaseEq(kInstallDirs[std::get<0>(GetParam())]));
}

TEST_P(InstallStaticUtilTest, GetRegistryPath) {
#if defined(OFFICIAL_BUILD)
  // The registry path strings for the brand's install modes; parallel to
  // kInstallModes.
  static constexpr const wchar_t* kRegistryPaths[] = {
      L"Software\\BraveSoftware\\Brave-Browser",
      L"Software\\BraveSoftware\\Brave-Browser-Beta",
      L"Software\\BraveSoftware\\Brave-Browser-Dev",
      L"Software\\BraveSoftware\\Brave-Browser-Nightly",
  };
#else
  // The registry path strings for the brand's install modes; parallel to
  // kInstallModes.
  static constexpr const wchar_t* kRegistryPaths[] = {
      L"Software\\BraveSoftware\\Brave-Browser-Development",
  };
#endif
  static_assert(base::size(kRegistryPaths) == NUM_INSTALL_MODES,
                "kRegistryPaths out of date.");
  EXPECT_THAT(GetRegistryPath(),
              StrCaseEq(kRegistryPaths[std::get<0>(GetParam())]));
}

TEST_P(InstallStaticUtilTest, GetUninstallRegistryPath) {
#if defined(OFFICIAL_BUILD)
  // The uninstall registry path strings for the brand's install modes; parallel
  // to kInstallModes.
  static constexpr const wchar_t* kUninstallRegistryPaths[] = {
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"  // (cont'd)
      L"BraveSoftware Brave-Browser",
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"  // (cont'd)
      L"BraveSoftware Brave-Browser-Beta",
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"  // (cont'd)
      L"BraveSoftware Brave-Browser-Dev",
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"  // (cont'd)
      L"BraveSoftware Brave-Browser-Nightly",
  };
#else
  // The registry path strings for the brand's install modes; parallel to
  // kInstallModes.
  static constexpr const wchar_t* kUninstallRegistryPaths[] = {
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"  // (cont'd)
      L"BraveSoftware Brave-Browser-Development",
  };
#endif
  static_assert(base::size(kUninstallRegistryPaths) == NUM_INSTALL_MODES,
                "kUninstallRegistryPaths out of date.");
  EXPECT_THAT(GetUninstallRegistryPath(),
              StrCaseEq(kUninstallRegistryPaths[std::get<0>(GetParam())]));
}

TEST_P(InstallStaticUtilTest, GetAppGuid) {
#if defined(OFFICIAL_BUILD)
  // The app guids for the brand's install modes; parallel to kInstallModes.
  static constexpr const wchar_t* kAppGuids[] = {
      L"{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}",  // Brave-Browser.
      L"{103BD053-949B-43A8-9120-2E424887DE11}",  // Brave-Browser-Beta.
      L"{CB2150F2-595F-4633-891A-E39720CE0531}",  // Brave-Browser-Dev.
      L"{C6CB981E-DB30-4876-8639-109F8933582C}",  // Brave-Browser-Nightly.
  };
  static_assert(base::size(kAppGuids) == NUM_INSTALL_MODES,
                "kAppGuids out of date.");
  EXPECT_THAT(GetAppGuid(), StrCaseEq(kAppGuids[std::get<0>(GetParam())]));
#else
  // For brands that do not integrate with Omaha/Google Update, the app guid is
  // an empty string.
  EXPECT_STREQ(L"", GetAppGuid());
#endif
}

TEST_P(InstallStaticUtilTest, GetBaseAppId) {
#if defined(OFFICIAL_BUILD)
  // The base app ids for the brand's install modes; parallel to kInstallModes.
  static constexpr const wchar_t* kBaseAppIds[] = {
      L"Brave", L"BraveBeta", L"BraveDev", L"BraveNightly",
  };
#else
  // The base app ids for the brand's install modes; parallel to kInstallModes.
  static constexpr const wchar_t* kBaseAppIds[] = {
      L"BraveDevelopment",
  };
#endif
  static_assert(base::size(kBaseAppIds) == NUM_INSTALL_MODES,
                "kBaseAppIds out of date.");
  EXPECT_THAT(GetBaseAppId(), StrCaseEq(kBaseAppIds[std::get<0>(GetParam())]));
}

TEST_P(InstallStaticUtilTest, GetToastActivatorClsid) {
#if defined(OFFICIAL_BUILD)
  // The toast activator CLSIDs for the brand's install modes; parallel to
  // kInstallModes.
  static constexpr CLSID kToastActivatorClsids[] = {
      { 0x6c9646d,
        0x2807,
        0x44c0,
        { 0x97, 0xd2, 0x6d, 0xa0, 0xdb, 0x62, 0x3d,
          0xb4 } },  // Brave-Browser.
      { 0x9560028d,
        0xcca,
        0x49f0,
        { 0x8d, 0x47, 0xef, 0x22, 0xbb, 0xc4, 0xb,
          0xa7 } },  // Brave-Browser-Beta.
      { 0x20b22981,
        0xf63a,
        0x47a6,
        { 0xa5, 0x47, 0x69, 0x1c, 0xc9, 0x4c, 0xae,
          0xe0 } },  // Brave-Browser-Dev.
      { 0xf2edbc59,
        0x7217,
        0x4da5,
        { 0xa2, 0x59, 0x3, 0x2, 0xda, 0x6a, 0x0,
          0xe1 } },  // Brave-Browser-Nightly.
  };

  // The string representation of the CLSIDs above.
  static constexpr const wchar_t* kToastActivatorClsidsString[] = {
      L"{06C9646D-2807-44C0-97D2-6DA0DB623DB4}",  // Brave-Browser.
      L"{9560028D-0CCA-49F0-8D47-EF22BBC40BA7}",  // Brave-Browser-Beta.
      L"{20B22981-F63A-47A6-A547-691CC94CAEE0}",  // Brave-Browser-Dev.
      L"{F2EDBC59-7217-4DA5-A259-0302DA6A00E1}",  // Brave-Browser-Nightly.
  };
#else
  // The toast activator CLSIDs for the brand's install modes; parallel to
  // kInstallModes.
  static constexpr CLSID kToastActivatorClsids[] = {
      { 0xeb41c6e8,
        0xba35,
        0x4c06,
        { 0x96, 0xe8, 0x6f, 0x30, 0xf1, 0x8c, 0xa5,
          0x5c } },  // Brave-Browser-Development.
  };

  // The string representation of the CLSIDs above.
  static constexpr const wchar_t* kToastActivatorClsidsString[] = {
      L"{EB41C6E8-BA35-4C06-96E8-6F30F18CA55C}"  // Brave-Browser-Development.
  };
#endif
  static_assert(base::size(kToastActivatorClsids) == NUM_INSTALL_MODES,
                "kToastActivatorClsids out of date.");

  EXPECT_EQ(GetToastActivatorClsid(),
            kToastActivatorClsids[std::get<0>(GetParam())]);

  const int kCLSIDSize = 39;
  wchar_t clsid_str[kCLSIDSize];
  ASSERT_EQ(::StringFromGUID2(GetToastActivatorClsid(), clsid_str, kCLSIDSize),
            kCLSIDSize);
  EXPECT_THAT(clsid_str,
              StrCaseEq(kToastActivatorClsidsString[std::get<0>(GetParam())]));
}

TEST_P(InstallStaticUtilTest, UsageStatsAbsent) {
  EXPECT_FALSE(GetCollectStatsConsent());
}

TEST_P(InstallStaticUtilTest, UsageStatsZero) {
  SetUsageStat(0, false);
  EXPECT_FALSE(GetCollectStatsConsent());
}

TEST_P(InstallStaticUtilTest, UsageStatsZeroMedium) {
  if (!system_level())
    return;
  SetUsageStat(0, true);
  EXPECT_FALSE(GetCollectStatsConsent());
}

TEST_P(InstallStaticUtilTest, UsageStatsOne) {
  SetUsageStat(1, false);
  EXPECT_TRUE(GetCollectStatsConsent());
}

TEST_P(InstallStaticUtilTest, UsageStatsOneMedium) {
  if (!system_level())
    return;
  SetUsageStat(1, true);
  EXPECT_TRUE(GetCollectStatsConsent());
}

TEST_P(InstallStaticUtilTest, ReportingIsEnforcedByPolicy) {
  bool reporting_enabled = false;
  EXPECT_FALSE(ReportingIsEnforcedByPolicy(&reporting_enabled));

  SetMetricsReportingPolicy(0);
  EXPECT_TRUE(ReportingIsEnforcedByPolicy(&reporting_enabled));
  EXPECT_FALSE(reporting_enabled);

  SetMetricsReportingPolicy(1);
  EXPECT_TRUE(ReportingIsEnforcedByPolicy(&reporting_enabled));
  EXPECT_TRUE(reporting_enabled);
}

TEST_P(InstallStaticUtilTest, UsageStatsPolicy) {
  // Policy alone.
  SetMetricsReportingPolicy(0);
  EXPECT_FALSE(GetCollectStatsConsent());

  SetMetricsReportingPolicy(1);
  EXPECT_TRUE(GetCollectStatsConsent());

  // Policy trumps usagestats.
  SetMetricsReportingPolicy(1);
  SetUsageStat(0, false);
  EXPECT_TRUE(GetCollectStatsConsent());

  SetMetricsReportingPolicy(0);
  SetUsageStat(1, false);
  EXPECT_FALSE(GetCollectStatsConsent());
}

TEST_P(InstallStaticUtilTest, GetChromeChannelName) {
  EXPECT_EQ(default_channel(), GetChromeChannelName());
}

TEST_P(InstallStaticUtilTest, GetChromeChannel) {
#if defined(OFFICIAL_BUILD)
  // Parallel to kInstallModes.
  static constexpr version_info::Channel kChannels[] = {
      version_info::Channel::STABLE,  // Brave-Browser.
      version_info::Channel::BETA,    // Brave-Browser-Beta.
      version_info::Channel::DEV,     // Brave-Browser-Dev.
      version_info::Channel::CANARY,  // Brave-Browser-Nightly.
  };
#else
  // Parallel to kInstallModes.
  static constexpr version_info::Channel kChannels[] = {
    version_info::Channel::UNKNOWN,
  };
#endif
  EXPECT_EQ(kChannels[std::get<0>(GetParam())], GetChromeChannel());
}

#if defined(OFFICIAL_BUILD)
// Stable supports user and system levels.
INSTANTIATE_TEST_CASE_P(Stable,
                        InstallStaticUtilTest,
                        testing::Combine(testing::Values(STABLE_INDEX),
                                         testing::Values("user", "system")));
// Beta supports user and system levels.
INSTANTIATE_TEST_CASE_P(Beta,
                        InstallStaticUtilTest,
                        testing::Combine(testing::Values(BETA_INDEX),
                                         testing::Values("user", "system")));
// Dev supports user and system levels.
INSTANTIATE_TEST_CASE_P(Dev,
                        InstallStaticUtilTest,
                        testing::Combine(testing::Values(DEV_INDEX),
                                         testing::Values("user", "system")));
// Canary is only at user level.
INSTANTIATE_TEST_CASE_P(Nightly,
                        InstallStaticUtilTest,
                        testing::Combine(testing::Values(NIGHTLY_INDEX),
                                         testing::Values("user")));
#else   // OFFICIAL_BUILD
// Chromium supports user and system levels.
INSTANTIATE_TEST_CASE_P(Development,
                        InstallStaticUtilTest,
                        testing::Combine(testing::Values(DEVELOPER_INDEX),
                                         testing::Values("user", "system")));
#endif  // !OFFICIAL_BUILD

}  // namespace install_static
