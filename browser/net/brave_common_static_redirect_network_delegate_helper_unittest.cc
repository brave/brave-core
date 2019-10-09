/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"

#include <memory>
#include <string>

#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "components/component_updater/component_updater_url_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

const char kComponentUpdaterProxy[] = "https://componentupdater.brave.com";

TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
     ModifyComponentUpdaterURL) {
  std::string query_string("?foo=bar");
  GURL url(std::string(component_updater::kUpdaterJSONDefaultUrl) +
           query_string);
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(
      std::string(kBraveUpdatesExtensionsEndpoint + query_string));
  int ret =
      OnBeforeURLRequest_CommonStaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(GURL(brave_request_info->new_url_spec), expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
     NoModifyComponentUpdaterURL) {
  GURL url(kComponentUpdaterProxy);
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url;
  int ret =
      OnBeforeURLRequest_CommonStaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
     RedirectChromecastDownload) {
  GURL url(
      "http://redirector.gvt1.com/edgedl/chromewebstore/"
      "random_hash/random_version_pkedcjkdefgpdelpbcmbmeomcjbeemfm.crx");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;

  int ret = OnBeforeURLRequest_CommonStaticRedirectWork(callback,
      brave_request_info);
  GURL redirect = GURL(brave_request_info->new_url_spec);
  EXPECT_EQ(redirect.host(), kBraveRedirectorProxy);
  EXPECT_TRUE(redirect.SchemeIs(url::kHttpsScheme));
  EXPECT_EQ(redirect.path(), url.path());
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
     RedirectGoogleClients4) {
  GURL url(
      "https://clients4.google.com/chrome-sync/dev");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;

  int ret = OnBeforeURLRequest_CommonStaticRedirectWork(callback,
      brave_request_info);
  GURL redirect = GURL(brave_request_info->new_url_spec);
  EXPECT_EQ(redirect.host(), kBraveClients4Proxy);
  EXPECT_TRUE(redirect.SchemeIs(url::kHttpsScheme));
  EXPECT_EQ(redirect.path(), url.path());
  EXPECT_EQ(ret, net::OK);
}

}  // namespace
