/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"

#include <memory>
#include <string>

#include "brave/browser/net/url_context.h"
#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/common/network_constants.h"
#include "brave/common/translate_network_constants.h"
#include "components/component_updater/component_updater_url_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

TEST(BraveStaticRedirectNetworkDelegateHelperTest, NoModifyTypicalURL) {
  GURL url("https://bradhatesprimes.brave.com/composite_numbers_ftw");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest, ModifyGeoURL) {
  GURL url("https://www.googleapis.com/geolocation/v1/geolocate?key=2_3_5_7");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(GOOGLEAPIS_ENDPOINT GOOGLEAPIS_API_KEY);
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest, ModifyCRLSet1) {
  GURL url(
      "https://dl.google.com/release2/chrome_component/AJ4r388iQSJq_4819/"
      "4819_all_crl-set-5934829738003798040.data.crx3");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(
      "https://crlsets.brave.com/release2/chrome_component/"
      "AJ4r388iQSJq_4819/4819_all_crl-set-5934829738003798040.data.crx3");
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest, ModifyCRLSet2) {
  GURL url(
      "https://r2---sn-8xgp1vo-qxoe.gvt1.com/edgedl/release2/"
      "chrome_component/AJ4r388iQSJq_4819/4819_all_crl-set-5934829738003798040"
      ".data.crx3");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(
      "https://crlsets.brave.com/edgedl/release2/chrome_compone"
      "nt/AJ4r388iQSJq_4819/4819_all_crl-set-5934829738003798040.data.crx3");
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest, ModifyCRLSet3) {
  GURL url(
      "https://www.google.com/dl/release2/chrome_component/LLjIBPPmveI_4988/"
      "4988_all_crl-set-6296993568184466307.data.crx3");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(
      "https://crlsets.brave.com/dl/release2/chrome_component/LLjIBPPmveI_4988/"
      "4988_all_crl-set-6296993568184466307.data.crx3");
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest, ModifyCRXDownload) {
  GURL url(
      "https://clients2.googleusercontent.com/crx/blobs/QgAAAC6zw0qH2DJtn"
      "Xe8Z7rUJP1RM6lX7kVcwkQ56ujmG3AWYOAkxoNnIdnEBUz_"
      "3z4keVhjzzAF10srsaL7lrntfB"
      "IflcYIrTziwX3SUS9i_P-CAMZSmuV5tdQl-Roo6cnVC_GRzKsnZSKm1Q/"
      "extension_2_0_67"
      "3_0.crx");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(
      "https://crxdownload.brave.com/crx/blobs/QgAAAC6"
      "zw0qH2DJtnXe8Z7rUJP1RM6lX7kVcwkQ56ujmG3AWYOAkxoNnIdnEBUz_"
      "3z4keVhjzzAF10sr"
      "saL7lrntfBIflcYIrTziwX3SUS9i_P-CAMZSmuV5tdQl-Roo6cnVC_GRzKsnZSKm1Q/"
      "extens"
      "ion_2_0_673_0.crx");
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest, ModifyCRLSet1_http) {
  GURL url(
      "http://dl.google.com/release2/chrome_component/AJ4r388iQSJq_4819/"
      "4819_all_crl-set-5934829738003798040.data.crx3");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(
      "https://crlsets.brave.com/release2/chrome_component/"
      "AJ4r388iQSJq_4819/4819_all_crl-set-5934829738003798040.data.crx3");
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest, ModifyCRLSet2_http) {
  GURL url(
      "http://r2---sn-8xgp1vo-qxoe.gvt1.com/edgedl/release2/"
      "chrome_component/AJ4r388iQSJq_4819/4819_all_crl-set-5934829738003798040"
      ".data.crx3");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(
      "https://crlsets.brave.com/edgedl/release2/chrome_compone"
      "nt/AJ4r388iQSJq_4819/4819_all_crl-set-5934829738003798040.data.crx3");
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest, ModifyCRLSet3_http) {
  GURL url(
      "http://www.google.com/dl/release2/chrome_component/LLjIBPPmveI_4988/"
      "4988_all_crl-set-6296993568184466307.data.crx3");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(
      "https://crlsets.brave.com/dl/release2/chrome_component/LLjIBPPmveI_4988/"
      "4988_all_crl-set-6296993568184466307.data.crx3");
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest, ModifyCRXDownload_http) {
  GURL url(
      "http://clients2.googleusercontent.com/crx/blobs/QgAAAC6zw0qH2DJtn"
      "Xe8Z7rUJP1RM6lX7kVcwkQ56ujmG3AWYOAkxoNnIdnEBUz_"
      "3z4keVhjzzAF10srsaL7lrntfB"
      "IflcYIrTziwX3SUS9i_P-CAMZSmuV5tdQl-Roo6cnVC_GRzKsnZSKm1Q/"
      "extension_2_0_67"
      "3_0.crx");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(
      "https://crxdownload.brave.com/crx/blobs/QgAAAC6"
      "zw0qH2DJtnXe8Z7rUJP1RM6lX7kVcwkQ56ujmG3AWYOAkxoNnIdnEBUz_"
      "3z4keVhjzzAF10sr"
      "saL7lrntfBIflcYIrTziwX3SUS9i_P-CAMZSmuV5tdQl-Roo6cnVC_GRzKsnZSKm1Q/"
      "extens"
      "ion_2_0_673_0.crx");
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest, ModifySafeBrowsingURLV4) {
  GURL url(
      "https://safebrowsing.googleapis.com/v4/"
      "threatListUpdates:fetch?$req=ChkKCGNocm9taXVtEg02Ni");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL::Replacements replacements;
  replacements.SetHostStr(SAFEBROWSING_ENDPOINT);
  GURL expected_url(url.ReplaceComponents(replacements));
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest, ModifySafeBrowsingURLV5) {
  GURL url(
      "https://safebrowsing.googleapis.com/v5/"
      "threatListUpdates:fetch?$req=ChkKCGNocm9taXVtEg02Ni");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL::Replacements replacements;
  replacements.SetHostStr(SAFEBROWSING_ENDPOINT);
  GURL expected_url(url.ReplaceComponents(replacements));
  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

// TODO(@fmarier): Re-enable download protection once we have
// truncated the list of metadata that it sends to the server
// (brave/brave-browser#6267).
//TEST(BraveStaticRedirectNetworkDelegateHelperTest,
//       ModifySafeBrowsingFileCheckURL) {
//  GURL url(
//      "https://sb-ssl.google.com/safebrowsing/clientreport/download?"
//      "key=DUMMY_KEY");
//  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
//  brave::ResponseCallback callback;
//  GURL expected_url(
//      "https://sb-ssl.brave.com/safebrowsing/clientreport/download?"
//      "key=DUMMY_KEY");
//  int ret = OnBeforeURLRequest_StaticRedirectWork(callback, brave_request_info);
//  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
//  EXPECT_EQ(ret, net::OK);
//}

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
TEST(BraveStaticRedirectNetworkDelegateHelperTest, RedirectTranslate) {
  std::string query_string(
      "?cb=cr.googleTranslate.onTranslateElementLoad&aus=true&"
      "clc=cr.googleTranslate.onLoadCSS&"
      "jlc=cr.googleTranslate.onLoadJavascript&hl=en&key=DUMMY_KEY");
  std::string path_string("/translate_a/element.js");
  std::string google_host_string("https://translate.googleapis.com");
  GURL url(google_host_string + path_string + query_string);

  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(kBraveTranslateServer + path_string + query_string);

  int ret = OnBeforeURLRequest_StaticRedirectWork(callback,
      brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveStaticRedirectNetworkDelegateHelperTest,
    RedirectTranslateLanguage) {
  GURL url(
      "https://translate.googleapis.com/translate_a/l?"
      "client=chrome&hl=en&key=DUMMY_KEY");

  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  GURL expected_url(kBraveTranslateLanguageEndpoint);

  int ret = OnBeforeURLRequest_StaticRedirectWork(callback,
      brave_request_info);
  EXPECT_EQ(brave_request_info->new_url_spec, expected_url);
  EXPECT_EQ(ret, net::OK);
}
#endif

}  // namespace
