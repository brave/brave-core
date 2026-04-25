/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <utility>
#include <vector>

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/webui_config_map.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/scoped_web_ui_controller_factory_registration.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/webui/untrusted_web_ui_browsertest_util.h"
#include "url/gurl.h"

namespace {

constexpr char kAddIframeScript[] =
    "var frame = document.createElement('iframe');\n"
    "frame.src = $1;\n"
    "document.body.appendChild(frame);\n";
}

class DocumentLocationBrowserTest : public InProcessBrowserTest {
 protected:
  DocumentLocationBrowserTest() {
    test_server_.ServeFilesFromSourceDirectory(GetChromeTestDataDir());
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(test_server_.Start());
  }

 protected:
  net::EmbeddedTestServer test_server_;
  content::TestWebUIControllerFactory factory_;
  content::ScopedWebUIControllerFactoryRegistration factory_registration_{
      &factory_};
};

IN_PROC_BROWSER_TEST_F(DocumentLocationBrowserTest,
                       ChromeUntrustedIsRemovedFromAncestorOrigins) {
  // Serve a WebUI with no iframe restrictions.
  GURL main_frame_url(content::GetWebUIURL(
      "web-ui/"
      "title1.html?childsrc=&requestableschemes=chrome-untrusted"));
  auto* main_frame_rfh =
      ui_test_utils::NavigateToURL(browser(), main_frame_url);
  ASSERT_TRUE(main_frame_rfh);
  auto* wc = content::WebContents::FromRenderFrameHost(main_frame_rfh);

  // Add a DataSource for the chrome-untrusted:// iframe.
  content::TestUntrustedDataSourceHeaders headers;
  std::vector<std::string> frame_ancestors({"chrome://web-ui"});
  headers.frame_ancestors =
      std::make_optional<std::vector<std::string>>(std::move(frame_ancestors));
  headers.child_src = "child-src *;";
  content::WebUIConfigMap::GetInstance().AddUntrustedWebUIConfig(
      std::make_unique<ui::TestUntrustedWebUIConfig>("test-host", headers));

  {
    // Add the iframe to the chrome://web-ui WebUI and verify it was
    // successfully embedded.
    content::TestNavigationObserver observer(wc);
    const GURL chrome_untrusted_url(
        content::GetChromeUntrustedUIURL("test-host/title1.html"));
    EXPECT_TRUE(
        ExecJs(wc, content::JsReplace(kAddIframeScript, chrome_untrusted_url)));
    observer.Wait();
    EXPECT_TRUE(observer.last_navigation_succeeded());
  }

  auto* chrome_untrusted_rfh = content::ChildFrameAt(main_frame_rfh, 0);
  ASSERT_TRUE(chrome_untrusted_rfh);
  {
    // Add the iframe to the chrome-untrusted://test-host and verify it was
    // successfully embedded.
    content::TestNavigationObserver observer(wc);
    const GURL a_com_url(test_server_.GetURL("a.com", "/simple.html"));
    EXPECT_TRUE(ExecJs(chrome_untrusted_rfh,
                       content::JsReplace(kAddIframeScript, a_com_url)));
    observer.Wait();
    EXPECT_TRUE(observer.last_navigation_succeeded());
  }

  auto* a_com_rfh = content::ChildFrameAt(chrome_untrusted_rfh, 0);
  ASSERT_TRUE(a_com_rfh);
  // Ensure ancestorOrigins doesn't include chrome-untrusted:// and other
  // parents.
  EXPECT_EQ(0, EvalJs(a_com_rfh, "location.ancestorOrigins.length"));
}
