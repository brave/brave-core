/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"

#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/component_updater/component_updater_url_constants.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"


namespace {

class BraveStaticRedirectNetworkDelegateHelperTest: public testing::Test {
 public:
  BraveStaticRedirectNetworkDelegateHelperTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {
  }
  ~BraveStaticRedirectNetworkDelegateHelperTest() override {}
  void SetUp() override {
    context_->Init();
  }
  net::TestURLRequestContext* context() { return context_.get(); }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
};

TEST_F(BraveStaticRedirectNetworkDelegateHelperTest, NoModifyTypicalURL) {
  net::TestDelegate test_delegate;
  GURL url("https://bradhatesprimes.brave.com/composite_numbers_ftw");
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                             TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo>
      before_url_context(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(), before_url_context);
  brave::ResponseCallback callback;
  int ret =
    OnBeforeURLRequest_StaticRedirectWork(callback,
        before_url_context);
  EXPECT_TRUE(before_url_context->new_url_spec.empty());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveStaticRedirectNetworkDelegateHelperTest, ModifyGeoURL) {
  net::TestDelegate test_delegate;
  GURL url("https://www.googleapis.com/geolocation/v1/geolocate?key=2_3_5_7");
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                             TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo>
      before_url_context(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(), before_url_context);
  brave::ResponseCallback callback;
  GURL expected_url(GOOGLEAPIS_ENDPOINT GOOGLEAPIS_API_KEY);
  int ret =
      OnBeforeURLRequest_StaticRedirectWork(callback,
                                            before_url_context);
  EXPECT_EQ(before_url_context->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveStaticRedirectNetworkDelegateHelperTest, ModifySafeBrowsingURLV4) {
  net::TestDelegate test_delegate;
  GURL url("https://safebrowsing.googleapis.com/v4/threatListUpdates:fetch?$req=ChkKCGNocm9taXVtEg02Ni");
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                             TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo>
      before_url_context(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(), before_url_context);
  brave::ResponseCallback callback;
  GURL::Replacements replacements;
  replacements.SetHostStr(SAFEBROWSING_ENDPOINT);
  GURL expected_url(url.ReplaceComponents(replacements));
  int ret =
      OnBeforeURLRequest_StaticRedirectWork(callback,
                                            before_url_context);
  EXPECT_EQ(before_url_context->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveStaticRedirectNetworkDelegateHelperTest, ModifySafeBrowsingURLV5) {
  net::TestDelegate test_delegate;
  GURL url("https://safebrowsing.googleapis.com/v5/threatListUpdates:fetch?$req=ChkKCGNocm9taXVtEg02Ni");
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                             TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo>
      before_url_context(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(), before_url_context);
  brave::ResponseCallback callback;
  GURL::Replacements replacements;
  replacements.SetHostStr(SAFEBROWSING_ENDPOINT);
  GURL expected_url(url.ReplaceComponents(replacements));
  int ret =
      OnBeforeURLRequest_StaticRedirectWork(callback,
                                            before_url_context);
  EXPECT_EQ(before_url_context->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

}  // namespace
