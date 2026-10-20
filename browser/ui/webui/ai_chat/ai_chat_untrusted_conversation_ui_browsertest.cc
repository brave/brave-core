// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

constexpr char kAddIframeScript[] = R"(
  new Promise(resolve => {
    const iframe = document.createElement('iframe');
    iframe.onload = () => resolve(true);
    iframe.src = $1;
    document.body.appendChild(iframe);
  })
)";

constexpr char kLoadImageScript[] = R"(
  new Promise(resolve => {
    const img = new Image();
    img.onload = () => resolve(true);
    img.onerror = () => resolve(false);
    img.src = $1;
  })
)";

}  // namespace

// The conversation frame's images come from other chrome-untrusted:// origins,
// which only serve it if they allow its origin.
class AIChatUntrustedConversationUIBrowserTest : public PlatformBrowserTest {
 public:
  AIChatUntrustedConversationUIBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}
  ~AIChatUntrustedConversationUIBrowserTest() override = default;

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    https_server_.ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());

    content::WebContents* web_contents =
        chrome_test_utils::GetActiveWebContents(this);
    ASSERT_TRUE(content::NavigateToURL(web_contents, GURL(kAIChatUIURL)));

    const GURL frame_url =
        GURL(kAIChatUntrustedConversationUIURL).Resolve("image-test");
    ASSERT_EQ(true,
              content::EvalJs(web_contents,
                              content::JsReplace(kAddIframeScript, frame_url)));
    content::RenderFrameHost* conversation_frame =
        content::FrameMatchingPredicate(
            web_contents->GetPrimaryPage(),
            base::BindRepeating(&content::FrameHasSourceUrl, frame_url));
    ASSERT_TRUE(conversation_frame);
    conversation_frame_id_ = conversation_frame->GetGlobalId();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::EvalJsResult LoadImage(const std::string& src) {
    return content::EvalJs(
        content::RenderFrameHost::FromID(conversation_frame_id_),
        content::JsReplace(kLoadImageScript, src));
  }

 protected:
  net::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  // By id, since the frame is destroyed before the fixture at shutdown.
  content::GlobalRenderFrameHostId conversation_frame_id_;
};

IN_PROC_BROWSER_TEST_F(AIChatUntrustedConversationUIBrowserTest,
                       LoadsUntrustedFavicon) {
  EXPECT_EQ(true, LoadImage("chrome-untrusted://favicon2?size=64&pageUrl="
                            "https%3A%2F%2Fexample.com"));
}

IN_PROC_BROWSER_TEST_F(AIChatUntrustedConversationUIBrowserTest,
                       LoadsUntrustedSanitizedImage) {
  const GURL image_url = https_server_.GetURL("a.com", "/logo.png");
  EXPECT_EQ(true, LoadImage(base::StrCat(
                      {"chrome-untrusted://image?url=",
                       base::EscapeQueryParamValue(image_url.spec(),
                                                   /*use_plus=*/false)})));
}
