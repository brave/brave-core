/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/functional/bind.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "url/gurl.h"

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_content_type("text/html");
  response->set_content("<html><body></body></html>");
  return response;
}

}  // namespace

// navigator.credentials is [SecureContext], so these navigate to the embedded
// test server rather than a data: URL, which is an opaque origin.
class WebOtpDisabledTest : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    embedded_test_server()->RegisterDefaultHandler(
        base::BindRepeating(&HandleRequest));
    ASSERT_TRUE(embedded_test_server()->Start());
    ASSERT_TRUE(content::NavigateToURL(web_contents(),
                                       embedded_test_server()->GetURL("/")));
  }

 protected:
  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }
};

// features::kWebOTP is shipped disabled, so the renderer must not expose the
// API either.
IN_PROC_BROWSER_TEST_F(WebOtpDisabledTest, ApiIsNotExposed) {
  EXPECT_EQ(false,
            content::EvalJs(web_contents(), "'OTPCredential' in window"));
}

// blink.mojom.WebOTPService is bound only while features::kWebOTP is enabled,
// so a renderer that still exposes WebOTP asks for a binder that is not there,
// which is a bad message and terminates the renderer.
IN_PROC_BROWSER_TEST_F(WebOtpDisabledTest, RequestingOtpDoesNotKillTab) {
  // With WebOTP disabled the `otp` member is not recognized, so no credential
  // type is requested and the promise rejects without the renderer ever asking
  // for the binder.
  EXPECT_EQ("NotSupportedError", content::EvalJs(web_contents(), R"(
      navigator.credentials.get({otp: {transport: ['sms']}})
          .then(() => 'resolved', e => e.name)
  )"));
  EXPECT_FALSE(web_contents()->IsCrashed());
}
