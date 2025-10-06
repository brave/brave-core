/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <vector>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/values_util.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/bind.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "base/test/test_timeouts.h"
#include "base/time/time.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_stats/switches.h"
#include "brave/browser/net/brave_network_audit_test_helper.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/p3a/star_randomness_meta.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/password_manager/profile_password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/test/browser_test.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gmock/include/gmock/gmock.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/components/playlist/common/features.h"
#endif

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
#include "brave/components/web_discovery/common/features.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

namespace brave {
namespace {

// Max amount of time to wait after getting an URL loaded, in milliseconds. Note
// that the value passed to --ui-test-action-timeout in //brave/package.json, as
// part of the 'network-audit' script, must be big enough to accomodate this.
//
// In particular:
//   --ui-test-action-timeout: should be greater than |kMaxTimeoutPerLoadedURL|.
//   --test-launcher-timeout: should be able to fit the total sum of timeouts.
constexpr int kMaxTimeoutPerLoadedURL = 30;

void WaitForTimeout(int timeout) {
  base::test::ScopedRunLoopTimeout file_download_timeout(
      FROM_HERE, base::Seconds(kMaxTimeoutPerLoadedURL + 1));
  base::RunLoop run_loop;
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::Seconds(timeout));
  run_loop.Run();
}

class BraveNetworkAuditTest : public InProcessBrowserTest {
 public:
  BraveNetworkAuditTest() {
    std::vector<base::test::FeatureRef> features;
#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
    features.push_back(playlist::features::kPlaylist);
#endif  // BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
    features.push_back(web_discovery::features::kBraveWebDiscoveryNative);
#endif
    scoped_feature_list_.InitWithFeatures(features, {});
  }

  BraveNetworkAuditTest(const BraveNetworkAuditTest&) = delete;
  BraveNetworkAuditTest& operator=(const BraveNetworkAuditTest&) = delete;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    ASSERT_TRUE(embedded_test_server()->Start());

    auto* local_state = g_browser_process->local_state();
    {
      ScopedDictPrefUpdate update(local_state,
                                  p3a::kTypicalConstellationLogsPrefName);
      base::Value::Dict* epoch_dict = update->EnsureDict("1");  // epoch 1
      epoch_dict->Set("dummy.metric", "dummy_message_content");
    }

    // Store current epoch and next epoch time in randomness meta prefs
    {
      ScopedDictPrefUpdate update(local_state,
                                  p3a::kRandomnessMetaDictPrefName);
      base::Value::Dict* meta_type_dict = update->EnsureDict(
          p3a::MetricLogTypeToString(p3a::MetricLogType::kTypical));
      meta_type_dict->Set(p3a::kCurrentEpochPrefKey, 1);
      meta_type_dict->Set(p3a::kNextEpochTimePrefKey,
                          base::TimeToValue(base::Time::Now() + base::Days(7)));
    }
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    CHECK(temp_dir_.CreateUniqueTempDir());
    base::FilePath source_root_path =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);

    // Full log containing all the network requests.
    net_log_path_ = temp_dir_.GetPath().AppendASCII("network_log.json");

    // Log containing the results of the audit only.
    audit_results_path_ =
        source_root_path.AppendASCII("network_audit_results.json");

    command_line->AppendSwitchPath(network::switches::kLogNetLog,
                                   net_log_path_);
    command_line->AppendSwitchASCII(network::switches::kNetLogCaptureMode,
                                    "Everything");
    command_line->AppendSwitchASCII(
        brave_stats::switches::kBraveStatsStartupDelay, "15");
  }

  void TearDownInProcessBrowserTestFixture() override {
    VerifyNetworkAuditLog(net_log_path_, audit_results_path_, allow_list_level_,
                          std::vector<std::string>());
  }

  Profile* profile() { return browser()->profile(); }

 protected:
  void RunTestTasks() {
    g_brave_browser_process->p3a_service()
        ->remote_config_manager()
        ->SetIsLoadedForTesting(true);
    g_brave_browser_process->p3a_service()->OnRemoteConfigLoaded();
#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE) || BUILDFLAG(ENABLE_EXTENSIONS)
    if (enable_web_discovery_) {
      profile()->GetPrefs()->SetBoolean(kWebDiscoveryEnabled, true);
    }
#endif

    // Load the Welcome page.
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL("brave://welcome")));
    WaitForTimeout(kMaxTimeoutPerLoadedURL);

    // Add a password to the password manager.
    password_manager::PasswordStoreInterface* password_store =
        ProfilePasswordStoreFactory::GetForProfile(
            browser()->profile(), ServiceAccessType::IMPLICIT_ACCESS)
            .get();
    password_manager::PasswordForm signin_form;
    signin_form.signon_realm = "https://www.facebook.com/";
    signin_form.url = GURL("https://www.facebook.com/");
    signin_form.action = GURL("https://www.facebook.com/");
    signin_form.username_value = u"john";
    signin_form.password_value = u"password1";
    password_store->AddLogin(signin_form);

    // Load the NTP to check requests made from the JS widgets.
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL("brave://newtab")));
    WaitForTimeout(kMaxTimeoutPerLoadedURL);

    // Load a simple HTML page from the test server.
    GURL simple_url(embedded_test_server()->GetURL("/simple.html"));
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), simple_url));
    WaitForTimeout(kMaxTimeoutPerLoadedURL);

    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL("brave://wallet")));
    WaitForTimeout(kMaxTimeoutPerLoadedURL);

#if defined(TOOLKIT_VIEWS)
    auto* sidebar_controller = browser()->GetFeatures().sidebar_controller();
    auto* sidebar_model = sidebar_controller->model();
    const auto& all_items = sidebar_model->GetAllSidebarItems();
    const int item_num = all_items.size();
    const int builtin_panel_item_total = 4;
    int builtin_panel_item_count = 0;
    for (int i = 0; i < item_num; ++i) {
      auto item = all_items[i];
      // Load all builtin panel items.
      if (item.is_built_in_type() && item.open_in_panel) {
        builtin_panel_item_count++;
        sidebar_controller->ActivateItemAt(i);
        WaitForTimeout(kMaxTimeoutPerLoadedURL);
      }
    }

    // Currently, we have 4 builtin panel items.
    // If it's increased, --test-launcher-time should be increased also.
    EXPECT_EQ(builtin_panel_item_total, builtin_panel_item_count);
#endif
  }

  bool enable_web_discovery_ = false;
  AllowListLevel allow_list_level_ = AllowListLevel::kBaseAndOther;
  base::FilePath audit_results_path_;

 private:
  base::ScopedTempDir temp_dir_;
  base::FilePath net_log_path_;

  base::test::ScopedFeatureList scoped_feature_list_;
};

// Loads brave://welcome first to simulate a first run and then loads another
// URL, waiting some time after each load to allow gathering network requests.
IN_PROC_BROWSER_TEST_F(BraveNetworkAuditTest, BasicTests) {
  allow_list_level_ = AllowListLevel::kBaseAndOther;
  RunTestTasks();
}

IN_PROC_BROWSER_TEST_F(BraveNetworkAuditTest, BasicTestsWithOptInTelemetry) {
  enable_web_discovery_ = true;
  allow_list_level_ = AllowListLevel::kFull;
  RunTestTasks();
}

// Ensures that network logs are reduced when Brave Origin is enabled.
IN_PROC_BROWSER_TEST_F(BraveNetworkAuditTest,
                       BasicTestsWithAdminPoliciesEnabled) {
  allow_list_level_ = AllowListLevel::kBase;
  audit_results_path_ = audit_results_path_.DirName().AppendASCII(
      "network_audit_origin_results.json");

  profile()->GetPrefs()->SetBoolean(ai_chat::prefs::kEnabledByPolicy, false);
#if BUILDFLAG(ENABLE_TOR)
  g_browser_process->local_state()->SetBoolean(tor::prefs::kTorDisabled, true);
#endif
  g_browser_process->local_state()->SetBoolean(kStatsReportingEnabled, false);
  g_browser_process->local_state()->SetBoolean(p3a::kP3AEnabled, false);
#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE) || BUILDFLAG(ENABLE_EXTENSIONS)
  profile()->GetPrefs()->SetBoolean(kWebDiscoveryEnabled, false);
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  profile()->GetPrefs()->SetBoolean(brave_rewards::prefs::kDisabledByPolicy,
                                    true);
  profile()->GetPrefs()->SetBoolean(brave_wallet::prefs::kDisabledByPolicy,
                                    true);
  profile()->GetPrefs()->SetBoolean(
      brave_news::prefs::kBraveNewsDisabledByPolicy, true);
  profile()->GetPrefs()->SetBoolean(kBraveTalkDisabledByPolicy, true);
#if BUILDFLAG(ENABLE_SPEEDREADER)
  profile()->GetPrefs()->SetBoolean(speedreader::kSpeedreaderPrefFeatureEnabled,
                                    false);
#endif
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  profile()->GetPrefs()->SetBoolean(brave_vpn::prefs::kManagedBraveVPNDisabled,
                                    true);
#endif

  RunTestTasks();
}

}  // namespace
}  // namespace brave
