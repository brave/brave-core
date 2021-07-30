/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/bind.h"
#include "base/path_service.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/common/brave_paths.h"
#include "brave/components/speedreader/features.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_switches.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

const char kTestHost[] = "theguardian.com";
const char kTestPage[] = "/guardian.html";
const base::FilePath::StringPieceType kTestWhitelist =
    FILE_PATH_LITERAL("speedreader_whitelist.json");

constexpr char kSpeedreaderToggleUMAHistogramName[] =
    "Brave.SpeedReader.ToggleCount";

constexpr char kSpeedreaderEnabledUMAHistogramName[] =
    "Brave.SpeedReader.Enabled";

class SpeedReaderBrowserTest : public InProcessBrowserTest {
 public:
  SpeedReaderBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(speedreader::kSpeedreaderFeature);
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    EXPECT_TRUE(https_server_.Start());
  }

  SpeedReaderBrowserTest(const SpeedReaderBrowserTest&) = delete;
  SpeedReaderBrowserTest& operator=(const SpeedReaderBrowserTest&) = delete;

  ~SpeedReaderBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    base::FilePath whitelist_path = test_data_dir.Append(kTestWhitelist);
    command_line->AppendSwitchPath(speedreader::kSpeedreaderWhitelistPath,
                                   whitelist_path);
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  content::WebContents* ActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  speedreader::SpeedreaderService* speedreader_service() {
    return speedreader::SpeedreaderServiceFactory::GetForProfile(
        browser()->profile());
  }

  void ToggleSpeedreader() {
    auto* speedreader_service =
        speedreader::SpeedreaderServiceFactory::GetForProfile(
            browser()->profile());
    speedreader_service->ToggleSpeedreader();
    ActiveWebContents()->GetController().Reload(content::ReloadType::NORMAL,
                                                false);
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer https_server_;
};

// disabled in https://github.com/brave/brave-browser/issues/11328
IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, DISABLED_SmokeTest) {
  ToggleSpeedreader();
  const GURL url = https_server_.GetURL(kTestHost, kTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents = ActiveWebContents();
  content::RenderFrameHost* rfh = contents->GetMainFrame();

  const char kGetStyleLength[] =
      "document.getElementById(\"brave_speedreader_style\").innerHTML.length";

  const char kGetContentLength[] = "document.body.innerHTML.length";

  // Check that the document became much smaller and that non-empty speedreader
  // style is injected.
  EXPECT_LT(0, content::EvalJs(rfh, kGetStyleLength));
  EXPECT_GT(17750 + 1, content::EvalJs(rfh, kGetContentLength));

  // Check that disabled speedreader doesn't affect the page.
  ToggleSpeedreader();
  ui_test_utils::NavigateToURL(browser(), url);
  rfh = contents->GetMainFrame();
  EXPECT_LT(106000, content::EvalJs(rfh, kGetContentLength));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, P3ATest) {
  base::HistogramTester tester;

  const GURL url = https_server_.GetURL(kTestHost, kTestPage);
  ui_test_utils::NavigateToURL(browser(), url);

  // SpeedReader never enabled
  EXPECT_FALSE(speedreader_service()->IsEnabled());
  tester.ExpectBucketCount(kSpeedreaderEnabledUMAHistogramName, 0, 1);
  tester.ExpectBucketCount(kSpeedreaderToggleUMAHistogramName, 0, 1);

  // SpeedReader recently enabled, toggled once
  ToggleSpeedreader();
  tester.ExpectBucketCount(kSpeedreaderEnabledUMAHistogramName, 2, 1);
  tester.ExpectBucketCount(kSpeedreaderToggleUMAHistogramName, 1, 1);
  tester.ExpectBucketCount(kSpeedreaderToggleUMAHistogramName, 2, 0);
}
