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

constexpr char kRegisterSwScript[] = R"(
  (async () => {
    await navigator.serviceWorker.register($1, {scope: './'});
    const registration = await navigator.serviceWorker.ready;
  })();
)";

constexpr char kWsOpenInSwScript[] = R"(
  (async () => {
    const registration = await navigator.serviceWorker.ready;
    const result = new Promise(resolve => {
      navigator.serviceWorker.onmessage = event => {
        resolve(event.data);
      };
    });
    registration.active.postMessage({cmd: 'open_ws', url: $1});
    return await result;
  })();
)";

constexpr char kWsCloseInSwScript[] = R"(
  (async () => {
    const registration = await navigator.serviceWorker.ready;
    registration.active.postMessage({cmd: 'close_ws', idx: $1});
  })();
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
    content::SetupCrossSiteRedirector(&https_server_);
    ASSERT_TRUE(https_server_.Start());

    ws_server_ = std::make_unique<net::SpawnedTestServer>(
        net::SpawnedTestServer::TYPE_WSS, net::GetWebSocketTestDataDirectory());
    ASSERT_TRUE(ws_server_->Start());

    ws_url_ = ws_server_->GetURL("a.com", "echo-with-no-extension");
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

  content::RenderFrameHost* GetNthChildFrameWithHost(
      content::RenderFrameHost* main,
      base::StringPiece host,
      size_t n = 0) {
    size_t child_idx = 0;
    while (true) {
      auto* child_rfh = content::ChildFrameAt(main, child_idx++);
      if (!child_rfh) {
        return nullptr;
      }
      if (child_rfh->GetLastCommittedOrigin().host() == host) {
        if (!n)
          return child_rfh;
        --n;
      }
    }
  }

 protected:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_{
      net::test_server::EmbeddedTestServer::TYPE_HTTPS};
  std::unique_ptr<net::SpawnedTestServer> ws_server_;

  GURL ws_url_;
};

IN_PROC_BROWSER_TEST_F(WebSocketsPoolLimitBrowserTest, PoolIsLimitedByDefault) {
  const GURL url(https_server_.GetURL("a.com", "/simple.html"));
  EXPECT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  const std::string& ws_open_script =
      content::JsReplace(kWsOpenScript, ws_url_);
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
                       PoolIsKeyedByTopFrameOrigin) {
  const GURL a_com_url(
      https_server_.GetURL("a.com", "/ephemeral_storage.html"));
  const GURL b_com_url(
      https_server_.GetURL("b.com", "/ephemeral_storage.html"));

  // Open a.com with nested b.com.
  auto* a_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), a_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  auto* b_com0_in_a_com_rfh = GetNthChildFrameWithHost(a_com_rfh, "b.com");

  // Test WebSockets limit in nested b.com.
  const std::string& ws_open_script =
      content::JsReplace(kWsOpenScript, ws_url_);
  for (int i = 0; i < kWebSocketsPoolLimit; ++i) {
    EXPECT_EQ("open", content::EvalJs(b_com0_in_a_com_rfh, ws_open_script));
  }
  EXPECT_EQ("error", content::EvalJs(b_com0_in_a_com_rfh, ws_open_script));

  // Expect the limit is also active in another nested b.com.
  auto* b_com1_in_a_com_rfh = GetNthChildFrameWithHost(a_com_rfh, "b.com", 1);
  EXPECT_EQ("error", content::EvalJs(b_com1_in_a_com_rfh, ws_open_script));

  // Expect the limit is NOT active in the first-party a.com frame, bc the pool
  // is located in the a.com renderer process.
  auto* a_com_in_a_com_rfh = GetNthChildFrameWithHost(a_com_rfh, "a.com");
  EXPECT_EQ("open", content::EvalJs(a_com_in_a_com_rfh, ws_open_script));

  // Open b.com with a nested a.com.
  auto* b_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), b_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  auto* a_com_in_b_com_rfh = GetNthChildFrameWithHost(b_com_rfh, "a.com");

  // Test WebSockets limit in nested a.com.
  for (int i = 0; i < kWebSocketsPoolLimit; ++i) {
    EXPECT_EQ("open", content::EvalJs(a_com_in_b_com_rfh, ws_open_script));
  }
  EXPECT_EQ("error", content::EvalJs(a_com_in_b_com_rfh, ws_open_script));

  // Expect the limit is STILL NOT active in the first-party a.com frame.
  EXPECT_EQ("open", content::EvalJs(a_com_in_a_com_rfh, ws_open_script));
}

IN_PROC_BROWSER_TEST_F(WebSocketsPoolLimitBrowserTest,
                       SandboxedFramesAreLimited) {
  const GURL a_com_url(
      https_server_.GetURL("a.com", "/csp_sandboxed_frame.html"));
  auto* a_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), a_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  EXPECT_TRUE(a_com_rfh->GetLastCommittedOrigin().opaque());

  const std::string& ws_open_script =
      content::JsReplace(kWsOpenScript, ws_url_);
  for (int i = 0; i < kWebSocketsPoolLimit; ++i) {
    EXPECT_EQ("open", content::EvalJs(a_com_rfh, ws_open_script));
  }
  EXPECT_EQ("error", content::EvalJs(a_com_rfh, ws_open_script));

  auto* c_com_in_a_com_rfh = content::ChildFrameAt(a_com_rfh, 0);
  for (int i = 0; i < kWebSocketsPoolLimit; ++i) {
    EXPECT_EQ("open", content::EvalJs(c_com_in_a_com_rfh, ws_open_script));
  }
  EXPECT_EQ("error", content::EvalJs(c_com_in_a_com_rfh, ws_open_script));

  const GURL b_com_url(
      https_server_.GetURL("b.com", "/csp_sandboxed_frame.html"));
  auto* b_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), b_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  EXPECT_TRUE(b_com_rfh->GetLastCommittedOrigin().opaque());

  for (int i = 0; i < kWebSocketsPoolLimit; ++i) {
    EXPECT_EQ("open", content::EvalJs(b_com_rfh, ws_open_script));
  }
  EXPECT_EQ("error", content::EvalJs(b_com_rfh, ws_open_script));

  auto* c_com_in_b_com_rfh = content::ChildFrameAt(b_com_rfh, 0);
  for (int i = 0; i < kWebSocketsPoolLimit; ++i) {
    EXPECT_EQ("open", content::EvalJs(c_com_in_b_com_rfh, ws_open_script));
  }
  EXPECT_EQ("error", content::EvalJs(c_com_in_b_com_rfh, ws_open_script));
}

IN_PROC_BROWSER_TEST_F(WebSocketsPoolLimitBrowserTest, ServiceWorkerIsLimited) {
  const GURL url(https_server_.GetURL("a.com", "/simple.html"));

  EXPECT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  const std::string& register_sw_script = content::JsReplace(
      kRegisterSwScript, "service-worker-websockets-limit.js");
  ASSERT_TRUE(content::ExecJs(GetWebContents(), register_sw_script));

  const std::string& ws_open_in_sw_script =
      content::JsReplace(kWsOpenInSwScript, ws_url_);
  for (int i = 0; i < kWebSocketsPoolLimit; ++i) {
    EXPECT_EQ("open", content::EvalJs(GetWebContents(), ws_open_in_sw_script));
  }

  // All new WebSocket instances should fail to open after a limit is hit.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ("error", content::EvalJs(GetWebContents(), ws_open_in_sw_script));
  }

  // Close 5 sockets.
  for (int i = 0; i < 5; ++i) {
    EXPECT_TRUE(content::ExecJs(GetWebContents(),
                                content::JsReplace(kWsCloseInSwScript, i)));
  }

  // Expect 5 new sockets can be opened.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ("open", content::EvalJs(GetWebContents(), ws_open_in_sw_script));
  }

  // All new WebSocket instances should fail to open after a limit is hit.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ("error", content::EvalJs(GetWebContents(), ws_open_in_sw_script));
  }

  // Expect no WebSockets can be created on a webpage when a limit is hit.
  EXPECT_EQ("error",
            content::EvalJs(GetWebContents(),
                            content::JsReplace(kWsOpenScript, ws_url_)));
}

IN_PROC_BROWSER_TEST_F(WebSocketsPoolLimitBrowserTest,
                       PoolIsNotLimitedWithDisabledShields) {
  const GURL url(https_server_.GetURL("a.com", "/ephemeral_storage.html"));
  // Disable shields.
  brave_shields::SetBraveShieldsEnabled(content_settings(), false, url);

  auto* a_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  const std::string& ws_open_script =
      content::JsReplace(kWsOpenScript, ws_url_);
  // No limits should be active.
  for (int i = 0; i < kWebSocketsPoolLimit + 5; ++i) {
    EXPECT_EQ("open", content::EvalJs(a_com_rfh, ws_open_script));
  }

  // No limits should be active in 3p frame.
  auto* b_com_in_a_com_rfh = GetNthChildFrameWithHost(a_com_rfh, "b.com");
  for (int i = 0; i < kWebSocketsPoolLimit + 5; ++i) {
    EXPECT_EQ("open", content::EvalJs(b_com_in_a_com_rfh, ws_open_script));
  }

  // No limits should be active in a ServiceWorker.
  const std::string& register_sw_script = content::JsReplace(
      kRegisterSwScript, "service-worker-websockets-limit.js");
  ASSERT_TRUE(content::ExecJs(a_com_rfh, register_sw_script));
  const std::string& ws_open_in_sw_script =
      content::JsReplace(kWsOpenInSwScript, ws_url_);
  for (int i = 0; i < kWebSocketsPoolLimit + 5; ++i) {
    EXPECT_EQ("open", content::EvalJs(a_com_rfh, ws_open_in_sw_script));
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

  const std::string& ws_open_script =
      content::JsReplace(kWsOpenScript, ws_url_);
  // No limits should be active.
  for (int i = 0; i < kWebSocketsPoolLimit + 5; ++i) {
    EXPECT_EQ("open", content::EvalJs(GetWebContents(), ws_open_script));
  }
}
