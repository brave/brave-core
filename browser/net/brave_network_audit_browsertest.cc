/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_file_value_serializer.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/bind.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "base/test/test_timeouts.h"
#include "base/time/time.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/net/brave_network_audit_allowed_lists.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "chrome/browser/password_manager/profile_password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/re2/src/re2/re2.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/sidebar/sidebar_item.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/components/playlist/common/features.h"
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
const int kMaxTimeoutPerLoadedURL = 30;

// Based on the implementation of isPrivateIP() from NPM's "ip" module.
// See https://github.com/indutny/node-ip/blob/master/lib/ip.js
constexpr const char* kPrivateIPRegexps[] = {
    "(::f{4}:)?10\\.([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})",
    "(::f{4}:)?192\\.168\\.([0-9]{1,3})\\.([0-9]{1,3})",
    "(::f{4}:)?172\\.(1[6-9]|2\\d|30|31)\\.([0-9]{1,3})\\.([0-9]{1,3})",
    "(::f{4}:)?127\\.([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})",
    "(::f{4}:)?169\\.254\\.([0-9]{1,3})\\.([0-9]{1,3})",
    "f[cd][0-9a-f]{2}:.*",
    "fe80:.*",
    "::1",
    "::"};

void WaitForTimeout(int timeout) {
  base::test::ScopedRunLoopTimeout file_download_timeout(
      FROM_HERE, base::Seconds(kMaxTimeoutPerLoadedURL + 1));
  base::RunLoop run_loop;
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::Seconds(timeout));
  run_loop.Run();
}

bool isPrivateURL(const GURL& url) {
  for (const char* regexp : kPrivateIPRegexps) {
    if (RE2::FullMatch(url.host(), regexp)) {
      return true;
    }
  }
  return false;
}

bool PerformNetworkAuditProcess(base::Value::List* events) {
  DCHECK(events);

  bool failed = false;
  events->EraseIf([&failed](base::Value& event_value) {
    base::Value::Dict* event_dict = event_value.GetIfDict();
    EXPECT_TRUE(event_dict);

    std::optional<int> event_type = event_dict->FindInt("type");
    EXPECT_TRUE(event_type.has_value());

    // Showing these helps determine which URL requests which don't
    // actually hit the network.
    if (static_cast<net::NetLogEventType>(event_type.value()) ==
        net::NetLogEventType::URL_REQUEST_FAKE_RESPONSE_HEADERS_CREATED) {
      return false;
    }

    const auto* source_dict = event_dict->FindDict("source");
    EXPECT_TRUE(source_dict);

    // Consider URL requests only.
    std::optional<int> source_type = source_dict->FindInt("type");
    EXPECT_TRUE(source_type.has_value());

    if (static_cast<net::NetLogSourceType>(source_type.value()) !=
        net::NetLogSourceType::URL_REQUEST) {
      return true;
    }

    // Discard events without URLs in the parameters.
    if (!event_dict->Find("params"))
      return true;

    const auto* params_dict2 = event_dict->FindDict("params");
    EXPECT_TRUE(params_dict2);

    if (!params_dict2->Find("url"))
      return true;

    const std::string* url_str = params_dict2->FindString("url");
    EXPECT_TRUE(url_str);

    GURL url(*url_str);
    if (!url.is_valid()) {
      // Network requests to invalid URLs don't pose a threat and can happen in
      // dev-only environments (e.g. building with brave_stats_updater_url="").
      return true;
    }

    if (RE2::FullMatch(url.host(), "[a-z]+")) {
      // Chromium sometimes sends requests to random non-resolvable hosts.
      return true;
    }

    for (const char* protocol : kAllowedUrlProtocols) {
      if (protocol == url.scheme()) {
        return true;
      }
    }

    bool found_prefix = false;
    for (const char* prefix : kAllowedUrlPrefixes) {
      if (!url.spec().rfind(prefix, 0)) {
        found_prefix = true;
        break;
      }
    }

    bool found_pattern = false;
    for (const char* pattern : kAllowedUrlPatterns) {
      if (RE2::FullMatch(url.spec(), pattern)) {
        found_pattern = true;
        break;
      }
    }

    if (!found_prefix && !found_pattern) {
      // Check if the URL is a private IP.
      if (isPrivateURL(url)) {
        // Warn but don't fail the audit.
        LOG(WARNING) << "NETWORK AUDIT WARNING:" << url.spec() << std::endl;
        return false;
      }

      LOG(ERROR) << "NETWORK AUDIT FAIL:" << url.spec() << std::endl;
      failed = true;
    }

    return false;
  });

  return !failed;
}

void WriteNetworkAuditResultsToDisk(const base::Value::Dict& results_dic,
                                    const base::FilePath& path) {
  std::string results;
  JSONFileValueSerializer serializer(path);
  serializer.Serialize(results_dic);

  LOG(INFO) << "Network audit results stored in " << path << std::endl;
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

    ASSERT_TRUE(embedded_test_server()->Start());

    // Create and start the Rewards service
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile()));
    base::RunLoop run_loop;
    rewards_service_->StartProcessForTesting(run_loop.QuitClosure());
    run_loop.Run();
  }

  void TearDownOnMainThread() override {
    rewards_service_->Shutdown();
    InProcessBrowserTest::TearDownOnMainThread();
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
    VerifyNetworkAuditLog();
  }

  bool EnableBraveRewards() {
    PrefService* pref_service = profile()->GetPrefs();
    pref_service->SetInteger("brave.rewards.version", 7);
    pref_service->SetBoolean("brave.rewards.enabled", true);
    return pref_service->GetBoolean("brave.rewards.enabled");
  }

  Profile* profile() { return browser()->profile(); }

 private:
  // Verify that the netlog file was written, appears to be well formed, and
  // includes the requested level of data.
  void VerifyNetworkAuditLog() {
    // Read the netlog from disk.
    std::string file_contents;
    ASSERT_TRUE(base::ReadFileToString(net_log_path_, &file_contents))
        << "Could not read: " << net_log_path_;

    // Parse it as JSON.
    auto parsed = base::JSONReader::Read(file_contents);
    ASSERT_TRUE(parsed.has_value());

    // Ensure the root value is a dictionary.
    auto* main = parsed->GetIfDict();
    ASSERT_TRUE(main);

    // Ensure it has a "constants" property.
    auto* constants = main->FindDict("constants");
    ASSERT_TRUE(constants);
    ASSERT_FALSE(constants->empty());

    // Ensure it has an "events" property.
    auto* events = main->FindList("events");
    ASSERT_TRUE(events);
    ASSERT_FALSE(events->empty());

    EXPECT_TRUE(PerformNetworkAuditProcess(events))
        << "network-audit FAILED. Import " << net_log_path_.AsUTF8Unsafe()
        << " in chrome://net-internals for more details.";

    // Write results of the audit to disk, useful for further debugging.
    WriteNetworkAuditResultsToDisk(*main, audit_results_path_);
    ASSERT_TRUE(base::PathExists(audit_results_path_));
  }

  raw_ptr<brave_rewards::RewardsServiceImpl> rewards_service_ = nullptr;
  base::FilePath net_log_path_;
  base::FilePath audit_results_path_;

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  base::test::ScopedFeatureList scoped_feature_list_;
#endif  // BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
};

// Loads brave://welcome first to simulate a first run and then loads another
// URL, and finally enables brave rewards, waiting some time after each load to
// allow gathering network requests.
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

  // Finally, load brave://rewards and enable Brave Rewards.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("brave://rewards")));
  ASSERT_TRUE(EnableBraveRewards());
  WaitForTimeout(kMaxTimeoutPerLoadedURL);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("brave://wallet")));
  WaitForTimeout(kMaxTimeoutPerLoadedURL);

#if defined(TOOLKIT_VIEWS)
  auto* sidebar_controller =
      static_cast<BraveBrowser*>(browser())->sidebar_controller();
  auto* sidebar_model = sidebar_controller->model();
  const auto& all_items = sidebar_model->GetAllSidebarItems();
  const int item_num = all_items.size();
  const int builtin_panel_item_total = 4;
  int builtin_panel_item_count = 0;
  for (int i = 0; i < item_num; ++i) {
    auto item = all_items[i];
    // Load all builtin panel items.
    if (item.IsBuiltInType() && item.open_in_panel) {
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

}  // namespace
}  // namespace brave
