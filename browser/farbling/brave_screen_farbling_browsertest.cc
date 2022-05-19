/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"

using brave_shields::ControlType;

class BraveScreenFarblingBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content_client_.reset(new ChromeContentClient);
    content::SetContentClient(content_client_.get());
    browser_content_client_.reset(new BraveContentBrowserClient());
    content::SetBrowserClientForTesting(browser_content_client_.get());

    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    content::SetupCrossSiteRedirector(https_server_.get());

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(https_server_->Start());
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW,
        https_server()->GetURL(domain, "/"));
  }

  void SetFingerprintingDefault(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT,
        https_server()->GetURL(domain, "/"));
  }

  content::WebContents* contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    return WaitForLoadStop(contents());
  }

  Browser* OpenPopup(const std::string& script) const {
    content::ExecuteScriptAsync(contents(), script);
    Browser* popup = ui_test_utils::WaitForBrowserToOpen();
    EXPECT_NE(popup, browser());
    auto* popup_contents = popup->tab_strip_model()->GetActiveWebContents();
    EXPECT_TRUE(WaitForRenderFrameReady(popup_contents->GetMainFrame()));
    return popup;
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

 private:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
  std::vector<std::string> user_agents_;
};

const char* testScript[] = {"window.screenX",
                            "window.screenY",
                            "window.screen.availLeft",
                            "window.screen.availTop",
                            "window.outerWidth - window.innerWidth",
                            "window.outerHeight - window.innerHeight",
                            "window.screen.availWidth - window.innerWidth",
                            "window.screen.availHeight - window.innerHeight",
                            "window.screen.width - window.innerWidth",
                            "window.screen.height - window.innerHeight"};

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest, FarbleScreenSize) {
  std::string domain = "a.test";
  GURL url = https_server()->GetURL(domain, "/simple.html");
  AllowFingerprinting(domain);
  NavigateToURLUntilLoadStop(url);
  for (int i = 0; i < static_cast<int>(std::size(testScript)); ++i) {
    EXPECT_LE(0, EvalJs(contents(), testScript[i]));
  }

  SetFingerprintingDefault(domain);
  NavigateToURLUntilLoadStop(url);
  for (int i = 0; i < static_cast<int>(std::size(testScript)); ++i) {
    EXPECT_GE(8, EvalJs(contents(), testScript[i]));
  }
}

const char* mediaQueryTestScripts[] = {
    "matchMedia(`(max-device-width: ${innerWidth + 8}px) and "
    "(min-device-width: ${innerWidth}px)`).matches",
    "matchMedia(`(max-device-height: ${innerHeight + 8}px) and "
    "(min-device-height: ${innerHeight}px)`).matches"};

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest, FarbleScreenMediaQuery) {
  std::string domain = "a.test";
  GURL url = https_server()->GetURL(domain, "/simple.html");
  SetFingerprintingDefault(domain);
  NavigateToURLUntilLoadStop(url);
  for (int i = 0; i < static_cast<int>(std::size(mediaQueryTestScripts)); ++i) {
    EXPECT_EQ(true, EvalJs(contents(), mediaQueryTestScripts[i]));
  }
  AllowFingerprinting(domain);
  for (int i = 0; i < static_cast<int>(std::size(mediaQueryTestScripts)); ++i) {
    EXPECT_EQ(false, EvalJs(contents(), mediaQueryTestScripts[i]));
  }
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest,
                       FarbleScreenPopupPosition) {
  std::string domain = "a.test";
  GURL url = https_server()->GetURL(domain, "/simple.html");
  SetFingerprintingDefault(domain);
  NavigateToURLUntilLoadStop(url);
  gfx::Rect parentBounds = browser()->window()->GetBounds();
  const char* script =
      "open('http://d.test/', '', `left=${screen.availLeft + "
      "50},top=${screen.availTop + 50},width=50,height=50`);";
  Browser* popup = OpenPopup(script);
  gfx::Rect childBounds = popup->window()->GetBounds();
  printf("%d %d %d %d\n", childBounds.x(), parentBounds.x(), childBounds.y(),
         parentBounds.y());
  EXPECT_GT(childBounds.x(), 50 + parentBounds.x());
  EXPECT_GT(childBounds.y(), 50 + parentBounds.y());
}
