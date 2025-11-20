/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <vector>

#include "base/base_paths.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/bind.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "base/test/test_timeouts.h"
#include "base/time/time.h"
#include "brave/browser/net/brave_network_audit_allowed_lists.h"
#include "brave/browser/net/brave_network_audit_test_helper.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "chrome/browser/password_manager/profile_password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/test/browser_test.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gmock/include/gmock/gmock.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/components/playlist/core/common/features.h"
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
#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
    scoped_feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
#endif  // BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  }

  BraveNetworkAuditTest(const BraveNetworkAuditTest&) = delete;
  BraveNetworkAuditTest& operator=(const BraveNetworkAuditTest&) = delete;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    auto* template_url_service =
        TemplateURLServiceFactory::GetForProfile(browser()->profile());
    search_test_utils::WaitForTemplateURLServiceToLoad(template_url_service);

    auto* google_template_url =
        template_url_service->GetTemplateURLForHost("www.google.com");
    ASSERT_TRUE(google_template_url);
    template_url_service->SetUserSelectedDefaultSearchProvider(
        google_template_url);

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    base::FilePath source_root_path =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);

    // Full log containing all the network requests.
    net_log_path_ = source_root_path.AppendASCII("network_log.json");

    // Log containing the results of the audit only.
    audit_results_path_ =
        source_root_path.AppendASCII("network_audit_results.json");

    command_line->AppendSwitchPath(network::switches::kLogNetLog,
                                   net_log_path_);
    command_line->AppendSwitchASCII(network::switches::kNetLogCaptureMode,
                                    "Everything");
  }

  void TearDownInProcessBrowserTestFixture() override {
    VerifyNetworkAuditLog(net_log_path_, audit_results_path_,
                          std::vector<std::string>());
  }

  Profile* profile() { return browser()->profile(); }

 private:
  base::FilePath net_log_path_;
  base::FilePath audit_results_path_;

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  base::test::ScopedFeatureList scoped_feature_list_;
#endif  // BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
};

// Loads brave://welcome first to simulate a first run and then loads another
// URL, waiting some time after each load to allow gathering network requests.
IN_PROC_BROWSER_TEST_F(BraveNetworkAuditTest, BasicTests) {
  // Load the Welcome page.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("brave://welcome")));
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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("brave://newtab")));
  WaitForTimeout(kMaxTimeoutPerLoadedURL);

  // Load a simple HTML page from the test server.
  GURL simple_url(embedded_test_server()->GetURL("/simple.html"));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), simple_url));
  WaitForTimeout(kMaxTimeoutPerLoadedURL);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("brave://wallet")));
  WaitForTimeout(kMaxTimeoutPerLoadedURL);

#if defined(TOOLKIT_VIEWS)
  auto* sidebar_controller = browser()->GetFeatures().sidebar_controller();
  auto* sidebar_model = sidebar_controller->model();
  const auto& all_items = sidebar_model->GetAllSidebarItems();
  const int item_num = all_items.size();
  for (int i = 0; i < item_num; ++i) {
    auto item = all_items[i];
    // Load all builtin panel items.
    if (item.is_built_in_type() && item.open_in_panel) {
      sidebar_controller->ActivateItemAt(i);
      WaitForTimeout(kMaxTimeoutPerLoadedURL);
    }
  }
#endif
}

}  // namespace
}  // namespace brave
