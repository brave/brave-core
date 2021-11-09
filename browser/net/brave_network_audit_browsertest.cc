// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_file_value_serializer.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/net/brave_network_audit_allowed_lists.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave {
namespace {

// Max amount of time to wait after getting an URL loaded, in milliseconds. Note
// that the value passed to --ui-test-action-timeout in //brave/package.json, as
// part of the 'network-audit' script, must be big enough to accomodate this.
//
// In particular:
//   --ui-test-action-timeout: should be greater than |kMaxTimeoutPerLoadedURL|.
//   --test-launcher-timeout: should be able to fit the total sum of timeouts.
const int kMaxTimeoutPerLoadedURL = 300000;

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
  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::Milliseconds(timeout));
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

bool PerformNetworkAuditProcess(base::Value* events) {
  DCHECK(events && events->is_list());

  bool failed = false;
  events->EraseListValueIf([&failed](base::Value& event_value) {
    base::DictionaryValue* event_dict;
    EXPECT_TRUE(event_value.GetAsDictionary(&event_dict));

    absl::optional<int> event_type = event_dict->FindIntPath("type");
    EXPECT_TRUE(event_type.has_value());

    // Showing these helps determine which URL requests which don't
    // actually hit the network.
    if (static_cast<net::NetLogEventType>(event_type.value()) ==
        net::NetLogEventType::URL_REQUEST_FAKE_RESPONSE_HEADERS_CREATED) {
      return false;
    }

    const base::Value* source_dict = event_dict->FindDictPath("source");
    EXPECT_TRUE(source_dict);

    // Consider URL requests only.
    absl::optional<int> source_type = source_dict->FindIntPath("type");
    EXPECT_TRUE(source_type.has_value());

    if (static_cast<net::NetLogSourceType>(source_type.value()) !=
        net::NetLogSourceType::URL_REQUEST) {
      return true;
    }

    // Discard events without URLs in the parameters.
    if (!event_dict->FindKey("params"))
      return true;

    const base::Value* params_dict2 = event_dict->FindDictPath("params");
    EXPECT_TRUE(params_dict2);

    if (!params_dict2->FindKey("url"))
      return true;

    const std::string* url_str = params_dict2->FindStringPath("url");
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

void WriteNetworkAuditResultsToDisk(const base::DictionaryValue& results_dic,
                                    const base::FilePath& path) {
  std::string results;
  JSONFileValueSerializer serializer(path);
  serializer.Serialize(results_dic);

  LOG(INFO) << "Network audit results stored in " << path << std::endl;
}

class BraveNetworkAuditTest : public InProcessBrowserTest {
 public:
  BraveNetworkAuditTest() = default;
  BraveNetworkAuditTest(const BraveNetworkAuditTest&) = delete;
  BraveNetworkAuditTest& operator=(const BraveNetworkAuditTest&) = delete;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    ASSERT_TRUE(embedded_test_server()->Start());

    // Create and start the Rewards service
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile()));
    base::RunLoop run_loop;
    rewards_service_->StartProcess(run_loop.QuitClosure());
    run_loop.Run();
  }

  void TearDownOnMainThread() override {
    rewards_service_->Shutdown();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    base::FilePath source_root_path;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &source_root_path);

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
    base::DictionaryValue* main;
    ASSERT_TRUE(parsed->GetAsDictionary(&main));

    // Ensure it has a "constants" property.
    base::Value* constants = main->FindDictPath("constants");
    ASSERT_TRUE(constants && constants->is_dict());
    ASSERT_FALSE(constants->DictEmpty());

    // Ensure it has an "events" property.
    base::Value* events = main->FindListPath("events");
    ASSERT_TRUE(events && events->is_list());
    ASSERT_FALSE(events->GetList().empty());

    EXPECT_TRUE(PerformNetworkAuditProcess(events))
        << "network-audit FAILED. Import " << net_log_path_.AsUTF8Unsafe()
        << " in chrome://net-internals for more details.";

    // Write results of the audit to disk, useful for further debugging.
    WriteNetworkAuditResultsToDisk(*main, audit_results_path_);
    ASSERT_TRUE(base::PathExists(audit_results_path_));
  }

  brave_rewards::RewardsServiceImpl* rewards_service_;
  base::FilePath net_log_path_;
  base::FilePath audit_results_path_;
};

// Loads brave://welcome first to simulate a first run and then loads another
// URL, and finally enables brave rewards, waiting some time after each load to
// allow gathering network requests.
IN_PROC_BROWSER_TEST_F(BraveNetworkAuditTest, BasicTests) {
  // Load the Welcome page.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("brave://welcome")));
  WaitForTimeout(kMaxTimeoutPerLoadedURL);

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
}

}  // namespace
}  // namespace brave
