/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/process/process.h"
#include "base/process/process_iterator.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/bind.h"
#include "base/test/thread_test_helper.h"
#include "base/time/time.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/tor/brave_tor_client_updater.h"
#include "brave/components/tor/brave_tor_pluggable_transport_updater.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "brave/components/tor/tor_launcher_observer.h"
#include "brave/components/tor/tor_profile_service.h"
#include "brave/components/tor/tor_utils.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/password_manager/chrome_password_manager_client.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/core/browser/browser_autofill_manager.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/ssl_host_state_delegate.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "gmock/gmock.h"
#include "net/base/features.h"
#include "url/gurl.h"

namespace {

void TestAutofillInWindow(content::WebContents* active_contents,
                          const GURL& fake_url,
                          bool enabled) {
  // Logins.
  autofill::ChromeAutofillClient* autofill_client =
      autofill::ChromeAutofillClient::FromWebContentsForTesting(
          active_contents);
  EXPECT_EQ(autofill_client->IsAutocompleteEnabled(), enabled);
  // Passwords.
  ChromePasswordManagerClient* client =
      ChromePasswordManagerClient::FromWebContents(active_contents);
  EXPECT_EQ(client->IsFillingEnabled(fake_url), enabled);
  // Other info.
  autofill::ContentAutofillDriver* cross_driver =
      autofill::ContentAutofillDriver::GetForRenderFrameHost(
          active_contents->GetPrimaryMainFrame());
  ASSERT_TRUE(cross_driver);
  EXPECT_EQ(cross_driver->GetAutofillClient().IsAutofillEnabled(), enabled);
}

struct MockTorLauncherObserver : public TorLauncherObserver {
 public:
  MOCK_METHOD(void, OnTorLauncherCrashed, (), (override));
  MOCK_METHOD(void, OnTorCrashed, (int64_t pid), (override));
  MOCK_METHOD(void, OnTorLaunched, (bool result, int64_t pid), (override));
  MOCK_METHOD(void, OnTorControlReady, (), (override));
  MOCK_METHOD(void, OnTorNewProxyURI, (const std::string& uri), (override));
  MOCK_METHOD(void, OnTorCircuitEstablished, (bool result), (override));
  MOCK_METHOD(void,
              OnTorInitializing,
              (const std::string& percentage, const std::string& message),
              (override));
  MOCK_METHOD(void, OnTorControlEvent, (const std::string& event), (override));
  MOCK_METHOD(void, OnTorLogUpdated, (), (override));
};

void DownloadTorComponent(const std::string& component_id) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  base::FilePath test_data_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

#if BUILDFLAG(IS_WIN)
  const char platform[] = "win";
#elif BUILDFLAG(IS_MAC)
  const char platform[] = "mac";
#elif BUILDFLAG(IS_LINUX)
  const char platform[] = "linux";
#endif

  const auto component_dir = test_data_dir.AppendASCII("tor")
                                 .AppendASCII("components")
                                 .AppendASCII(platform)
                                 .AppendASCII(component_id);
  ASSERT_TRUE(base::PathExists(component_dir)) << component_dir;

  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);

  ASSERT_TRUE(base::CopyDirectory(
      component_dir, user_data_dir.AppendASCII(component_id), true));
}

bool CheckComponentExists(const std::string& component_id) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  return base::PathExists(user_data_dir.AppendASCII(component_id));
}

void NonBlockingDelay(base::TimeDelta delay) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
  run_loop.Run();
}

}  // namespace

class BraveTorBrowserTest : public InProcessBrowserTest {
 public:
  struct TorInfo {
    raw_ptr<Profile, DanglingUntriaged> tor_profile = nullptr;
    int tor_pid = 0;
  };

  BraveTorBrowserTest() {
    BraveSettingsUI::ShouldExposeElementsForTesting() = true;
  }

  ~BraveTorBrowserTest() override {
    BraveSettingsUI::ShouldExposeElementsForTesting() = false;
  }

  void DownloadTorClient() const {
    DownloadTorComponent(tor::kTorClientComponentId);
  }

  void DownloadTorPluggableTransports() const {
    DownloadTorComponent(tor::kTorPluggableTransportComponentId);
  }

  Profile* OpenTorWindow() {
    Browser* tor_browser =
        TorProfileManager::SwitchToTorProfile(browser()->profile());
    return tor_browser ? tor_browser->profile() : nullptr;
  }

  TorInfo WaitForTorLaunched() {
    MockTorLauncherObserver observer;
    TorLauncherFactory::GetInstance()->AddObserver(&observer);

    base::RunLoop loop;
    EXPECT_CALL(observer, OnTorLaunched(testing::_, testing::_))
        .WillOnce(testing::Invoke(&loop, &base::RunLoop::Quit));

    Profile* tor_profile = OpenTorWindow();

    loop.Run();

    int tor_pid = TorLauncherFactory::GetInstance()->GetTorPid();

    TorLauncherFactory::GetInstance()->RemoveObserver(&observer);

    return {tor_profile, tor_pid};
  }

  void CloseTorWindow(Profile* tor_profile) {
    TorProfileManager::CloseTorProfileWindows(tor_profile);
  }

  void WaitForUpdaterThread(brave_component_updater::BraveComponent* updater) {
    scoped_refptr<base::ThreadTestHelper> io_helper(
        new base::ThreadTestHelper(updater->GetTaskRunner()));
    ASSERT_TRUE(io_helper->Run());
  }

  void WaitForProcess(const std::string& name) {
    while (!base::GetProcessCount(base::FilePath::FromASCII(name).value(),
                                  nullptr)) {
      NonBlockingDelay(base::Milliseconds(25));
    }
  }

  void WaitProcessExit(int pid) {
    base::Process p = base::Process::Open(pid);
    if (p.IsValid()) {
      p.WaitForExit(nullptr);
    }
  }

  void WaitProcessExit(const std::string& name) {
    while (base::GetProcessCount(base::FilePath::FromASCII(name).value(),
                                 nullptr)) {
      NonBlockingDelay(base::Milliseconds(25));
    }
  }
};

IN_PROC_BROWSER_TEST_F(BraveTorBrowserTest, OpenCloseDisableTorWindow) {
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
  DownloadTorClient();

  // Open Tor window, wait for the Tor process to start.
  auto tor = WaitForTorLaunched();
  EXPECT_TRUE(base::Process::Open(tor.tor_pid).IsValid());
  ASSERT_TRUE(tor.tor_profile);

  // Close Tor window, expect the Tor process to die.
  {
    Browser* tor_browser = chrome::FindBrowserWithProfile(tor.tor_profile);
    CloseTorWindow(tor.tor_profile);
    ui_test_utils::BrowserChangeObserver(
        tor_browser, ui_test_utils::BrowserChangeObserver::ChangeType::kRemoved)
        .Wait();

    WaitProcessExit(tor.tor_pid);
  }

  // Disable tor, expect executables are removed.
  {
    TorProfileServiceFactory::SetTorDisabled(true);
    EXPECT_TRUE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));

    WaitForUpdaterThread(g_brave_browser_process->tor_client_updater());
    content::RunAllTasksUntilIdle();

    EXPECT_FALSE(CheckComponentExists(tor::kTorClientComponentId));
  }
}

class BraveTorWithCustomProfileBrowserTest : public BraveTorBrowserTest {
 private:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    if (GetTestPreCount() > 0) {
      base::ScopedAllowBlockingForTesting allow_blocking;

      base::ScopedTempDir user_data_dir;
      ASSERT_TRUE(user_data_dir.CreateUniqueTempDir());

      // Used Take because InProcessBrowserTest will remove it.
      const auto profile_path = user_data_dir.Take().AppendASCII("white space");
      ASSERT_TRUE(base::CreateDirectory(profile_path));

      command_line->AppendSwitchPath(switches::kUserDataDir, profile_path);
    }
  }
};

IN_PROC_BROWSER_TEST_F(BraveTorWithCustomProfileBrowserTest, PRE_SetupBridges) {
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
  DownloadTorClient();

  // No bridges by default.
  auto bridges_config = TorProfileServiceFactory::GetTorBridgesConfig();
  EXPECT_EQ(tor::BridgesConfig::Usage::kNotUsed, bridges_config.use_bridges);
  EXPECT_TRUE(bridges_config.provided_bridges.empty());
  EXPECT_TRUE(bridges_config.requested_bridges.empty());

  // Open Tor window, wait for the Tor process to start.
  auto tor = WaitForTorLaunched();
  EXPECT_TRUE(tor.tor_profile);

  // Pluggable transport component isn't installed.
  EXPECT_FALSE(CheckComponentExists(tor::kTorPluggableTransportComponentId));
  EXPECT_EQ(
      0, base::GetProcessCount(
             base::FilePath::FromASCII(tor::kSnowflakeExecutableName).value(),
             nullptr));
  EXPECT_EQ(0, base::GetProcessCount(
                   base::FilePath::FromASCII(tor::kObfs4ExecutableName).value(),
                   nullptr));

  // Enable bridges
  DownloadTorPluggableTransports();

  bridges_config.use_bridges = tor::BridgesConfig::Usage::kProvide;
  bridges_config.provided_bridges.push_back(
      "snowflake 192.0.2.3:1 2B280B23E1107BB62ABFC40DDCC8824814F80A72");
  TorProfileServiceFactory::SetTorBridgesConfig(bridges_config);

  // Wait Snowflake executable is launched.
  EXPECT_TRUE(CheckComponentExists(tor::kTorPluggableTransportComponentId));
  WaitForProcess(tor::kSnowflakeExecutableName);

  EXPECT_TRUE(
      g_brave_browser_process->tor_pluggable_transport_updater()->IsReady());

  // Add obfs config.
  bridges_config.provided_bridges.push_back(
      "obfs4 144.217.20.138:80 FB70B257C162BF1038CA669D568D76F5B7F0BABB "
      "cert=vYIV5MgrghGQvZPIi1tJwnzorMgqgmlKaB77Y3Z9Q/"
      "v94wZBOAXkW+fdx4aSxLVnKO+xNw iat-mode=0");
  TorProfileServiceFactory::SetTorBridgesConfig(bridges_config);

  // Obfs4 executable is launched.
  WaitForProcess(tor::kSnowflakeExecutableName);
  WaitForProcess(tor::kObfs4ExecutableName);

  // Disable tor.
  TorProfileServiceFactory::SetTorDisabled(true);
  EXPECT_TRUE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
  WaitForUpdaterThread(g_brave_browser_process->tor_client_updater());
  WaitForUpdaterThread(
      g_brave_browser_process->tor_pluggable_transport_updater());
}

IN_PROC_BROWSER_TEST_F(BraveTorWithCustomProfileBrowserTest, SetupBridges) {
  // Tor is disabled in PRE, check pluggable transports are removed.
  EXPECT_FALSE(CheckComponentExists(tor::kTorPluggableTransportComponentId));

  // Pluggable transport processes was terminated at exit.
  EXPECT_EQ(
      0, base::GetProcessCount(
             base::FilePath::FromASCII(tor::kSnowflakeExecutableName).value(),
             nullptr));
  EXPECT_EQ(0, base::GetProcessCount(
                   base::FilePath::FromASCII(tor::kObfs4ExecutableName).value(),
                   nullptr));
}

IN_PROC_BROWSER_TEST_F(BraveTorWithCustomProfileBrowserTest, Incognito) {
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
  EXPECT_FALSE(TorProfileServiceFactory::IsTorManaged(browser()->profile()));

  content::WebContents* web_contents = nullptr;

  const auto is_element_enabled = [&](const char* id) {
    return EvalJs(web_contents,
                  base::StrCat({"!window.testing.torSubpage.getElementById('",
                                id, "').disabled"}))
        .value.GetBool();
  };

  // Disable incognito mode for this profile.
  IncognitoModePrefs::SetAvailability(
      browser()->profile()->GetPrefs(),
      policy::IncognitoModeAvailability::kDisabled);

  EXPECT_TRUE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
  EXPECT_TRUE(TorProfileServiceFactory::IsTorManaged(browser()->profile()));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                           GURL("brave://settings/privacy")));
  web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_FALSE(is_element_enabled("torEnabled"));
  EXPECT_FALSE(is_element_enabled("useBridges"));
  EXPECT_TRUE(is_element_enabled("onionOnlyInTorWindows"));
  EXPECT_TRUE(is_element_enabled("torSnowflake"));

  auto* tor_profile = OpenTorWindow();
  EXPECT_EQ(nullptr, tor_profile);

  // Force incognito mode.
  IncognitoModePrefs::SetAvailability(
      browser()->profile()->GetPrefs(),
      policy::IncognitoModeAvailability::kForced);
  EXPECT_TRUE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
  EXPECT_TRUE(TorProfileServiceFactory::IsTorManaged(browser()->profile()));

  tor_profile = OpenTorWindow();
  EXPECT_EQ(nullptr, tor_profile);

  // Allow incognito.
  IncognitoModePrefs::SetAvailability(
      browser()->profile()->GetPrefs(),
      policy::IncognitoModeAvailability::kEnabled);
  tor_profile = OpenTorWindow();
  EXPECT_NE(nullptr, tor_profile);
  EXPECT_TRUE(tor_profile->IsTor());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                           GURL("brave://settings/privacy")));
  web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(is_element_enabled("torEnabled"));
  EXPECT_TRUE(is_element_enabled("useBridges"));
  EXPECT_TRUE(is_element_enabled("onionOnlyInTorWindows"));
  EXPECT_TRUE(is_element_enabled("torSnowflake"));
}

IN_PROC_BROWSER_TEST_F(BraveTorWithCustomProfileBrowserTest, Autofill) {
  GURL fake_url("http://brave.com/");
  // Disable autofill in private windows.
  browser()->profile()->GetPrefs()->SetBoolean(kBraveAutofillPrivateWindows,
                                               false);
  auto* tor_profile = OpenTorWindow();
  EXPECT_NE(nullptr, tor_profile);
  EXPECT_TRUE(tor_profile->IsTor());
  Browser* tor_browser = chrome::FindBrowserWithProfile(tor_profile);
  content::WebContents* web_contents =
      tor_browser->tab_strip_model()->GetActiveWebContents();
  TestAutofillInWindow(web_contents, fake_url, false);

  // Enable autofill in private windows.
  browser()->profile()->GetPrefs()->SetBoolean(kBraveAutofillPrivateWindows,
                                               true);
  web_contents->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));
  TestAutofillInWindow(web_contents, fake_url, true);
}

IN_PROC_BROWSER_TEST_F(BraveTorBrowserTest, PRE_ResetBridges) {
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
  DownloadTorClient();
  DownloadTorPluggableTransports();

  auto bridges_config = TorProfileServiceFactory::GetTorBridgesConfig();
  bridges_config.use_bridges = tor::BridgesConfig::Usage::kProvide;
  bridges_config.provided_bridges.push_back(
      "snowflake 192.0.2.3:1 2B280B23E1107BB62ABFC40DDCC8824814F80A72");
  TorProfileServiceFactory::SetTorBridgesConfig(bridges_config);

  // Open Tor window, wait for the Tor process to start.
  auto tor = WaitForTorLaunched();
  EXPECT_TRUE(tor.tor_profile);

  // Wait Snowflake executable is launched.
  EXPECT_TRUE(CheckComponentExists(tor::kTorPluggableTransportComponentId));
  WaitForProcess(tor::kSnowflakeExecutableName);

  // Reset bridges
  bridges_config.use_bridges = tor::BridgesConfig::Usage::kNotUsed;
  TorProfileServiceFactory::SetTorBridgesConfig(bridges_config);
  WaitProcessExit(tor::kSnowflakeExecutableName);
}

IN_PROC_BROWSER_TEST_F(BraveTorBrowserTest, ResetBridges) {
  // Tor is enabled and bridges are disabled check pluggable transports are
  // removed.
  EXPECT_TRUE(CheckComponentExists(tor::kTorClientComponentId));
  EXPECT_FALSE(CheckComponentExists(tor::kTorPluggableTransportComponentId));
}

IN_PROC_BROWSER_TEST_F(BraveTorBrowserTest, HttpAllowlistIsolation) {
  // Normal window
  Profile* main_profile = browser()->profile();
  auto* main_storage_partition = main_profile->GetDefaultStoragePartition();
  content::SSLHostStateDelegate* main_state =
      main_profile->GetSSLHostStateDelegate();

  // Incognito window
  Browser* incognito_browser = CreateIncognitoBrowser(nullptr);
  Profile* incognito_profile = incognito_browser->profile();
  auto* incognito_storage_partition =
      incognito_profile->GetDefaultStoragePartition();
  content::SSLHostStateDelegate* incognito_state =
      incognito_profile->GetSSLHostStateDelegate();

  // Tor window
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
  DownloadTorClient();
  auto tor = WaitForTorLaunched();
  Profile* tor_profile = tor.tor_profile;
  auto* tor_storage_partition = tor_profile->GetDefaultStoragePartition();
  content::SSLHostStateDelegate* tor_state =
      tor_profile->GetSSLHostStateDelegate();

  // Confirm that main, incognito, and tor profiles are all different.
  EXPECT_NE(main_profile, incognito_profile);
  EXPECT_NE(main_profile, tor_profile);
  EXPECT_NE(incognito_profile, tor_profile);

  // Test domains, one to "allow http" for each profile.
  std::string host1("example1.test");
  std::string host2("example2.test");
  std::string host3("example3.test");
  main_state->AllowHttpForHost(host1, main_storage_partition);
  incognito_state->AllowHttpForHost(host2, incognito_storage_partition);
  tor_state->AllowHttpForHost(host3, tor_storage_partition);

  // Check that each domain was added to the correct allowlist and
  // there is no leaking between the three profiles.
  EXPECT_TRUE(main_state->IsHttpAllowedForHost(host1, main_storage_partition));
  EXPECT_FALSE(incognito_state->IsHttpAllowedForHost(
      host1, incognito_storage_partition));
  EXPECT_FALSE(tor_state->IsHttpAllowedForHost(host1, tor_storage_partition));
  EXPECT_FALSE(main_state->IsHttpAllowedForHost(host2, main_storage_partition));
  EXPECT_TRUE(incognito_state->IsHttpAllowedForHost(
      host2, incognito_storage_partition));
  EXPECT_FALSE(tor_state->IsHttpAllowedForHost(host2, tor_storage_partition));
  EXPECT_FALSE(main_state->IsHttpAllowedForHost(host3, main_storage_partition));
  EXPECT_FALSE(incognito_state->IsHttpAllowedForHost(
      host3, incognito_storage_partition));
  EXPECT_TRUE(tor_state->IsHttpAllowedForHost(host3, tor_storage_partition));
}

class BraveTorBrowserTest_EnableTorHttpsOnlyFlag
    : public BraveTorBrowserTest,
      public ::testing::WithParamInterface<bool> {
 public:
  BraveTorBrowserTest_EnableTorHttpsOnlyFlag() {
    if (IsBraveHttpsByDefaultEnabled()) {
      std::vector<base::test::FeatureRef> enabled_features{
          net::features::kBraveTorWindowsHttpsOnly};
      std::vector<base::test::FeatureRef> disabled_features;
      if (IsBraveHttpsByDefaultEnabled()) {
        enabled_features.push_back(net::features::kBraveHttpsByDefault);
      } else {
        disabled_features.push_back(net::features::kBraveHttpsByDefault);
      }
      scoped_feature_list_.InitWithFeatures(enabled_features,
                                            disabled_features);
    }
  }

  ~BraveTorBrowserTest_EnableTorHttpsOnlyFlag() override = default;

  bool IsBraveHttpsByDefaultEnabled() { return GetParam(); }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(BraveTorBrowserTest_EnableTorHttpsOnlyFlag,
                       TorWindowHttpsOnly) {
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
  DownloadTorClient();

  Profile* tor_profile = OpenTorWindow();
  PrefService* prefs = tor_profile->GetPrefs();
  // Check that HTTPS-Only Mode has been enabled for the Tor window.
  EXPECT_TRUE(prefs->GetBoolean(prefs::kHttpsOnlyModeEnabled));
}

INSTANTIATE_TEST_SUITE_P(BraveTorBrowserTest_EnableTorHttpsOnlyFlag,
                         BraveTorBrowserTest_EnableTorHttpsOnlyFlag,
                         ::testing::Bool());
