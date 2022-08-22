/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/decentralized_dns_network_delegate_helper.h"

#include <memory>

#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service_test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/decentralized_dns/constants.h"
#include "brave/components/decentralized_dns/pref_names.h"
#include "brave/components/decentralized_dns/utils.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/net_errors.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using brave::ResponseCallback;

namespace decentralized_dns {

class DecentralizedDnsNetworkDelegateHelperTest : public testing::Test {
 public:
  DecentralizedDnsNetworkDelegateHelperTest()
      : local_state_(std::make_unique<ScopedTestingLocalState>(
            TestingBrowserProcess::GetGlobal())) {}

  ~DecentralizedDnsNetworkDelegateHelperTest() override = default;

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &test_url_loader_factory_);
    json_rpc_service_ =
        brave_wallet::JsonRpcServiceFactory::GetServiceForContext(
            browser_context());
    json_rpc_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
  }

  void TearDown() override {
    profile_.reset();
    local_state_.reset();
  }

  content::BrowserContext* browser_context() { return profile_.get(); }
  TestingProfile* profile() { return profile_.get(); }
  PrefService* local_state() { return local_state_->Get(); }
  network::TestURLLoaderFactory& test_url_loader_factory() {
    return test_url_loader_factory_;
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  raw_ptr<brave_wallet::JsonRpcService> json_rpc_service_ = nullptr;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(DecentralizedDnsNetworkDelegateHelperTest,
       DecentralizedDnsPreRedirectWork) {
  GURL url("http://brave.crypto");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->browser_context = profile();

  // No redirect if resolve method is not set to Ethereum.
  EXPECT_FALSE(IsUnstoppableDomainsResolveMethodEthereum(local_state()));
  int rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
      ResponseCallback(), brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  local_state()->SetInteger(kUnstoppableDomainsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ETHEREUM));
  EXPECT_TRUE(IsUnstoppableDomainsResolveMethodEthereum(local_state()));

  // No redirect for OTR context.
  brave_request_info->browser_context =
      profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(ResponseCallback(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  brave_request_info->browser_context = profile();

  // TLD is not .crypto
  brave_request_info->request_url = GURL("http://test.com");
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(ResponseCallback(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  brave_request_info->request_url = url;

  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(ResponseCallback(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::ERR_IO_PENDING);

  // No redirect if ENS resolve method is not set to Ethereum.
  EXPECT_FALSE(IsENSResolveMethodEthereum(local_state()));
  brave_request_info->request_url = GURL("http://brave.eth");
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(ResponseCallback(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  local_state()->SetInteger(kENSResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ETHEREUM));
  EXPECT_TRUE(IsENSResolveMethodEthereum(local_state()));
  brave_request_info->request_url = GURL("http://brave.eth");
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(ResponseCallback(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::ERR_IO_PENDING);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
}

TEST_F(DecentralizedDnsNetworkDelegateHelperTest,
       DecentralizedDnsPreRedirectTLDs) {
  local_state()->SetInteger(kUnstoppableDomainsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ETHEREUM));
  struct TestCase {
    const char* url;
    bool is_valid;
  } test_cases[] = {
      {"https://brave.crypto", true},
      {"https://brave.x", true},
      {"https://brave.coin", true},
      {"https://brave.nft", true},
      {"https://brave.dao", true},
      {"https://brave.wallet", true},
      {"https://brave.888", true},
      {"https://brave.blockchain", true},
      {"https://brave.bitcoin", true},
      {"https://brave.zil", true},
      {"https://brave", false},
      {"https://brave.com", false},
      {"", false},
  };

  for (const auto& test_case : test_cases) {
    auto brave_request_info =
        std::make_shared<brave::BraveRequestInfo>(GURL(test_case.url));
    brave_request_info->browser_context = profile();
    EXPECT_EQ(test_case.is_valid ? net::ERR_IO_PENDING : net::OK,
              OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
                  ResponseCallback(), brave_request_info));
  }
}

TEST_F(DecentralizedDnsNetworkDelegateHelperTest,
       UnstoppableDomainsRedirectWork) {
  local_state()->SetInteger(kUnstoppableDomainsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ETHEREUM));

  GURL url("http://brave.crypto");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->browser_context = profile();

  auto polygon_spec = brave_wallet::GetUnstoppableDomainsRpcUrl(
                          brave_wallet::mojom::kPolygonMainnetChainId)
                          .spec();
  auto eth_spec = brave_wallet::GetUnstoppableDomainsRpcUrl(
                      brave_wallet::mojom::kMainnetChainId)
                      .spec();

  // No redirect for failed requests.
  EXPECT_EQ(net::ERR_IO_PENDING,
            OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
                ResponseCallback(), brave_request_info));
  test_url_loader_factory().SimulateResponseForPendingRequest(
      polygon_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"", "", "", "", "", "https://brave.com"}),
      net::HTTP_REQUEST_TIMEOUT);
  test_url_loader_factory().SimulateResponseForPendingRequest(
      eth_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"", "", "", "", "", "https://brave.com"}),
      net::HTTP_REQUEST_TIMEOUT);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  // Polygon result.
  EXPECT_EQ(net::ERR_IO_PENDING,
            OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
                ResponseCallback(), brave_request_info));
  test_url_loader_factory().SimulateResponseForPendingRequest(
      polygon_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"", "", "", "", "", "https://brave.com"}),
      net::HTTP_OK);
  test_url_loader_factory().SimulateResponseForPendingRequest(
      eth_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"hash", "", "", "", "", ""}),
      net::HTTP_OK);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(brave_request_info->new_url_spec, "https://brave.com/");

  // Eth result.
  EXPECT_EQ(net::ERR_IO_PENDING,
            OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
                ResponseCallback(), brave_request_info));
  test_url_loader_factory().SimulateResponseForPendingRequest(
      polygon_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse({"", "", "", "", "", ""}),
      net::HTTP_OK);
  test_url_loader_factory().SimulateResponseForPendingRequest(
      eth_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"hash", "", "", "", "", ""}),
      net::HTTP_OK);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(brave_request_info->new_url_spec, "ipfs://hash");
}

TEST_F(DecentralizedDnsNetworkDelegateHelperTest, EnsRedirectWork) {
  GURL url("http://brantly.eth");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);

  // No redirect for failed requests.
  OnBeforeURLRequest_EnsRedirectWork(
      ResponseCallback(), brave_request_info, "",
      brave_wallet::mojom::ProviderError::kInternalError, "todo");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  OnBeforeURLRequest_EnsRedirectWork(
      ResponseCallback(), brave_request_info, "",
      brave_wallet::mojom::ProviderError::kSuccess, "");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  // No redirect for invalid content hash.
  std::string content_hash_encoded_string =
      "0x0000000000000000000000000000000000000000000000000000000000000020000000"
      "000000000000000000000000000000000000000000000000000000002655010170122023"
      "e0160eec32d7875c19c5ac7c03bc1f306dc260080d621454bc5f631e7310a70000000000"
      "000000000000000000000000000000000000000000";

  std::string content_hash;
  EXPECT_TRUE(brave_wallet::DecodeString(66, content_hash_encoded_string,
                                         &content_hash));
  OnBeforeURLRequest_EnsRedirectWork(
      ResponseCallback(), brave_request_info, content_hash,
      brave_wallet::mojom::ProviderError::kSuccess, "");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  // Redirect for valid content hash.
  content_hash_encoded_string =
      "0x0000000000000000000000000000000000000000000000000000000000000020000000"
      "0000000000000000000000000000000000000000000000000000000026e3010170122023"
      "e0160eec32d7875c19c5ac7c03bc1f306dc260080d621454bc5f631e7310a70000000000"
      "000000000000000000000000000000000000000000";

  content_hash = "";
  EXPECT_TRUE(brave_wallet::DecodeString(66, content_hash_encoded_string,
                                         &content_hash));
  OnBeforeURLRequest_EnsRedirectWork(
      ResponseCallback(), brave_request_info, content_hash,
      brave_wallet::mojom::ProviderError::kSuccess, "");
  EXPECT_EQ(brave_request_info->new_url_spec,
            "ipfs://"
            "bafybeibd4ala53bs26dvygofvr6ahpa7gbw4eyaibvrbivf4l5rr44yqu4");
}

}  // namespace decentralized_dns
