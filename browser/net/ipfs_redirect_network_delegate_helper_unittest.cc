/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/ipfs_redirect_network_delegate_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/browser/net/url_context.h"
#include "brave/common/translate_network_constants.h"
#include "brave/components/ipfs/ipfs_gateway.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
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

TEST(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIHTTPScheme) {
  GURL url("http://a.com/ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
}

TEST(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPFSSchemeLocal) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->resolve_ipfs_enabled = true;
  brave_request_info->ipfs_gateway_url = GetLocalGateway();
  brave_request_info->initiator_url = ipfs::GetIPFSGatewayURL(
      initiator_cid, "",
      ipfs::GetDefaultIPFSLocalGateway(chrome::GetChannel()));
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(brave_request_info->new_url_spec,
            "http://localhost:48080/ipfs/"
            "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
}

TEST(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPFSScheme) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->resolve_ipfs_enabled = true;
  brave_request_info->ipfs_gateway_url = GetPublicGateway();
  brave_request_info->initiator_url =
      ipfs::GetIPFSGatewayURL(initiator_cid, "", ipfs::GetDefaultIPFSGateway());
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(
      brave_request_info->new_url_spec,
      "https://dweb.link/ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
}

TEST(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPNSSchemeLocal) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->resolve_ipfs_enabled = true;
  brave_request_info->ipfs_gateway_url = GetLocalGateway();
  brave_request_info->initiator_url = ipfs::GetIPFSGatewayURL(
      initiator_cid, "",
      ipfs::GetDefaultIPFSLocalGateway(chrome::GetChannel()));
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(brave_request_info->new_url_spec,
            "http://localhost:48080/ipns/"
            "QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
}

TEST(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPNSScheme) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->resolve_ipfs_enabled = true;
  brave_request_info->ipfs_gateway_url = GetPublicGateway();
  brave_request_info->initiator_url =
      ipfs::GetIPFSGatewayURL(initiator_cid, "", ipfs::GetDefaultIPFSGateway());
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(
      brave_request_info->new_url_spec,
      "https://dweb.link/ipns/QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
}

}  // namespace
