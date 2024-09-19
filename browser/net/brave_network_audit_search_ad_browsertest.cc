/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <vector>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_file_value_serializer.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "base/test/test_timeouts.h"
#include "base/time/time.h"
#include "brave/browser/net/brave_network_audit_allowed_lists.h"
#include "brave/browser/net/brave_network_audit_test_helper.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave {
namespace {

// Both timeouts in seconds
const int kMaxTimeoutForAdsServiceInit = 10;
const int kMaxTimeoutPerLoadedURL = 30;
const char kEmbeddedTestServerDirectory[] = "brave_ads";
const char kDomain[] = "search.brave.com";
const char kBraveSearchPath[] = "/search_result_ad_click.html";

void WaitForTimeout(int timeout) {
  base::test::ScopedRunLoopTimeout file_download_timeout(
      FROM_HERE, base::Seconds(kMaxTimeoutPerLoadedURL + 1));
  base::RunLoop run_loop;
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::Seconds(timeout));
  run_loop.Run();
}

class BraveNetworkAuditSearchAdTest : public InProcessBrowserTest {
 public:
  BraveNetworkAuditSearchAdTest() {
    feature_list_.InitWithFeatures(
        {brave_ads::kShouldAlwaysRunBraveAdsServiceFeature,
         brave_ads::kShouldAlwaysTriggerBraveSearchResultAdEventsFeature},
        {});
  }

  BraveNetworkAuditSearchAdTest(const BraveNetworkAuditSearchAdTest&) = delete;
  BraveNetworkAuditSearchAdTest& operator=(
      const BraveNetworkAuditSearchAdTest&) = delete;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    https_server_->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(https_server_->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    base::FilePath source_root_path =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);

    // Full log containing all the network requests.
    net_log_path_ = source_root_path.AppendASCII("network_search_ad_log.json");

    // Log containing the results of the audit only.
    audit_results_path_ =
        source_root_path.AppendASCII("network_audit_search_ad_results.json");

    command_line->AppendSwitchPath(network::switches::kLogNetLog,
                                   net_log_path_);
    command_line->AppendSwitchASCII(network::switches::kNetLogCaptureMode,
                                    "Everything");
    mock_cert_verifier_.SetUpCommandLine(command_line);
    InProcessBrowserTest::SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    std::string port =
        base::NumberToString(https_server()->host_port_pair().port());
    std::vector<std::string> allowed_prefixes;

    // Brave search
    // The test simulation has a pattern https://search.brave.com:<port>
    // port is changed dynamically
    const char kAllowedBraveSearchTemplate[] = "https://search.brave.com:%s/";
    allowed_prefixes.push_back(
        base::StringPrintf(kAllowedBraveSearchTemplate, port.c_str()));
    VerifyNetworkAuditLog(net_log_path_, audit_results_path_, allowed_prefixes);
  }

  Profile* profile() { return browser()->profile(); }
  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;

  base::FilePath net_log_path_;
  base::FilePath audit_results_path_;
};

IN_PROC_BROWSER_TEST_F(BraveNetworkAuditSearchAdTest, SearchAdTest) {
  WaitForTimeout(kMaxTimeoutForAdsServiceInit);
  GURL url = https_server()->GetURL(kDomain, kBraveSearchPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForTimeout(kMaxTimeoutPerLoadedURL);

  EXPECT_TRUE(content::ExecJs(contents,
                              "document.getElementById('ad_link_1').click();"));
}

}  // namespace
}  // namespace brave
