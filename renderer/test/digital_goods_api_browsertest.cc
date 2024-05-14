/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

class DigitalGoodsAPIBrowserTest : public InProcessBrowserTest,
                                   public ::testing::WithParamInterface<bool> {
 public:
  DigitalGoodsAPIBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.ServeFilesFromDirectory(test_data_dir);
  }

  ~DigitalGoodsAPIBrowserTest() override = default;

  bool IsDigitalGoodsAPIEnabled() { return GetParam(); }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    if (IsDigitalGoodsAPIEnabled()) {
#if BUILDFLAG(IS_ANDROID)
      command_line->AppendSwitch(
          switches::kEnableExperimentalWebPlatformFeatures);
#else
      command_line->AppendSwitch(switches::kEnableBlinkTestFeatures);
#endif
    }
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    EXPECT_TRUE(https_server_.Start());
    // Map all hosts to localhost.
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* primary_main_frame() {
    return web_contents()->GetPrimaryMainFrame();
  }

 protected:
  net::EmbeddedTestServer https_server_;
};

// The API is unavailable in /1 variation even though it should be available.
// Disabling for now. TODO(https://github.com/brave/brave-browser/issues/37883)
IN_PROC_BROWSER_TEST_P(DigitalGoodsAPIBrowserTest, DISABLED_DigitalGoods) {
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  auto result =
      content::EvalJs(primary_main_frame(), "window.getDigitalGoodsService()");
  if (IsDigitalGoodsAPIEnabled()) {
    EXPECT_TRUE(result.error.find(
                    "Failed to execute 'getDigitalGoodsService' on "
                    "'Window': 1 argument required, but only 0 present.") !=
                std::string::npos)
        << result.error;
  } else {
    EXPECT_TRUE(
        result.error.find("window.getDigitalGoodsService is not a function") !=
        std::string::npos)
        << result.error;
  }
}

INSTANTIATE_TEST_SUITE_P(DigitalGoodsAPIBrowserTest,
                         DigitalGoodsAPIBrowserTest,
                         ::testing::Bool());
