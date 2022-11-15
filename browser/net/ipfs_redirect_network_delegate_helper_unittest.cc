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
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
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
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));

  GURL url("http://a.com/ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->browser_context = profile();
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPFSSchemeLocal) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

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
            "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest,
       ProperMainFrameErrorCodeWhenIPFSDisabled) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));

  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->resource_type = blink::mojom::ResourceType::kMainFrame;
  brave_request_info->browser_context = profile();
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::ERR_IPFS_DISABLED);
  EXPECT_EQ(brave_request_info->blocked_by, brave::kOtherBlocked);
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest,
       SubFrameRequestDisabledWhenIPFSDisabled) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));

  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->resource_type = blink::mojom::ResourceType::kSubFrame;
  brave_request_info->browser_context = profile();
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(brave_request_info->blocked_by, brave::kOtherBlocked);
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest,
       SubFrameRequestDisabledWhenIPFSDisabled_Incognito) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->resource_type = blink::mojom::ResourceType::kMainFrame;
  brave_request_info->browser_context = profile()->GetOffTheRecordProfile(
      Profile::OTRProfileID::CreateUnique("incognito"), true);
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::ERR_INCOGNITO_IPFS_NOT_ALLOWED);
  EXPECT_EQ(brave_request_info->blocked_by, brave::kOtherBlocked);
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest,
       SubFrameRequestDisabledWhenIPFSDisabled_Incognito_Subframe) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->resource_type = blink::mojom::ResourceType::kSubFrame;
  brave_request_info->browser_context = profile()->GetOffTheRecordProfile(
      Profile::OTRProfileID::CreateUnique("incognito"), true);
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(brave_request_info->blocked_by, brave::kOtherBlocked);
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest,
       LoadDisabledWhenIPFS_WhenWrongIPFSUrl) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  // IPFS Subframe
  {
    GURL url("ipfs://10.10.10.1");
    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
    brave_request_info->resource_type = blink::mojom::ResourceType::kSubFrame;
    brave_request_info->browser_context = profile();
    int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(
        brave::ResponseCallback(), brave_request_info);
    EXPECT_EQ(rc, net::OK);
    ASSERT_EQ(brave_request_info->blocked_by, brave::kOtherBlocked);
  }

  // IPFS Mainframe
  {
    GURL url("ipfs://10.10.10.1");
    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
    brave_request_info->resource_type = blink::mojom::ResourceType::kMainFrame;
    brave_request_info->browser_context = profile();
    int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(
        brave::ResponseCallback(), brave_request_info);
    EXPECT_EQ(rc, net::OK);
    ASSERT_EQ(brave_request_info->blocked_by, brave::kOtherBlocked);
  }

  // IPNS Subframe
  {
    GURL url("ipns://10.10.10.1");
    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
    brave_request_info->resource_type = blink::mojom::ResourceType::kSubFrame;
    brave_request_info->browser_context = profile();
    int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(
        brave::ResponseCallback(), brave_request_info);
    EXPECT_EQ(rc, net::OK);
    ASSERT_EQ(brave_request_info->blocked_by, brave::kOtherBlocked);
  }

  // IPFS Mainframe
  {
    GURL url("ipns://10.10.10.1");
    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
    brave_request_info->resource_type = blink::mojom::ResourceType::kMainFrame;
    brave_request_info->browser_context = profile();
    int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(
        brave::ResponseCallback(), brave_request_info);
    EXPECT_EQ(rc, net::OK);
    // It is correct ipns url.
    ASSERT_EQ(brave_request_info->blocked_by, brave::kNotBlocked);
  }
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest,
       SubFrameRequestDisabledWhen_NoContext) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->resource_type = blink::mojom::ResourceType::kSubFrame;
  brave_request_info->browser_context = nullptr;
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(brave_request_info->blocked_by, brave::kOtherBlocked);
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPFSScheme) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));

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
      "https://dweb.link/ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPNSSchemeLocal) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

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
            "QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest, TranslateIPFSURIIPNSScheme) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));

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
      "https://dweb.link/ipns/QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
}

TEST_F(IPFSRedirectNetworkDelegateHelperTest, PrivateProfile) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->browser_context = profile()->GetPrimaryOTRProfile(true);
  brave_request_info->ipfs_gateway_url = GetPublicGateway();
  brave_request_info->resource_type = blink::mojom::ResourceType::kMainFrame;
  brave_request_info->initiator_url = ipfs::GetIPFSGatewayURL(
      initiator_cid, "", ipfs::GetDefaultIPFSGateway(profile()->GetPrefs()));
  int rc = ipfs::OnBeforeURLRequest_IPFSRedirectWork(brave::ResponseCallback(),
                                                     brave_request_info);
  EXPECT_EQ(rc, net::ERR_INCOGNITO_IPFS_NOT_ALLOWED);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
}

}  // namespace ipfs
