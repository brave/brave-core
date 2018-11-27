/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/url_context.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "url/gurl.h"

namespace {

class URLContextTest: public testing::Test {
 public:
  URLContextTest() :
      thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
      context_(new net::TestURLRequestContext(true)) {
  }

  ~URLContextTest() override {}

  void SetUp() override {
    context_->Init();
  }

  net::TestURLRequestContext* context() { return context_.get(); }

 protected:

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
};

TEST_F(URLContextTest, TabHostResolvesProperlyForTabContext) {
  GURL url("https://www.brave.com/prime_numbers/127");
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                             TRAFFIC_ANNOTATION_FOR_TESTS);
  request->set_site_for_cookies(GURL("https://be.brave.com/test.html"));

  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(), brave_request_info);
  ASSERT_EQ(brave_request_info->tab_origin, "https://be.brave.com/");
}

TEST_F(URLContextTest, PDFJSTabHostResolvesProperlyForTabContext) {
  GURL url("https://www.brave.com/prime_numbers/131");
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                             TRAFFIC_ANNOTATION_FOR_TESTS);
  request->set_site_for_cookies(GURL("chrome-extension://oemmndcbldboiebfnladdacbdfmadadm/https://example.com/test.pdf"));

  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(), brave_request_info);
  ASSERT_EQ(brave_request_info->tab_origin, "https://example.com/");
}

}  // namespace

