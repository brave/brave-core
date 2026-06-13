/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <string_view>

#include "base/strings/string_util.h"
#include "extensions/buildflags/buildflags.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "extensions/common/extension.h"
#include "extensions/test/test_extension_dir.h"
#include "url/gurl.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

namespace {

// Handles GET /redirect-to-extension?ext=<extension-id> and replies with a
// server-side 302 redirect to chrome-extension://<extension-id>/callback.html.
// The extension ID is embedded in the query string so this handler is
// stateless and safe to invoke from the IO thread.
std::unique_ptr<net::test_server::HttpResponse> HandleExtensionRedirectRequest(
    const net::test_server::HttpRequest& request) {
  static constexpr std::string_view kPrefix = "/redirect-to-extension?ext=";
  if (!base::StartsWith(request.relative_url, kPrefix)) {
    return nullptr;
  }
  const std::string ext_id = request.relative_url.substr(kPrefix.size());
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_FOUND);
  response->AddCustomHeader(
      "Location", "chrome-extension://" + ext_id + "/callback.html?code=test");
  return response;
}

}  // namespace

#if BUILDFLAG(ENABLE_EXTENSIONS)

// Regression tests for https://github.com/brave/brave-browser/issues/56271
//
// A server-side (302) redirect from an HTTP page to a chrome-extension://
// web_accessible_resource should navigate successfully.
class BraveProxyingURLLoaderFactoryBrowserTest : public InProcessBrowserTest {
 public:
  BraveProxyingURLLoaderFactoryBrowserTest() = default;
  ~BraveProxyingURLLoaderFactoryBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    // Handlers must be registered before Start().
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&HandleExtensionRedirectRequest));
    ASSERT_TRUE(embedded_test_server()->Start());

    LoadTestExtension();
  }

 protected:
  const std::string& extension_id() const { return extension_id_; }

 private:
  void LoadTestExtension() {
    // MV3 extension with a single web_accessible_resource (callback.html)
    // accessible from any origin — the minimal repro from the bug report.
    test_extension_dir_.WriteManifest(R"json({
      "name": "Test Redirect Callback Extension",
      "manifest_version": 3,
      "version": "0.1",
      "web_accessible_resources": [{
        "resources": ["callback.html"],
        "matches": ["<all_urls>"]
      }]
    })json");
    test_extension_dir_.WriteFile(
        FILE_PATH_LITERAL("callback.html"),
        "<html><body>Callback page loaded</body></html>");

    extensions::ChromeTestExtensionLoader loader(browser()->profile());
    scoped_refptr<const extensions::Extension> extension =
        loader.LoadExtension(test_extension_dir_.UnpackedPath());
    ASSERT_TRUE(extension) << "Failed to load test extension";
    extension_id_ = extension->id();
  }

  // Must outlive the loaded extension.
  extensions::TestExtensionDir test_extension_dir_;
  std::string extension_id_;
};

// Verifies that a server-side 302 redirect from HTTP to a
// chrome-extension:// web_accessible_resource completes and the extension
// page commits in the active tab.
//
// Regression test for https://github.com/brave/brave-browser/issues/56271.
IN_PROC_BROWSER_TEST_F(BraveProxyingURLLoaderFactoryBrowserTest,
                       ServerRedirectToExtensionWebAccessibleResourceLoads) {
  // Embed the extension ID in the query string so the server-side handler can
  // build the Location header without accessing UI-thread state.
  const GURL redirect_url = embedded_test_server()->GetURL(
      "/redirect-to-extension?ext=" + extension_id());
  const GURL expected_extension_url =
      GURL("chrome-extension://" + extension_id() + "/callback.html?code=test");

  // NavigateToURL follows the 302 and waits for the final page to load.
  // If the regression is present this call hangs indefinitely because
  // the chrome-extension:// URL loader is never started.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), redirect_url));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(expected_extension_url, web_contents->GetLastCommittedURL());
  EXPECT_FALSE(web_contents->IsLoading());
}

// Same scenario navigated in a new foreground tab to rule out any
// single-tab-specific quirks in the loader lifecycle.
//
// Regression test for https://github.com/brave/brave-browser/issues/56271.
IN_PROC_BROWSER_TEST_F(BraveProxyingURLLoaderFactoryBrowserTest,
                       ServerRedirectToExtensionInNewTabLoads) {
  const GURL redirect_url = embedded_test_server()->GetURL(
      "/redirect-to-extension?ext=" + extension_id());
  const GURL expected_extension_url =
      GURL("chrome-extension://" + extension_id() + "/callback.html?code=test");

  content::RenderFrameHost* rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), redirect_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(rfh);
  content::WebContents* new_tab =
      content::WebContents::FromRenderFrameHost(rfh);
  ASSERT_TRUE(new_tab);
  EXPECT_EQ(expected_extension_url, new_tab->GetLastCommittedURL());
  EXPECT_FALSE(new_tab->IsLoading());
}

#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
