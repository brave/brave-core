/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/permissions/permission_request.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "third_party/blink/public/common/device_memory/approximated_device_memory.h"

using brave_shields::ControlType;

const char kDeviceMemoryScript[] = "navigator.deviceMemory * 1024";

class BraveDeviceMemoryFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveDeviceMemoryFarblingBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    EXPECT_TRUE(https_server_.Start());
  }

  BraveDeviceMemoryFarblingBrowserTest(
      const BraveDeviceMemoryFarblingBrowserTest&) = delete;
  BraveDeviceMemoryFarblingBrowserTest& operator=(
      const BraveDeviceMemoryFarblingBrowserTest&) = delete;

  ~BraveDeviceMemoryFarblingBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content_client_.reset(new ChromeContentClient);
    content::SetContentClient(content_client_.get());
    browser_content_client_.reset(new BraveContentBrowserClient());
    content::SetBrowserClientForTesting(browser_content_client_.get());

    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer https_server_;

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW,
        https_server_.GetURL(domain, "/"));
  }

  void BlockFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK,
        https_server_.GetURL(domain, "/"));
  }

  void SetFingerprintingDefault(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT,
        https_server_.GetURL(domain, "/"));
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    ui_test_utils::NavigateToURL(browser(), url);
    return WaitForLoadStop(contents());
  }

 private:
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

// Tests results of farbling known values
IN_PROC_BROWSER_TEST_F(BraveDeviceMemoryFarblingBrowserTest,
                       FarbleDeviceMemory) {
  std::string domain1 = "b.com";
  std::string domain2 = "z.com";
  GURL url1 = https_server_.GetURL(domain1, "/simple.html");
  GURL url2 = https_server_.GetURL(domain2, "/simple.html");
  // set memory to 10GB
  blink::ApproximatedDeviceMemory::SetPhysicalMemoryMBForTesting(1024 * 10);
  int true_value =
      blink::ApproximatedDeviceMemory::GetApproximatedDeviceMemory() * 1024;
  EXPECT_EQ(true_value, 8192);
  // Farbling level: off
  AllowFingerprinting(domain1);
  NavigateToURLUntilLoadStop(url1);
  EXPECT_EQ(true_value, EvalJs(contents(), kDeviceMemoryScript));
  AllowFingerprinting(domain2);
  NavigateToURLUntilLoadStop(url2);
  EXPECT_EQ(true_value, EvalJs(contents(), kDeviceMemoryScript));

  // Farbling level: default
  SetFingerprintingDefault(domain1);
  NavigateToURLUntilLoadStop(url1);
  EXPECT_EQ(4096, EvalJs(contents(), kDeviceMemoryScript));
  SetFingerprintingDefault(domain2);
  NavigateToURLUntilLoadStop(url2);
  EXPECT_EQ(2048, EvalJs(contents(), kDeviceMemoryScript));

  // Farbling level: maximum
  BlockFingerprinting(domain1);
  NavigateToURLUntilLoadStop(url1);
  EXPECT_EQ(1024, EvalJs(contents(), kDeviceMemoryScript));
  AllowFingerprinting(domain2);
  NavigateToURLUntilLoadStop(url2);
  EXPECT_EQ(8192, EvalJs(contents(), kDeviceMemoryScript));
}
