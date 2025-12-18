// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "services/network/public/cpp/network_switches.h"

namespace brave_wallet {

class TrezorUIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.StartAcceptingConnections();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void SetUp() override {
    ASSERT_TRUE(https_server_.InitializeAndListen());
    InProcessBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP * " + https_server_.host_port_pair().ToString());
  }

 protected:
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

IN_PROC_BROWSER_TEST_F(TrezorUIBrowserTest, CheckOpenerInPopup) {
  auto* trezor_bridge = ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(kUntrustedTrezorURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  {
    // Trezor connect tries to open popup this way. Make sure our patch works
    // and does this in a bit different way so opened window has `opener` set.
    content::WebContentsAddedObserver trezor_popup_observer;
    EXPECT_TRUE(content::ExecJs(
        trezor_bridge,
        "window.open('https://connect.trezor.io/empty.html', 'modal')"));

    content::WebContents* trezor_popup = trezor_popup_observer.GetWebContents();
    EXPECT_TRUE(WaitForLoadStop(trezor_popup));

    // Ensure there is non-null `opener` in opened window.
    EXPECT_TRUE(content::EvalJs(trezor_popup, "!!window.opener").ExtractBool());
    trezor_popup->Close();
  }

  {
    // Try to open non-"connect.trezor.io" origin window. It will not have
    // opener set.
    content::WebContentsAddedObserver example_popup_observer;
    EXPECT_TRUE(content::ExecJs(
        trezor_bridge,
        "window.open('https://example.com/empty.html', 'modal')"));
    content::WebContents* example_popup =
        example_popup_observer.GetWebContents();
    EXPECT_TRUE(WaitForLoadStop(example_popup));
    EXPECT_FALSE(
        content::EvalJs(example_popup, "!!window.opener").ExtractBool());
    example_popup->Close();
  }
}

}  // namespace brave_wallet
