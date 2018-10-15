/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_httpse_network_delegate_helper.h"

#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"

namespace {

class BraveHTTPSENetworkDelegateHelperTest: public testing::Test {
 public:
  BraveHTTPSENetworkDelegateHelperTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {
  }
  ~BraveHTTPSENetworkDelegateHelperTest() override {}
  void SetUp() override {
    context_->Init();
  }
  net::TestURLRequestContext* context() { return context_.get(); }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
};


TEST_F(BraveHTTPSENetworkDelegateHelperTest, AlreadySetNewURLNoOp) {
  net::TestDelegate test_delegate;
  GURL url("http://bradhatesprimes.brave.com/composite_numbers_ftw");
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  request->set_site_for_cookies(
      GURL("http://brad.brave.com/hide_all_primes_in_ui/composites_forever"));
  brave_request_info->new_url_spec = "data:image/png;base64,iVB";
  brave::ResponseCallback callback;
  GURL new_url;
  int ret =
    OnBeforeURLRequest_HttpsePreFileWork(&new_url, callback,
        brave_request_info);
  EXPECT_TRUE(new_url.is_empty());
  EXPECT_EQ(ret, net::OK);
}

}  // namespace
