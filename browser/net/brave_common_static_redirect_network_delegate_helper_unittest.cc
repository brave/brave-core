/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/net/features.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/constants/network_constants.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_constants.h"

using brave::ResponseCallback;

namespace {

// Pointer strategy types for parameterized testing
struct SharedPtrStrategy {
  template <typename T>
  using Ptr = std::shared_ptr<T>;
};

struct WeakPtrStrategy {
  template <typename T>
  using Ptr = base::WeakPtr<T>;
};

}  // namespace

template <typename PtrStrategy>
class BraveCommonStaticRedirectNetworkDelegateHelperTest
    : public testing::Test {
 public:
  void SetUp() override {
    // Enable feature flag if using WeakPtrStrategy, disable if
    // SharedPtrStrategy
    bool enable_flag = std::is_same_v<
        typename PtrStrategy::template Ptr<brave::BraveRequestInfo>,
        base::WeakPtr<brave::BraveRequestInfo>>;
    scoped_feature_list_.InitWithFeatureState(
        features::kBraveRequestInfoUniquePtr, enable_flag);
  }

  typename PtrStrategy::template Ptr<brave::BraveRequestInfo> MakeRequest(
      const GURL& url) {
    if constexpr (std::is_same_v<typename PtrStrategy::template Ptr<
                                     brave::BraveRequestInfo>,
                                 std::shared_ptr<brave::BraveRequestInfo>>) {
      return std::make_shared<brave::BraveRequestInfo>(url);
    } else {
      owned_request_ = std::make_unique<brave::BraveRequestInfo>(url);
      return owned_request_->AsWeakPtr();
    }
  }

 private:
  std::unique_ptr<brave::BraveRequestInfo> owned_request_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

using PtrStrategies = testing::Types<SharedPtrStrategy, WeakPtrStrategy>;
TYPED_TEST_SUITE(BraveCommonStaticRedirectNetworkDelegateHelperTest,
                 PtrStrategies);

TYPED_TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
           RedirectChromecastDownload) {
  const GURL url(
      "http://redirector.gvt1.com/edgedl/chromewebstore/"
      "random_hash/random_version_pkedcjkdefgpdelpbcmbmeomcjbeemfm.crx");
  auto request_info = this->MakeRequest(url);

  int rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                       request_info);
  const GURL redirect = GURL(request_info->new_url_spec());
  EXPECT_EQ(redirect.host(), kBraveRedirectorProxy);
  EXPECT_TRUE(redirect.SchemeIs(url::kHttpsScheme));
  EXPECT_EQ(redirect.path(), url.path());
  EXPECT_EQ(rc, net::OK);
}

TYPED_TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
           RedirectGoogleClients4) {
  const GURL url("https://clients4.google.com/chrome-sync/dev");
  auto request_info = this->MakeRequest(url);

  int rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                       request_info);
  const GURL redirect = GURL(request_info->new_url_spec());
  EXPECT_EQ(redirect.host(), kBraveClients4Proxy);
  EXPECT_TRUE(redirect.SchemeIs(url::kHttpsScheme));
  EXPECT_EQ(redirect.path(), url.path());
  EXPECT_EQ(rc, net::OK);
}

TYPED_TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
           RedirectBugsChromium) {
  // Check when we will redirect.
  const GURL url(
      "https://bugs.chromium.org/p/chromium/issues/"
      "entry?template=Crash%20Report&comment=IMPORTANT%20Chrome&labels="
      "Restrict-View-"
      "EditIssue%2CStability-Crash%2CUser-Submitted");
  auto request_info = this->MakeRequest(url);

  int rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                       request_info);
  const GURL redirect = GURL(request_info->new_url_spec());
  EXPECT_EQ(redirect.host(), "github.com");
  EXPECT_TRUE(redirect.SchemeIs(url::kHttpsScheme));
  EXPECT_EQ(redirect.path(), "/brave/brave-browser/issues/new");
  EXPECT_EQ(redirect.query(),
            "title=Crash%20Report&labels=crash&body=IMPORTANT%20Brave");
  EXPECT_EQ(rc, net::OK);

  // Check when we should not redirect: wrong query keys count
  const GURL url_fewer_keys(
      "https://bugs.chromium.org/p/chromium/issues/entry?template=A");
  request_info = this->MakeRequest(url_fewer_keys);
  rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                   request_info);
  EXPECT_TRUE(request_info->new_url_spec().empty());
  EXPECT_EQ(rc, net::OK);

  // Check when we should not redirect: wrong query keys
  const GURL url_wrong_keys(
      "https://bugs.chromium.org/p/chromium/issues/entry?t=A&l=B&c=C");
  request_info = this->MakeRequest(url_wrong_keys);
  rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                   request_info);
  EXPECT_TRUE(request_info->new_url_spec().empty());
  EXPECT_EQ(rc, net::OK);
}
