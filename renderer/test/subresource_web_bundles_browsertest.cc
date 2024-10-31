/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/web_package/web_bundle_builder.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/url_loader_interceptor.h"
#include "url/gurl.h"

namespace {

constexpr char kTestHostname[] = "https://localhost";

constexpr char kPage[] = "page.html";
constexpr char kPageHeaders[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";

constexpr char kWebBundle[] = "web_bundle.wbn";

constexpr char kPassJs[] = "pass.js";
constexpr char kPassJsHeaders[] = "HTTP/1.1 404 Not Found\n\n";

constexpr char kPageHtml[] = R"(
<html>
  <head>
    <title>Loaded</title>
  </head>
  <body>
    <script type="webbundle">
      {
        "source": "https://localhost/web_bundle.wbn",
        "resources": ["https://localhost/pass.js"]
      }
    </script>
  </body>
</html>
)";

constexpr char kLoadPassJs[] = R"(
  new Promise(function (resolve, reject) {
    var s = document.createElement('script');
    s.onload = () => { resolve(true); };
    s.onerror = () => { resolve(false); };
    s.src = 'pass.js';
    document.head.appendChild(s);
  })
)";

std::string GetHeadersForURL(const std::string& url) {
  if (url.ends_with(kPage)) {
    return kPageHeaders;
  } else if (url.ends_with(kPassJs)) {
    return kPassJsHeaders;
  } else {
    EXPECT_FALSE(url.ends_with(kWebBundle))
        << "Received request for web bundle headers, which should not have "
           "happened. URL:"
        << url;
    return std::string();
  }
}

std::string GetContentForURL(const std::string& url) {
  if (url.ends_with(kPage)) {
    return kPageHtml;
  } else {
    EXPECT_FALSE(url.ends_with(kWebBundle))
        << "Received request for web bundle content, which should not have "
           "happened. URL:"
        << url;
    return std::string();
  }
}

bool URLLoaderInterceptorCallback(
    content::URLLoaderInterceptor::RequestParams* params) {
  content::URLLoaderInterceptor::WriteResponse(
      GetHeadersForURL(params->url_request.url.path()),
      GetContentForURL(params->url_request.url.path()), params->client.get());
  return true;
}

}  // namespace

class SubresourceWebBundlesBrowserTest : public InProcessBrowserTest {
 public:
  SubresourceWebBundlesBrowserTest() = default;
  ~SubresourceWebBundlesBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    url_loader_interceptor_ = std::make_unique<content::URLLoaderInterceptor>(
        base::BindRepeating(&URLLoaderInterceptorCallback));
  }

  void TearDownOnMainThread() override {
    url_loader_interceptor_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* primary_main_frame() {
    return web_contents()->GetPrimaryMainFrame();
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<content::URLLoaderInterceptor> url_loader_interceptor_;
};

IN_PROC_BROWSER_TEST_F(SubresourceWebBundlesBrowserTest,
                       SubresourceWebBundles) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL(base::JoinString({kTestHostname, kPage}, "/"))));

  EXPECT_EQ(false, content::EvalJs(primary_main_frame(), kLoadPassJs));
}
