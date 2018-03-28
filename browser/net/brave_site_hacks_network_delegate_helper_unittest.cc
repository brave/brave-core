/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"

#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"

namespace {

class BraveSiteHacksNetworkDelegateHelperTest: public testing::Test {
 public:
  BraveSiteHacksNetworkDelegateHelperTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {
  }
  ~BraveSiteHacksNetworkDelegateHelperTest() override {}
  void SetUp() override {}
  net::TestURLRequestContext* context() { return context_.get(); }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
};


TEST_F(BraveSiteHacksNetworkDelegateHelperTest, NoChangeURL) {
  net::TestDelegate test_delegate;
  GURL url("https://bradhatesprimes.brave.com/composite_numbers_ftw");
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::OnBeforeURLRequestContext>
      before_url_context(new brave::OnBeforeURLRequestContext());
  brave::ResponseCallback callback;
  GURL new_url;
  int ret =
    OnBeforeURLRequest_SiteHacksWork(request.get(), &new_url, callback,
        before_url_context);
  EXPECT_TRUE(new_url.is_empty());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, RedirectsToEmptyDataURLs) {
  std::vector<GURL> urls({
    GURL("https://sp1.nypost.com"),
    GURL("https://sp.nasdaq.com")
  });
  std::for_each(urls.begin(), urls.end(),
      [this](GURL url){
    net::TestDelegate test_delegate;
    std::unique_ptr<net::URLRequest> request =
        context()->CreateRequest(url, net::IDLE, &test_delegate,
                                 TRAFFIC_ANNOTATION_FOR_TESTS);
    std::shared_ptr<brave::OnBeforeURLRequestContext>
        before_url_context(new brave::OnBeforeURLRequestContext());
    brave::ResponseCallback callback;
    GURL new_url;
    int ret =
      OnBeforeURLRequest_SiteHacksWork(request.get(), &new_url, callback,
          before_url_context);
    EXPECT_EQ(ret, net::OK);
    EXPECT_STREQ(new_url.spec().c_str(), kEmptyDataURI);
  });
}

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, RedirectsToStubs) {
  std::vector<GURL> urls({
    GURL(kGoogleTagManagerPattern),
    GURL(kGoogleTagServicesPattern)
  });
  std::for_each(urls.begin(), urls.end(),
      [this](GURL url){
    net::TestDelegate test_delegate;
    std::unique_ptr<net::URLRequest> request =
        context()->CreateRequest(url, net::IDLE, &test_delegate,
                                 TRAFFIC_ANNOTATION_FOR_TESTS);
    std::shared_ptr<brave::OnBeforeURLRequestContext>
        before_url_context(new brave::OnBeforeURLRequestContext());
    brave::ResponseCallback callback;
    GURL new_url;
    int ret =
      OnBeforeURLRequest_SiteHacksWork(request.get(), &new_url, callback,
          before_url_context);
    EXPECT_EQ(ret, net::OK);
    EXPECT_TRUE(new_url.SchemeIs("data"));
  });
}

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, Blocking) {
  std::vector<GURL> urls({
    GURL("https://www.lesechos.fr/xtcore.js"),
    GURL("https://bradhatesprimes.y8.com/js/sdkloader/outstream.js")
  });
  std::for_each(urls.begin(), urls.end(),
      [this](GURL url){
    net::TestDelegate test_delegate;
    std::unique_ptr<net::URLRequest> request =
        context()->CreateRequest(url, net::IDLE, &test_delegate,
                                 TRAFFIC_ANNOTATION_FOR_TESTS);
    std::shared_ptr<brave::OnBeforeURLRequestContext>
        before_url_context(new brave::OnBeforeURLRequestContext());
    brave::ResponseCallback callback;
    GURL new_url;
    int ret =
      OnBeforeURLRequest_SiteHacksWork(request.get(), &new_url, callback,
          before_url_context);
    EXPECT_EQ(ret, net::ERR_ABORTED);
  });
}



}  // namespace
