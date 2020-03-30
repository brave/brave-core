/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_switches.h"

#include "base/bind.h"
#include "base/path_service.h"
#include "brave/app/brave_command_ids.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

const char kTestPage[] = "/guardian.html";

class SpeedReaderBrowserTest : public InProcessBrowserTest {
 public:
  SpeedReaderBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    https_server_.ServeFilesFromDirectory(test_data_dir);
    EXPECT_TRUE(https_server_.Start());
  }

  SpeedReaderBrowserTest(const SpeedReaderBrowserTest&) = delete;
  SpeedReaderBrowserTest& operator=(const SpeedReaderBrowserTest&) = delete;

  ~SpeedReaderBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(speedreader::kEnableSpeedreader);
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
  }

 protected:
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, SmokeTest) {
  chrome::ExecuteCommand(browser(), IDC_TOGGLE_SPEEDREADER);
  const GURL url = https_server_.GetURL(kTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::RenderFrameHost* rfh = contents->GetMainFrame();

  const char kGetStyle[] =
      "document.getElementById(\"brave_speedreader_style\").innerHTML";

  const char kGetContent[] = "document.body.innerHTML";

  // Check that the document became much smaller and that non-empty speedreader
  // style is injected.
  EXPECT_LT(0ull, content::EvalJs(rfh, kGetStyle).ExtractString().size());
  EXPECT_GT(4096ull, content::EvalJs(rfh, kGetContent).ExtractString().size());

  // Check that disabled speedreader doesn't affect the page.
  chrome::ExecuteCommand(browser(), IDC_TOGGLE_SPEEDREADER);
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_LT(106000ull,
            content::EvalJs(rfh, kGetContent).ExtractString().size());

}
