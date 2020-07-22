/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"

#include <memory>
#include <string>

#include "base/command_line.h"
#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "brave/components/brave_component_updater/browser/switches.h"
#include "components/component_updater/component_updater_url_constants.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_constants.h"

using brave::ResponseCallback;

namespace {
const char kComponentUpdaterProxy[] = "https://componentupdater.brave.com";
}

TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
     ModifyComponentUpdaterURL) {
  brave::SetUpdateURLHostForTesting(true);
  const std::string query_string("?foo=bar");
  const GURL url(component_updater::kUpdaterJSONDefaultUrl + query_string);
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  const GURL expected_url(
      std::string(brave::kUpdaterTestingEndpoint + query_string));

  int rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                       request_info);
  EXPECT_EQ(GURL(request_info->new_url_spec), expected_url);
  EXPECT_EQ(rc, net::OK);
}

TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
     ModifyComponentUpdaterURLDev) {
  brave::SetUpdateURLHostForTesting(true);
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      brave_component_updater::kUseGoUpdateDev);
  const std::string query_string("?foo=bar");
  const GURL url(component_updater::kUpdaterJSONDefaultUrl + query_string);
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  const GURL expected_url(
      std::string(brave::kUpdaterTestingEndpoint + query_string));

  int rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                       request_info);
  EXPECT_EQ(GURL(request_info->new_url_spec), expected_url);
  EXPECT_EQ(rc, net::OK);
}

TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
     NoModifyComponentUpdaterURL) {
  const GURL url(kComponentUpdaterProxy);
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);

  int rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                       request_info);
  EXPECT_EQ(request_info->new_url_spec, GURL());
  EXPECT_EQ(rc, net::OK);
}

TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
     RedirectChromecastDownload) {
  const GURL url(
      "http://redirector.gvt1.com/edgedl/chromewebstore/"
      "random_hash/random_version_pkedcjkdefgpdelpbcmbmeomcjbeemfm.crx");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);

  int rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                       request_info);
  const GURL redirect = GURL(request_info->new_url_spec);
  EXPECT_EQ(redirect.host(), kBraveRedirectorProxy);
  EXPECT_TRUE(redirect.SchemeIs(url::kHttpsScheme));
  EXPECT_EQ(redirect.path(), url.path());
  EXPECT_EQ(rc, net::OK);
}

TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
     RedirectGoogleClients4) {
  const GURL url("https://clients4.google.com/chrome-sync/dev");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);

  int rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                       request_info);
  const GURL redirect = GURL(request_info->new_url_spec);
  EXPECT_EQ(redirect.host(), kBraveClients4Proxy);
  EXPECT_TRUE(redirect.SchemeIs(url::kHttpsScheme));
  EXPECT_EQ(redirect.path(), url.path());
  EXPECT_EQ(rc, net::OK);
}

TEST(BraveCommonStaticRedirectNetworkDelegateHelperTest,
     RedirectBugsChromium) {
  // Check when we will redirect.
  const GURL url(
      "https://bugs.chromium.org/p/chromium/issues/"
      "entry?template=Crash%20Report&comment=IMPORTANT%20Chrome&labels="
      "Restrict-View-"
      "EditIssue%2CStability-Crash%2CUser-Submitted");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);

  int rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                       request_info);
  const GURL redirect = GURL(request_info->new_url_spec);
  EXPECT_EQ(redirect.host(), "github.com");
  EXPECT_TRUE(redirect.SchemeIs(url::kHttpsScheme));
  EXPECT_EQ(redirect.path(), "/brave/brave-browser/issues/new");
  EXPECT_EQ(redirect.query(),
            "title=Crash%20Report&labels=crash&body=IMPORTANT%20Brave");
  EXPECT_EQ(rc, net::OK);

  // Check when we should not redirect: wrong query keys count
  request_info.reset();
  const GURL url_fewer_keys(
      "https://bugs.chromium.org/p/chromium/issues/entry?template=A");
  request_info = std::make_shared<brave::BraveRequestInfo>(url_fewer_keys);
  rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                       request_info);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(rc, net::OK);

  // Check when we should not redirect: wrong query keys
  request_info.reset();
  const GURL url_wrong_keys(
      "https://bugs.chromium.org/p/chromium/issues/entry?t=A&l=B&c=C");
  request_info = std::make_shared<brave::BraveRequestInfo>(url_wrong_keys);
  rc = OnBeforeURLRequest_CommonStaticRedirectWork(ResponseCallback(),
                                                   request_info);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(rc, net::OK);
}
