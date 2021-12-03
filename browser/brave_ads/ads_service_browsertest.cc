/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "bat/ads/pref_names.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_browser_tests --filter=BraveAdsBrowserTest.*

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct BraveAdsUpgradePathParamInfo {
  // |preferences| should be set to the name of the preferences filename located
  // at "src/brave/test/data/rewards-data/migration"
  std::string preferences;

  // |supported_locale| should be set to true if the locale should be set to a
  // supported locale; otherwise, should be set to false
  bool supported_locale;

  // |newly_supported_locale| should be set to true if the locale should be set
  // to a newly supported locale; otherwise, should be set to false
  bool newly_supported_locale;

  // |rewards_enabled| should be set to true if Brave rewards should be enabled
  // after upgrade; otherwise, should be set to false
  bool rewards_enabled;

  // |ads_enabled| should be set to true if Brave ads should be enabled after
  // upgrade; otherwise, should be set to false
  bool ads_enabled;
};

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  http_response->set_content(
      "<html>"
      "  <head></head>"
      "  <body>"
      "    <div>Hello, world!</div>"
      "  </body>"
      "</html>");
  return std::move(http_response);
}

}  // namespace

class TestRewardsServiceObserver
    : public brave_rewards::RewardsServiceObserver {
 public:
  TestRewardsServiceObserver() = default;
  ~TestRewardsServiceObserver() override = default;

  void WaitForRewardsInitialization() {
    if (rewards_initialized_) {
      return;
    }

    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

  // RewardsServiceObserver implementation
  void OnRewardsInitialized(brave_rewards::RewardsService* service) override {
    rewards_initialized_ = true;
    if (run_loop_) {
      run_loop_->Quit();
    }
  }

 private:
  std::unique_ptr<base::RunLoop> run_loop_;
  bool rewards_initialized_ = false;
};

class BraveAdsBrowserTest : public InProcessBrowserTest,
                            public base::SupportsWeakPtr<BraveAdsBrowserTest> {
 public:
  BraveAdsBrowserTest() {
    // You can do set-up work for each test here

    MaybeMockLocaleHelper();
  }

  ~BraveAdsBrowserTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  void SetUpOnMainThread() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    // Setup embedded test server for HTTPS requests
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(base::BindRepeating(&HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    brave::RegisterPathProvider();
    ReadTestData();

    auto* browser_profile = browser()->profile();

    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(browser_profile));
    rewards_service_->AddObserver(&rewards_service_observer_);

    rewards_service_->ForTestingSetTestResponseCallback(base::BindRepeating(
        &BraveAdsBrowserTest::GetTestResponse, base::Unretained(this)));

    rewards_service_->SetLedgerEnvForTesting();

    ads_service_ = static_cast<brave_ads::AdsService*>(
        brave_ads::AdsServiceFactory::GetForProfile(browser_profile));
    ASSERT_NE(nullptr, ads_service_);
  }

  void TearDownOnMainThread() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
    rewards_service_->RemoveObserver(&rewards_service_observer_);

    InProcessBrowserTest::TearDownOnMainThread();
  }

  void GetTestDataDir(base::FilePath* test_data_dir) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir));
    *test_data_dir = test_data_dir->AppendASCII("rewards-data");
    ASSERT_TRUE(base::PathExists(*test_data_dir));
  }

  void ReadTestData() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath path;
    GetTestDataDir(&path);
    ASSERT_TRUE(
        base::ReadFileToString(path.AppendASCII("wallet_resp.json"), &wallet_));
    ASSERT_TRUE(base::ReadFileToString(path.AppendASCII("parameters_resp.json"),
                                       &parameters_));
  }

  void GetTestResponse(const std::string& url,
                       int32_t method,
                       int* response_status_code,
                       std::string* response,
                       base::flat_map<std::string, std::string>* headers) {
    if (url.find("/v3/wallet/brave") != std::string::npos) {
      *response = wallet_;
      *response_status_code = net::HTTP_CREATED;
      return;
    }
  }

  bool SetUpUserDataDirectory() override {
    MaybeMockUserProfilePreferencesForBraveAdsUpgradePath();

    return true;
  }

  void RunUntilIdle() {
    base::RunLoop loop;
    loop.RunUntilIdle();
  }

  PrefService* GetPrefs() const { return browser()->profile()->GetPrefs(); }

  bool IsAdsEnabled() { return ads_service_->IsEnabled(); }

  void MaybeMockLocaleHelper() {
    const std::map<std::string, std::string> locale_for_tests = {
        {"BraveAdsLocaleIsSupported", "en_US"},
        {"BraveAdsLocaleIsNotSupported", "en_XX"},
        {"BraveAdsLocaleIsNewlySupported", "ja_JP"},
        {"BraveAdsLocaleIsNewlySupportedForLatestSchemaVersion",
         newly_supported_locale_},
        {"BraveAdsLocaleIsNotNewlySupported", "en_XX"},
        {"PRE_AutoEnableAdsForSupportedLocales", "en_US"},
        {"AutoEnableAdsForSupportedLocales", "en_US"},
        {"PRE_DoNotAutoEnableAdsForUnsupportedLocales", "en_XX"},
        {"DoNotAutoEnableAdsForUnsupportedLocales", "en_XX"}};

    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    ASSERT_NE(nullptr, test_info);

    const auto it = locale_for_tests.find(test_info->name());
    if (it == locale_for_tests.end()) {
      MaybeMockLocaleHelperForBraveAdsUpgradePath();
      return;
    }

    MockLocaleHelper(it->second);
  }

  void MaybeMockLocaleHelperForBraveAdsUpgradePath() {
    std::vector<std::string> parameters;
    if (!GetUpgradePathParams(&parameters)) {
      return;
    }

    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    ASSERT_NE(nullptr, test_info);
    const std::string test_name = test_info->name();

    const std::string newly_supported_locale_parameter = parameters.at(2);
    ASSERT_TRUE(!newly_supported_locale_parameter.empty());

    std::string locale;
    if (test_name.find("PRE_UpgradePath") == 0) {
      if (newly_supported_locale_parameter == "ForNewlySupportedLocale") {
        locale = newly_supported_locale_;
      } else {
        locale = "en_US";
      }
    } else {
      const std::string supported_locale_parameter = parameters.at(1);
      ASSERT_TRUE(!supported_locale_parameter.empty());

      if (newly_supported_locale_parameter == "ForNewlySupportedLocale") {
        locale = newly_supported_locale_;
      } else if (supported_locale_parameter == "ForSupportedLocale") {
        locale = "en_US";
      } else {
        locale = "en_XX";
      }
    }

    MockLocaleHelper(locale);
  }

  void MockLocaleHelper(const std::string& locale) {
    locale_helper_mock_ =
        std::make_unique<NiceMock<brave_l10n::LocaleHelperMock>>();

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());

    ON_CALL(*locale_helper_mock_, GetLocale()).WillByDefault(Return(locale));
  }

  void MaybeMockUserProfilePreferencesForBraveAdsUpgradePath() {
    std::vector<std::string> parameters;
    if (!GetUpgradePathParams(&parameters)) {
      return;
    }

    const std::string preferences_parameter = parameters.at(0);
    ASSERT_TRUE(!preferences_parameter.empty());

    MockUserProfilePreferences(preferences_parameter);
  }

  bool GetUpgradePathParams(std::vector<std::string>* parameters) {
    EXPECT_NE(nullptr, parameters);

    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    EXPECT_NE(nullptr, test_info);

    const std::string test_suite_name = test_info->test_suite_name();
    if (test_suite_name != "BraveAdsBrowserTest/BraveAdsUpgradeBrowserTest") {
      return false;
    }

    const std::string test_name = test_info->name();
    const auto test_name_components = base::SplitString(
        test_name, "/", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    EXPECT_EQ(2UL, test_name_components.size());

    // test_name_components:
    // 0 = Name
    // 1 = Parameters

    const std::string name = test_name_components.at(0);
    if (name != "UpgradePath" && name != "PRE_UpgradePath") {
      return false;
    }

    // parameters:
    // 0 = Preferences
    // 1 = Supported locale
    //   2 = Newly supported locale
    //   3 = Rewards enabled
    //   4 = Ads enabled

    *parameters =
        base::SplitString(test_name_components.at(1), "_",
                          base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    EXPECT_EQ(5UL, parameters->size());

    return true;
  }

  base::FilePath GetUserDataPath() const {
    base::FilePath path;
    base::PathService::Get(chrome::DIR_USER_DATA, &path);
    path = path.AppendASCII(TestingProfile::kTestUserProfileDir);
    return path;
  }

  base::FilePath GetTestDataPath() const {
    // TODO(tmancey): We should be able to use |GetTestDataDir| however the path
    // was invalid during setup, therefore investigate further
    base::FilePath path;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.Append(FILE_PATH_LITERAL("brave"));
    path = path.Append(FILE_PATH_LITERAL("test"));
    path = path.Append(FILE_PATH_LITERAL("data"));
    return path;
  }

  void MockUserProfilePreferences(const std::string& preference) const {
    auto user_data_path = GetUserDataPath();
    ASSERT_TRUE(base::CreateDirectory(user_data_path));

    const auto preferences_path =
        user_data_path.Append(chrome::kPreferencesFilename);

    // TODO(tmancey): We should be able to use |GetTestDataDir| however the path
    // was invalid during setup, therefore investigate further
    auto test_data_path = GetTestDataPath();
    test_data_path = test_data_path.AppendASCII("rewards-data");
    test_data_path = test_data_path.AppendASCII("migration");
    test_data_path = test_data_path.AppendASCII(preference);
    ASSERT_TRUE(base::PathExists(test_data_path));

    ASSERT_TRUE(base::CopyFile(test_data_path, preferences_path));
  }

  MOCK_METHOD1(OnGetEnvironment, void(ledger::type::Environment));
  MOCK_METHOD1(OnGetDebug, void(bool));
  MOCK_METHOD1(OnGetReconcileTime, void(int32_t));
  MOCK_METHOD1(OnGetRetryInterval, void(int32_t));

  std::unique_ptr<net::EmbeddedTestServer> https_server_;

  raw_ptr<brave_rewards::RewardsServiceImpl> rewards_service_ = nullptr;

  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;

  TestRewardsServiceObserver rewards_service_observer_;

  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  const std::string newly_supported_locale_ = "en_830";

  std::string wallet_;
  std::string parameters_;
};

IN_PROC_BROWSER_TEST_F(BraveAdsBrowserTest, BraveAdsLocaleIsSupported) {
  EXPECT_TRUE(ads_service_->IsSupportedLocale());
}

IN_PROC_BROWSER_TEST_F(BraveAdsBrowserTest, BraveAdsLocaleIsNotSupported) {
  EXPECT_FALSE(ads_service_->IsSupportedLocale());
}

IN_PROC_BROWSER_TEST_F(BraveAdsBrowserTest,
                       PRE_BraveAdsMigrateDefaultAdsPerHourFromVersion9) {
  GetPrefs()->SetInteger(brave_ads::prefs::kVersion, 9);

  GetPrefs()->SetInt64(ads::prefs::kAdsPerHour, -1);
  ASSERT_TRUE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
}

IN_PROC_BROWSER_TEST_F(BraveAdsBrowserTest,
                       BraveAdsMigrateDefaultAdsPerHourFromVersion9) {
  EXPECT_FALSE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
  EXPECT_EQ(-1, GetPrefs()->GetInt64(ads::prefs::kAdsPerHour));
}

IN_PROC_BROWSER_TEST_F(BraveAdsBrowserTest,
                       PRE_BraveAdsMigrateLegacyDefaultPerHourFromVersion9) {
  GetPrefs()->SetInteger(brave_ads::prefs::kVersion, 9);

  GetPrefs()->SetInt64(ads::prefs::kAdsPerHour, 2);
  ASSERT_TRUE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
}

IN_PROC_BROWSER_TEST_F(BraveAdsBrowserTest,
                       BraveAdsMigrateLegacyDefaultPerHourFromVersion9) {
  EXPECT_FALSE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
  EXPECT_EQ(-1, GetPrefs()->GetInt64(ads::prefs::kAdsPerHour));
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsBrowserTest,
    PRE_BraveAdsMigrateAdsPerHourForFreshInstallFromVersion9) {
  GetPrefs()->SetInteger(brave_ads::prefs::kVersion, 9);

  ASSERT_FALSE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
}

IN_PROC_BROWSER_TEST_F(BraveAdsBrowserTest,
                       BraveAdsMigrateAdsPerHourForFreshInstallFromVersion9) {
  EXPECT_FALSE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
  EXPECT_EQ(-1, GetPrefs()->GetInt64(ads::prefs::kAdsPerHour));
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsBrowserTest,
    PRE_BraveAdsMigrateAdsPerHourForIssue17155FromVersion10) {
  GetPrefs()->SetInteger(brave_ads::prefs::kVersion, 10);

  GetPrefs()->SetInt64(ads::prefs::kAdsPerHour, 0);
  ASSERT_TRUE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
}

IN_PROC_BROWSER_TEST_F(BraveAdsBrowserTest,
                       BraveAdsMigrateAdsPerHourForIssue17155FromVersion10) {
  EXPECT_FALSE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
  EXPECT_EQ(-1, GetPrefs()->GetInt64(ads::prefs::kAdsPerHour));
}

IN_PROC_BROWSER_TEST_F(BraveAdsBrowserTest,
                       PRE_BraveAdsMigrateDefaultAdsPerHourFromVersion10) {
  GetPrefs()->SetInteger(brave_ads::prefs::kVersion, 10);

  GetPrefs()->SetInt64(ads::prefs::kAdsPerHour, -1);
  ASSERT_TRUE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
}

IN_PROC_BROWSER_TEST_F(BraveAdsBrowserTest,
                       BraveAdsMigrateDefaultAdsPerHourFromVersion10) {
  EXPECT_FALSE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
  EXPECT_EQ(-1, GetPrefs()->GetInt64(ads::prefs::kAdsPerHour));
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsBrowserTest,
    PRE_BraveAdsMigrateAdsPerHourForFreshInstallFromVersion10) {
  GetPrefs()->SetInteger(brave_ads::prefs::kVersion, 10);

  ASSERT_FALSE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
}

IN_PROC_BROWSER_TEST_F(BraveAdsBrowserTest,
                       BraveAdsMigrateAdsPerHourForFreshInstallFromVersion10) {
  EXPECT_FALSE(GetPrefs()->HasPrefPath(ads::prefs::kAdsPerHour));
  EXPECT_EQ(-1, GetPrefs()->GetInt64(ads::prefs::kAdsPerHour));
}

class BraveAdsUpgradeBrowserTest
    : public BraveAdsBrowserTest,
      public ::testing::WithParamInterface<BraveAdsUpgradePathParamInfo> {};

const BraveAdsUpgradePathParamInfo kTests[] = {
    // Test Suite with expected outcomes for upgrade paths instantiated using
    // Value-Parameterized Tests

    // Upgrade from 0.62 to current version
    {
        "PreferencesForVersion062WithRewardsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion062WithRewardsEnabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion062WithRewardsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion062WithRewardsEnabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion062WithRewardsDisabled",
        true,  /* supported_locale */
        true,  /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion062WithRewardsEnabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },
    //
    // Upgrade from 0.63 to current version (Initial release of Brave ads)
    {
        "PreferencesForVersion063WithRewardsAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion063WithRewardsEnabledAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion063WithRewardsAndAdsEnabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion063WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion063WithRewardsEnabledAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    // TODO(tmancey): The following test failed due to the ads_enabled flag
    // being
    // incorrectly set to false
    // {
    //   "PreferencesForVersion063WithRewardsAndAdsEnabled",
    //   true,  /* supported_locale */
    //   false, /* newly_supported_locale */
    //   true,  /* rewards_enabled */
    //   true  /* ads_enabled */
    // },
    {
        "PreferencesForVersion063WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        true,  /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion063WithRewardsEnabledAndAdsDisabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },
    {
        "PreferencesForVersion063WithRewardsAndAdsEnabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },

    // Upgrade from 0.67 to current version
    {
        "PreferencesForVersion067WithRewardsAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion067WithRewardsEnabledAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion067WithRewardsAndAdsEnabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion067WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion067WithRewardsEnabledAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion067WithRewardsAndAdsEnabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        true   /* ads_enabled */
    },
    {
        "PreferencesForVersion067WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        true,  /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion067WithRewardsEnabledAndAdsDisabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },
    {
        "PreferencesForVersion067WithRewardsAndAdsEnabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },

    // Upgrade from 0.68 to current version
    {
        "PreferencesForVersion068WithRewardsAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion068WithRewardsEnabledAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion068WithRewardsAndAdsEnabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion068WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion068WithRewardsEnabledAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion068WithRewardsAndAdsEnabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        true   /* ads_enabled */
    },
    {
        "PreferencesForVersion068WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        true,  /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion068WithRewardsEnabledAndAdsDisabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },
    {
        "PreferencesForVersion068WithRewardsAndAdsEnabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },

    // Upgrade from 0.69 to current version
    {
        "PreferencesForVersion069WithRewardsAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion069WithRewardsEnabledAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion069WithRewardsAndAdsEnabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion069WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion069WithRewardsEnabledAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion069WithRewardsAndAdsEnabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        true   /* ads_enabled */
    },
    {
        "PreferencesForVersion069WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        true,  /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion069WithRewardsEnabledAndAdsDisabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },
    {
        "PreferencesForVersion069WithRewardsAndAdsEnabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },

    // Upgrade from 0.70 to current version
    {
        "PreferencesForVersion070WithRewardsAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion070WithRewardsEnabledAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion070WithRewardsAndAdsEnabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion070WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion070WithRewardsEnabledAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion070WithRewardsAndAdsEnabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        true   /* ads_enabled */
    },
    {
        "PreferencesForVersion070WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        true,  /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion070WithRewardsEnabledAndAdsDisabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },
    {
        "PreferencesForVersion070WithRewardsAndAdsEnabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },

    // Upgrade from 0.71 to current version
    {
        "PreferencesForVersion071WithRewardsAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion071WithRewardsEnabledAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion071WithRewardsAndAdsEnabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion071WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion071WithRewardsEnabledAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion071WithRewardsAndAdsEnabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        true   /* ads_enabled */
    },
    {
        "PreferencesForVersion071WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        true,  /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion071WithRewardsEnabledAndAdsDisabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },
    {
        "PreferencesForVersion071WithRewardsAndAdsEnabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },

    // Upgrade from 0.72 to current version
    {
        "PreferencesForVersion072WithRewardsAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion072WithRewardsEnabledAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion072WithRewardsAndAdsEnabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion072WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion072WithRewardsEnabledAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion072WithRewardsAndAdsEnabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        true   /* ads_enabled */
    },
    {
        "PreferencesForVersion072WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        true,  /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion072WithRewardsEnabledAndAdsDisabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },
    {
        "PreferencesForVersion072WithRewardsAndAdsEnabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },

    // Upgrade from 1.2 to current version
    {
        "PreferencesForVersion12WithRewardsAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion12WithRewardsEnabledAndAdsDisabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion12WithRewardsAndAdsEnabled",
        false, /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion12WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion12WithRewardsEnabledAndAdsDisabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion12WithRewardsAndAdsEnabled",
        true,  /* supported_locale */
        false, /* newly_supported_locale */
        true,  /* rewards_enabled */
        true   /* ads_enabled */
    },
    {
        "PreferencesForVersion12WithRewardsAndAdsDisabled",
        true,  /* supported_locale */
        true,  /* newly_supported_locale */
        false, /* rewards_enabled */
        false  /* ads_enabled */
    },
    {
        "PreferencesForVersion12WithRewardsEnabledAndAdsDisabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    },
    {
        "PreferencesForVersion12WithRewardsAndAdsEnabled",
        true, /* supported_locale */
        true, /* newly_supported_locale */
        true, /* rewards_enabled */
        false /* ads_enabled */
    }};

IN_PROC_BROWSER_TEST_P(BraveAdsUpgradeBrowserTest, PRE_UpgradePath) {
  // Handled in |MaybeMockLocaleHelperForBraveAdsUpgradePath|

  const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  ASSERT_NE(nullptr, test_info);
  const std::string test_name = test_info->name();

  // Wait for Brave Rewards to be initialized before proceeding with
  // tests that rely on Rewards
  if ((test_name.find("WithRewardsEnabled_") != std::string::npos ||
       test_name.find("WithRewardsAndAdsEnabled_") != std::string::npos) &&
      !rewards_service_->IsInitialized()) {
    rewards_service_observer_.WaitForRewardsInitialization();
  }
}

IN_PROC_BROWSER_TEST_P(BraveAdsUpgradeBrowserTest, UpgradePath) {
  BraveAdsUpgradePathParamInfo param(GetParam());

  EXPECT_EQ(IsAdsEnabled(), param.ads_enabled);
}

// Generate the test case name from the metadata included in
// |BraveAdsUpgradePathParamInfo|
static std::string GetTestCaseName(
    ::testing::TestParamInfo<BraveAdsUpgradePathParamInfo> param_info) {
  const char* preferences = param_info.param.preferences.c_str();

  const char* supported_locale = param_info.param.supported_locale
                                     ? "ForSupportedLocale"
                                     : "ForUnsupportedLocale";

  const char* newly_supported_locale = param_info.param.newly_supported_locale
                                           ? "ForNewlySupportedLocale"
                                           : "ForUnsupportedLocale";

  const char* rewards_enabled = param_info.param.rewards_enabled
                                    ? "RewardsShouldBeEnabled"
                                    : "RewardsShouldBeDisabled";

  const char* ads_enabled = param_info.param.ads_enabled
                                ? "AdsShouldBeEnabled"
                                : "AdsShouldBeDisabled";

  // NOTE: You should not remove, change the format or reorder the following
  // parameters as they are parsed in |GetUpgradePathParams|
  return base::StringPrintf("%s_%s_%s_%s_%s", preferences, supported_locale,
                            newly_supported_locale, rewards_enabled,
                            ads_enabled);
}

INSTANTIATE_TEST_SUITE_P(BraveAdsBrowserTest,
                         BraveAdsUpgradeBrowserTest,
                         ::testing::ValuesIn(kTests),
                         GetTestCaseName);
