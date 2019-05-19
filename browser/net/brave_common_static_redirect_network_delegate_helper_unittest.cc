/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"

#include <memory>
#include <string>

#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/component_updater/component_updater_url_constants.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

const char kComponentUpdaterProxy[] = "https://componentupdater.brave.com";

class BraveCommonStaticRedirectNetworkDelegateHelperTest
    : public testing::Test {
 public:
  BraveCommonStaticRedirectNetworkDelegateHelperTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {}
  ~BraveCommonStaticRedirectNetworkDelegateHelperTest() override {}
  void SetUp() override { context_->Init(); }
  net::TestURLRequestContext* context() { return context_.get(); }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
};

TEST_F(BraveCommonStaticRedirectNetworkDelegateHelperTest,
       ModifyComponentUpdaterURL) {
  net::TestDelegate test_delegate;
  std::string query_string("?foo=bar");
  GURL url(std::string(component_updater::kUpdaterJSONDefaultUrl) +
           query_string);
  std::unique_ptr<net::URLRequest> request = context()->CreateRequest(
      url, net::IDLE, &test_delegate, TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo> before_url_context(
      new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              before_url_context);
  brave::ResponseCallback callback;
  GURL expected_url(
      std::string(kBraveUpdatesExtensionsEndpoint + query_string));
  int ret =
      OnBeforeURLRequest_CommonStaticRedirectWork(callback, before_url_context);
  EXPECT_EQ(GURL(before_url_context->new_url_spec), expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveCommonStaticRedirectNetworkDelegateHelperTest,
       NoModifyComponentUpdaterURL) {
  net::TestDelegate test_delegate;
  GURL url(kComponentUpdaterProxy);
  std::unique_ptr<net::URLRequest> request = context()->CreateRequest(
      url, net::IDLE, &test_delegate, TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo> before_url_context(
      new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              before_url_context);
  brave::ResponseCallback callback;
  GURL expected_url;
  int ret =
      OnBeforeURLRequest_CommonStaticRedirectWork(callback, before_url_context);
  EXPECT_EQ(before_url_context->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

}  // namespace
