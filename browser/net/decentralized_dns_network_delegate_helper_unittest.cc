/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/decentralized_dns_network_delegate_helper.h"

#include <memory>

#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service_test_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/decentralized_dns/core/constants.h"
#include "brave/components/decentralized_dns/core/pref_names.h"
#include "brave/components/decentralized_dns/core/utils.h"
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
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            browser_context())
            ->json_rpc_service();
    json_rpc_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
  }

  void TearDown() override {
    json_rpc_service_ = nullptr;
    profile_.reset();
    local_state_.reset();
  }

  content::BrowserContext* browser_context() { return profile_.get(); }
  TestingProfile* profile() { return profile_.get(); }
  PrefService* local_state() { return local_state_->Get(); }
  network::TestURLLoaderFactory& test_url_loader_factory() {
    return test_url_loader_factory_;
  }

  content::BrowserTaskEnvironment task_environment_;

 private:
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
  EXPECT_FALSE(IsUnstoppableDomainsResolveMethodEnabled(local_state()));
  int rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
      base::DoNothing(), brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  local_state()->SetInteger(kUnstoppableDomainsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ENABLED));
  EXPECT_TRUE(IsUnstoppableDomainsResolveMethodEnabled(local_state()));

  // No redirect for OTR context.
  brave_request_info->browser_context =
      profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(base::DoNothing(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  brave_request_info->browser_context = profile();

  // TLD is not .crypto
  brave_request_info->request_url = GURL("http://test.com");
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(base::DoNothing(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  brave_request_info->request_url = url;

  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(base::DoNothing(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::ERR_IO_PENDING);

  // No redirect if ENS resolve method is not set to Ethereum.
  EXPECT_FALSE(IsENSResolveMethodEnabled(local_state()));
  brave_request_info->request_url = GURL("http://brave.eth");
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(base::DoNothing(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  local_state()->SetInteger(kENSResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ENABLED));
  EXPECT_TRUE(IsENSResolveMethodEnabled(local_state()));
  brave_request_info->request_url = GURL("http://brave.eth");
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(base::DoNothing(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::ERR_IO_PENDING);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
}

TEST_F(DecentralizedDnsNetworkDelegateHelperTest,
       DecentralizedDnsPreRedirectTLDs) {
  local_state()->SetInteger(kUnstoppableDomainsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ENABLED));
  struct TestCase {
    const char* url;
    bool is_valid;
  } test_cases[] = {
      {"https://brave.crypto", true},
      {"https://brave.x", true},
      {"https://brave.coin", false},
      {"https://brave.nft", true},
      {"https://brave.dao", true},
      {"https://brave.wallet", true},
      {"https://brave.888", false},
      {"https://brave.blockchain", true},
      {"https://brave.bitcoin", true},
      {"https://brave.zil", true},
      {"https://brave.altimist", true},
      {"https://brave.anime", true},
      {"https://brave.klever", true},
      {"https://brave.manga", true},
      {"https://brave.polygon", true},
      {"https://brave.unstoppable", true},
      {"https://brave.pudgy", true},
      {"https://brave.tball", true},
      {"https://brave.stepn", true},
      {"https://brave.secret", true},
      {"https://brave.raiin", true},
      {"https://brave.pog", true},
      {"https://brave.clay", true},
      {"https://brave.metropolis", true},
      {"https://brave.witg", true},
      {"https://brave.ubu", true},
      {"https://brave.kryptic", true},
      {"https://brave.farms", true},
      {"https://brave.dfz", true},
      {"https://brave.kresus", true},
      {"https://brave.binanceus", true},
      {"https://brave.austin", true},
      {"https://brave.bitget", true},
      {"https://brave.wrkx", true},
      {"https://brave.bald", true},
      {"https://brave.benji", true},
      {"https://brave.chomp", true},
      {"https://brave.dream", true},
      {"https://brave.ethermail", true},
      {"https://brave.lfg", true},
      {"https://brave.propykeys", true},
      {"https://brave.smobler", true},
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
                  base::DoNothing(), brave_request_info));
  }
}

TEST_F(DecentralizedDnsNetworkDelegateHelperTest,
       UnstoppableDomainsRedirectWork) {
  local_state()->SetInteger(kUnstoppableDomainsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ENABLED));

  GURL url("http://brave.crypto");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->browser_context = profile();

  auto polygon_spec = brave_wallet::NetworkManager::GetUnstoppableDomainsRpcUrl(
                          brave_wallet::mojom::kPolygonMainnetChainId)
                          .spec();
  auto eth_spec = brave_wallet::NetworkManager::GetUnstoppableDomainsRpcUrl(
                      brave_wallet::mojom::kMainnetChainId)
                      .spec();
  auto base_spec = brave_wallet::NetworkManager::GetUnstoppableDomainsRpcUrl(
                       brave_wallet::mojom::kBaseMainnetChainId)
                       .spec();

  // No redirect for failed requests.
  EXPECT_EQ(net::ERR_IO_PENDING,
            OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
                base::DoNothing(), brave_request_info));
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
  test_url_loader_factory().SimulateResponseForPendingRequest(
      base_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"", "", "", "", "", "https://brave.com"}),
      net::HTTP_REQUEST_TIMEOUT);
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  // Polygon result.
  EXPECT_EQ(net::ERR_IO_PENDING,
            OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
                base::DoNothing(), brave_request_info));
  test_url_loader_factory().SimulateResponseForPendingRequest(
      polygon_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"", "", "", "", "", "https://brave.com"}),
      net::HTTP_OK);
  test_url_loader_factory().SimulateResponseForPendingRequest(
      base_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"", "", "", "", "", "https://brave.com/base"}),
      net::HTTP_OK);
  test_url_loader_factory().SimulateResponseForPendingRequest(
      eth_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR", "", "", "", "",
           ""}),
      net::HTTP_OK);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(brave_request_info->new_url_spec, "https://brave.com/");

  // Base result.
  EXPECT_EQ(net::ERR_IO_PENDING,
            OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
                base::DoNothing(), brave_request_info));
  test_url_loader_factory().SimulateResponseForPendingRequest(
      polygon_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse({"", "", "", "", "", ""}),
      net::HTTP_OK);
  test_url_loader_factory().SimulateResponseForPendingRequest(
      base_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"", "", "", "", "", "https://brave.com/base"}),
      net::HTTP_OK);
  test_url_loader_factory().SimulateResponseForPendingRequest(
      eth_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR", "", "", "", "",
           ""}),
      net::HTTP_OK);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(brave_request_info->new_url_spec, "https://brave.com/base");

  // Eth result.
  EXPECT_EQ(net::ERR_IO_PENDING,
            OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
                base::DoNothing(), brave_request_info));
  test_url_loader_factory().SimulateResponseForPendingRequest(
      polygon_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse({"", "", "", "", "", ""}),
      net::HTTP_OK);
  test_url_loader_factory().SimulateResponseForPendingRequest(
      base_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse({"", "", "", "", "", ""}),
      net::HTTP_OK);
  test_url_loader_factory().SimulateResponseForPendingRequest(
      eth_spec,
      brave_wallet::MakeJsonRpcStringArrayResponse(
          {"QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR", "", "", "", "",
           ""}),
      net::HTTP_OK);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(
      brave_request_info->new_url_spec,
      "https://ipfs.io/ipfs/QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR");
}

TEST_F(DecentralizedDnsNetworkDelegateHelperTest, EnsRedirectWork) {
  GURL url("http://brantly.eth");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);

  // No redirect for failed requests.
  OnBeforeURLRequest_EnsRedirectWork(
      base::DoNothing(), brave_request_info, {}, false,
      brave_wallet::mojom::ProviderError::kInternalError, "todo");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  OnBeforeURLRequest_EnsRedirectWork(
      base::DoNothing(), brave_request_info, {}, false,
      brave_wallet::mojom::ProviderError::kSuccess, "");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  // No redirect for invalid content hash.
  std::string content_hash_encoded_string =
      "0x0000000000000000000000000000000000000000000000000000000000000020000000"
      "000000000000000000000000000000000000000000000000000000002655010170122023"
      "e0160eec32d7875c19c5ac7c03bc1f306dc260080d621454bc5f631e7310a70000000000"
      "000000000000000000000000000000000000000000";

  auto content_hash = *brave_wallet::eth_abi::ExtractBytesFromTuple(
      *brave_wallet::PrefixedHexStringToBytes(content_hash_encoded_string), 0);
  OnBeforeURLRequest_EnsRedirectWork(
      base::DoNothing(), brave_request_info, content_hash, false,
      brave_wallet::mojom::ProviderError::kSuccess, "");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  // Redirect for valid content hash.
  content_hash_encoded_string =
      "0x0000000000000000000000000000000000000000000000000000000000000020000000"
      "0000000000000000000000000000000000000000000000000000000026e3010170122023"
      "e0160eec32d7875c19c5ac7c03bc1f306dc260080d621454bc5f631e7310a70000000000"
      "000000000000000000000000000000000000000000";

  content_hash = *brave_wallet::eth_abi::ExtractBytesFromTuple(
      *brave_wallet::PrefixedHexStringToBytes(content_hash_encoded_string), 0);
  OnBeforeURLRequest_EnsRedirectWork(
      base::DoNothing(), brave_request_info, content_hash, false,
      brave_wallet::mojom::ProviderError::kSuccess, "");
  EXPECT_EQ(brave_request_info->new_url_spec,
            "https://ipfs.io/ipfs/"
            "bafybeibd4ala53bs26dvygofvr6ahpa7gbw4eyaibvrbivf4l5rr44yqu4");

  EXPECT_FALSE(brave_request_info->pending_error.has_value());
}

TEST_F(DecentralizedDnsNetworkDelegateHelperTest,
       EnsRedirect_OffchainLookupRequired) {
  GURL url("http://brantly.eth");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);

  // Offchain lookup required.
  OnBeforeURLRequest_EnsRedirectWork(
      base::DoNothing(), brave_request_info, {}, true,
      brave_wallet::mojom::ProviderError::kSuccess, "");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  EXPECT_EQ(brave_request_info->pending_error,
            net::ERR_ENS_OFFCHAIN_LOOKUP_NOT_SELECTED);
}

TEST_F(DecentralizedDnsNetworkDelegateHelperTest, SnsRedirectWork) {
  GURL url("http://test.sol");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);

  // No redirect for failed requests.
  OnBeforeURLRequest_SnsRedirectWork(
      base::DoNothing(), brave_request_info, {},
      brave_wallet::mojom::SolanaProviderError::kInternalError, "todo");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  OnBeforeURLRequest_SnsRedirectWork(
      base::DoNothing(), brave_request_info, {},
      brave_wallet::mojom::SolanaProviderError::kSuccess, "");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  // No redirect for invalid url.
  OnBeforeURLRequest_SnsRedirectWork(
      base::DoNothing(), brave_request_info, GURL("invalid"),
      brave_wallet::mojom::SolanaProviderError::kSuccess, "");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  // Redirect for valid url.
  OnBeforeURLRequest_SnsRedirectWork(
      base::DoNothing(), brave_request_info, GURL("https://brave.com"),
      brave_wallet::mojom::SolanaProviderError::kSuccess, "");
  EXPECT_EQ(brave_request_info->new_url_spec, GURL("https://brave.com"));

  EXPECT_FALSE(brave_request_info->pending_error.has_value());
}

}  // namespace decentralized_dns
