/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/ipfs_redirect_network_delegate_helper.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/net/url_context.h"
#include "brave/common/translate_network_constants.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "url/gurl.h"

namespace {

GURL GetLocalGateway() {
  return GURL("http://localhost:48080");
}

GURL GetPublicGateway() {
  return GURL("https://dweb.link");
}

const char initiator_cid[] =
    "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq";

}  // namespace

namespace ipfs {

class IPFSRedirectNetworkDelegateHelperTest : public testing::Test {
 public:
  IPFSRedirectNetworkDelegateHelperTest() : profile_(new TestingProfile) {}
  ~IPFSRedirectNetworkDelegateHelperTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
  }

  TestingProfile* profile() { return profile_.get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIHTTPScheme) {
  GURL url("http://a.com/ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->browser_context = profile();
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPFSSchemeLocal) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->browser_context = profile();
  brave_request_info->ipfs_gateway_url = GetLocalGateway();
  brave_request_info->initiator_url = ipfs::GetIPFSGatewayURL(
      initiator_cid, "",
      ipfs::GetDefaultIPFSLocalGateway(chrome::GetChannel()));
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(brave_request_info->new_url_spec,
            "http://localhost:48080/ipfs/"
            "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG/");
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPFSScheme) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->browser_context = profile();
  brave_request_info->ipfs_gateway_url = GetPublicGateway();
  brave_request_info->initiator_url = ipfs::GetIPFSGatewayURL(
      initiator_cid, "", ipfs::GetDefaultIPFSGateway(profile()->GetPrefs()));
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(
      brave_request_info->new_url_spec,
      "https://dweb.link/ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG/");
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPNSSchemeLocal) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->browser_context = profile();
  brave_request_info->ipfs_gateway_url = GetLocalGateway();
  brave_request_info->initiator_url = ipfs::GetIPFSGatewayURL(
      initiator_cid, "",
      ipfs::GetDefaultIPFSLocalGateway(chrome::GetChannel()));
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(brave_request_info->new_url_spec,
            "http://localhost:48080/ipns/"
            "QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd/");
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPNSScheme) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->browser_context = profile();
  brave_request_info->ipfs_gateway_url = GetPublicGateway();
  brave_request_info->initiator_url = ipfs::GetIPFSGatewayURL(
      initiator_cid, "", ipfs::GetDefaultIPFSGateway(profile()->GetPrefs()));
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(
      brave_request_info->new_url_spec,
      "https://dweb.link/ipns/QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd/");
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest, HeadersIPFSWorkWithRedirect) {
  GURL url(
      "https://cloudflare-ipfs.com/ipfs/"
      "QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->browser_context = profile();
  request_info->ipfs_gateway_url = GetPublicGateway();
  request_info->initiator_url = ipfs::GetIPFSGatewayURL(
      initiator_cid, "", ipfs::GetDefaultIPFSGateway(profile()->GetPrefs()));
  request_info->resource_type = blink::mojom::ResourceType::kImage;
  request_info->ipfs_auto_fallback = true;

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
      new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("x-ipfs-path", "/test");
  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
      new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url;

  int rc = ipfs::OnHeadersReceived_IPFSRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, brave::ResponseCallback(), request_info);

  EXPECT_EQ(rc, net::OK);

  std::string location;
  EXPECT_TRUE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                          &location));
  GURL converted_url = GURL("https://dweb.link/test");
  EXPECT_EQ(location, converted_url);
  EXPECT_EQ(allowed_unsafe_redirect_url, converted_url);
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest, HeadersIPFSWorkNoRedirect) {
  GURL url(
      "https://cloudflare-ipfs.com/ipfs/"
      "QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->browser_context = profile();
  request_info->ipfs_gateway_url = GetPublicGateway();
  request_info->initiator_url = ipfs::GetIPFSGatewayURL(
      initiator_cid, "", ipfs::GetDefaultIPFSGateway(profile()->GetPrefs()));
  request_info->resource_type = blink::mojom::ResourceType::kImage;
  request_info->ipfs_auto_fallback = false;

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
      new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("x-ipfs-path", "/test");
  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
      new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url;

  int rc = ipfs::OnHeadersReceived_IPFSRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, brave::ResponseCallback(), request_info);

  EXPECT_EQ(rc, net::OK);

  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                           &location));
  EXPECT_TRUE(allowed_unsafe_redirect_url.is_empty());

  request_info->request_url = GetAPIServer(chrome::GetChannel());
  request_info->ipfs_auto_fallback = true;

  rc = ipfs::OnHeadersReceived_IPFSRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, brave::ResponseCallback(), request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(allowed_unsafe_redirect_url.is_empty());
}

}  // namespace ipfs
