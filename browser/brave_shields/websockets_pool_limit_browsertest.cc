/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "net/test/test_data_directory.h"
#include "third_party/blink/public/common/features.h"
#include "url/gurl.h"

namespace {

const int kWebSocketsPoolLimit = 10;

constexpr char kWsOpenScript[] = R"(
  if (typeof sockets == 'undefined') {
    sockets = []
  }
  new Promise(resolve => {
    socket = new WebSocket($1);
    sockets.push(socket);
    socket.addEventListener('open', () => resolve('open'));
    socket.addEventListener('error', () => resolve('error'));
  });
)";

constexpr char kWsCloseScript[] = R"(
  sockets[$1].close();
)";

}  // namespace

class WebSocketsPoolLimitBrowserTest : public InProcessBrowserTest {
 public:
  WebSocketsPoolLimitBrowserTest() = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_.Start());

    ws_server_ = std::make_unique<net::SpawnedTestServer>(
        net::SpawnedTestServer::TYPE_WS, net::GetWebSocketTestDataDirectory());
    ASSERT_TRUE(ws_server_->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* GetWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

 protected:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
  std::unique_ptr<net::SpawnedTestServer> ws_server_;
};

IN_PROC_BROWSER_TEST_F(WebSocketsPoolLimitBrowserTest, PoolIsLimitedByDefault) {
  const GURL url(https_server_.GetURL("a.com", "/simple.html"));
  EXPECT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  const std::string& ws_open_script = content::JsReplace(
      kWsOpenScript, ws_server_->GetURL("a.com", "echo-with-no-extension"));
  for (int i = 0; i < kWebSocketsPoolLimit; ++i) {
    EXPECT_EQ("open", content::EvalJs(GetWebContents(), ws_open_script));
  }

  // All new WebSocket instances should fail to open after a limit is hit.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ("error", content::EvalJs(GetWebContents(), ws_open_script));
  }

  // Close 5 sockets.
  for (int i = 0; i < 5; ++i) {
    EXPECT_TRUE(content::ExecJs(GetWebContents(),
                                content::JsReplace(kWsCloseScript, i)));
  }

  // Expect 5 new sockets can be opened.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ("open", content::EvalJs(GetWebContents(), ws_open_script));
  }

  // All new WebSocket instances should fail to open after a limit is hit.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ("error", content::EvalJs(GetWebContents(), ws_open_script));
  }
}

IN_PROC_BROWSER_TEST_F(WebSocketsPoolLimitBrowserTest,
                       PoolIsNotLimitedWithDisabledShields) {
  const GURL url(https_server_.GetURL("a.com", "/simple.html"));
  // Disable shields.
  brave_shields::SetBraveShieldsEnabled(content_settings(), false, url);

  EXPECT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  const std::string& ws_open_script = content::JsReplace(
      kWsOpenScript, ws_server_->GetURL("a.com", "echo-with-no-extension"));
  // No limits should be active.
  for (int i = 0; i < kWebSocketsPoolLimit + 5; ++i) {
    EXPECT_EQ("open", content::EvalJs(GetWebContents(), ws_open_script));
  }
}

class WebSocketsPoolLimitDisabledBrowserTest
    : public WebSocketsPoolLimitBrowserTest {
 public:
  WebSocketsPoolLimitDisabledBrowserTest() {
    scoped_feature_list_.InitAndDisableFeature(
        blink::features::kRestrictWebSocketsPool);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(WebSocketsPoolLimitDisabledBrowserTest,
                       PoolIsNotLimited) {
  const GURL url(https_server_.GetURL("a.com", "/simple.html"));
  EXPECT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  const std::string& ws_open_script = content::JsReplace(
      kWsOpenScript, ws_server_->GetURL("a.com", "echo-with-no-extension"));
  // No limits should be active.
  for (int i = 0; i < kWebSocketsPoolLimit + 5; ++i) {
    EXPECT_EQ("open", content::EvalJs(GetWebContents(), ws_open_script));
  }
}
