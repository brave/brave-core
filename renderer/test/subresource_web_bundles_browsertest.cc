/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/embedder_support/switches.h"
#include "components/web_package/test_support/web_bundle_builder.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/url_loader_interceptor.h"
#include "third_party/blink/public/common/features.h"
#include "url/gurl.h"

namespace {

constexpr char kOriginTrialTestPublicKey[] =
    "dRCs+TocuKkocNKa0AtZ4awrt9XKH2SQCI6o4FY6BNA=";

constexpr char kOriginTrialTestHostname[] = "https://localhost";

constexpr char kOriginTrialPage[] = "page.html";
constexpr char kOriginTrialPageHeaders[] =
    "HTTP/1.1 200 OK\nContent-type: text/html\n\n";

constexpr char kWebBundle[] = "web_bundle.wbn";
constexpr char kWebBundleHeaders[] =
    "HTTP/1.1 200 OK\nContent-type: application/webbundle\n\n";

constexpr char kPassJs[] = "pass.js";
constexpr char kPassJsHeaders[] = "HTTP/1.1 404 Not Found\n\n";

// tools/origin_trials/generate_token.py \
//    --expire-days 3650 https://localhost SubresourceWebBundles
constexpr char kOriginTrialToken[] =
    "A0bbldJxinw6xRKnkDrBLVob3U638q6NVqmE5nax5Bdu+hZVIgy1sXCM9ccc+5wvAZb+V48iSV"
    "vGX8H6s+cbGgsAAABdeyJvcmlnaW4iOiAiaHR0cHM6Ly9sb2NhbGhvc3Q6NDQzIiwgImZlYXR1"
    "cmUiOiAiU3VicmVzb3VyY2VXZWJCdW5kbGVzIiwgImV4cGlyeSI6IDE5MjEwNzcxMjB9";

constexpr char kPageHtml[] = R"(
<html>
  <head>
    <title>Loaded</title>
    META_TAG
  </head>
  <body>
    <script>
      (() => {
        const wbn_url =
            new URL('./web_bundle.wbn', location.href).toString();
        const pass_js_url = new URL('./pass.js', location.href).toString();
        const link = document.createElement('link');
        link.rel = 'webbundle';
        link.href = wbn_url;
        link.resources = pass_js_url;
        document.body.appendChild(link);
      })();
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

std::string CreateWebBundle() {
  std::string pass_js_url_str =
      GURL(base::JoinString({kOriginTrialTestHostname, kPassJs}, "/")).spec();
  // Currently the web bundle format requires a valid GURL for the fallback
  // URL of a web bundle.
  std::string flbk_js_url_str =
      GURL(base::JoinString({kOriginTrialTestHostname, "fallback.js"}, "/"))
          .spec();
  web_package::test::WebBundleBuilder builder(flbk_js_url_str, "");
  auto pass_js_location = builder.AddResponse(
      {{":status", "200"}, {"content-type", "application/javascript"}},
      "document.title = 'script loaded';");
  builder.AddIndexEntry(pass_js_url_str, "", {pass_js_location});
  std::vector<uint8_t> bundle = builder.CreateBundle();
  return std::string(bundle.begin(), bundle.end());
}

std::string GetHeadersForURL(const std::string& url) {
  if (base::EndsWith(url, kOriginTrialPage, base::CompareCase::SENSITIVE)) {
    return kOriginTrialPageHeaders;
  } else if (base::EndsWith(url, kWebBundle, base::CompareCase::SENSITIVE)) {
    return kWebBundleHeaders;
  } else if (base::EndsWith(url, kPassJs, base::CompareCase::SENSITIVE)) {
    return kPassJsHeaders;
  } else {
    return std::string();
  }
}

std::string GetContentForURL(const std::string& url) {
  if (base::EndsWith(url, kOriginTrialPage, base::CompareCase::SENSITIVE)) {
    std::string response = kPageHtml;
    std::string meta_tag =
        base::StrCat({R"(<meta http-equiv="origin-trial" content=")",
                      kOriginTrialToken, R"(">)"});
    base::ReplaceFirstSubstringAfterOffset(&response, 0, "META_TAG", meta_tag);
    return response;
  } else if (base::EndsWith(url, kWebBundle, base::CompareCase::SENSITIVE)) {
    return CreateWebBundle();
  } else {
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

class SubresourceWebBundlesBrowserTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<bool> {
 public:
  SubresourceWebBundlesBrowserTest() = default;
  ~SubresourceWebBundlesBrowserTest() override = default;

  bool IsSubresourceWebBundlesEnabled() { return GetParam(); }

  void SetUp() override {
    if (IsSubresourceWebBundlesEnabled()) {
      scoped_feature_list_.InitAndEnableFeature(
          features::kSubresourceWebBundles);
    }
    InProcessBrowserTest::SetUp();
  }

  void SetUpDefaultCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpDefaultCommandLine(command_line);
    if (!IsSubresourceWebBundlesEnabled()) {
      // With feature initially disabled, use origin trial.
      command_line->AppendSwitchASCII(embedder_support::kOriginTrialPublicKey,
                                      kOriginTrialTestPublicKey);
    }
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // We use a URLLoaderInterceptor, rather than the EmbeddedTestServer, since
    // the origin trial token in the response is associated with a fixed
    // origin, whereas EmbeddedTestServer serves content on a random port.
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

  content::RenderFrameHost* main_frame() {
    return web_contents()->GetMainFrame();
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<content::URLLoaderInterceptor> url_loader_interceptor_;
};

IN_PROC_BROWSER_TEST_P(SubresourceWebBundlesBrowserTest,
                       DISABLED_SubresourceWebBundles) {
  EXPECT_EQ(IsSubresourceWebBundlesEnabled(),
            base::FeatureList::IsEnabled(features::kSubresourceWebBundles));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL(base::JoinString(
                     {kOriginTrialTestHostname, kOriginTrialPage}, "/"))));

  if (IsSubresourceWebBundlesEnabled()) {
    std::u16string expected_title(u"script loaded");
    content::TitleWatcher title_watcher(web_contents(), expected_title);
    EXPECT_EQ(true, content::EvalJs(main_frame(), kLoadPassJs));
    EXPECT_EQ(expected_title, title_watcher.WaitAndGetTitle());
  } else {
    EXPECT_EQ(false, content::EvalJs(main_frame(), kLoadPassJs));
  }
}

INSTANTIATE_TEST_SUITE_P(SubresourceWebBundlesBrowserTest,
                         SubresourceWebBundlesBrowserTest,
                         ::testing::Bool());
