/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/nft_metadata_fetcher.h"

#include <memory>
#include <string_view>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

// Compare two JSON strings, ignoring the order of the keys and other
// insignificant whitespace differences.
void CompareJSON(const std::string& response,
                 const std::string& expected_response) {
  auto response_val = base::JSONReader::Read(response);
  auto expected_response_val = base::JSONReader::Read(expected_response);
  EXPECT_EQ(response_val, expected_response_val);
  if (response_val) {
    // If the JSON is valid, compare the parsed values.
    EXPECT_EQ(*response_val, *expected_response_val);
  } else {
    // If the JSON is invalid, compare the raw strings.
    EXPECT_EQ(response, expected_response);
  }
}
constexpr char https_metadata_response[] = R"({
    "attributes": [
      {
        "trait_type": "Feet",
        "value": "Green Shoes"
      },
      {
        "trait_type": "Legs",
        "value": "Tan Pants"
      },
      {
        "trait_type": "Suspenders",
        "value": "White Suspenders"
      },
      {
        "trait_type": "Upper Body",
        "value": "Indigo Turtleneck"
      },
      {
        "trait_type": "Sleeves",
        "value": "Long Sleeves"
      },
      {
        "trait_type": "Hat",
        "value": "Yellow / Blue Pointy Beanie"
      },
      {
        "trait_type": "Eyes",
        "value": "White Nerd Glasses"
      },
      {
        "trait_type": "Mouth",
        "value": "Toothpick"
      },
      {
        "trait_type": "Ears",
        "value": "Bing Bong Stick"
      },
      {
        "trait_type": "Right Arm",
        "value": "Swinging"
      },
      {
        "trait_type": "Left Arm",
        "value": "Diamond Hand"
      },
      {
        "trait_type": "Background",
        "value": "Blue"
      }
    ],
    "description": "5,000 animated Invisible Friends hiding in the metaverse. A collection by Markus Magnusson & Random Character Collective.",
    "image": "https://rcc.mypinata.cloud/ipfs/QmXmuSenZRnofhGMz2NyT3Yc4Zrty1TypuiBKDcaBsNw9V/1817.gif",
    "name": "Invisible Friends #1817"
  })";

}  // namespace

class NftMetadataFetcherUnitTest : public testing::Test {
 public:
  NftMetadataFetcherUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_ = std::make_unique<brave_wallet::JsonRpcService>(
        shared_url_loader_factory_, &prefs_);
    nft_metadata_fetcher_ = std::make_unique<NftMetadataFetcher>(
        shared_url_loader_factory_, json_rpc_service_.get(), GetPrefs());
  }

  PrefService* GetPrefs() { return &prefs_; }

  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return brave_wallet::GetNetworkURL(GetPrefs(), chain_id, coin);
  }

  void TestFetchMetadata(const GURL& url,
                         const std::string& expected_response,
                         int expected_error,
                         const std::string& expected_error_message) {
    base::RunLoop run_loop;
    nft_metadata_fetcher_->FetchMetadata(
        url,
        base::BindLambdaForTesting([&](const std::string& response, int error,
                                       const std::string& error_message) {
          CompareJSON(response, expected_response);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetEthTokenMetadata(const std::string& contract,
                               const std::string& token_id,
                               const std::string& chain_id,
                               const std::string& interface_id,
                               const std::string& expected_response,
                               mojom::ProviderError expected_error,
                               const std::string& expected_error_message) {
    base::RunLoop run_loop;
    nft_metadata_fetcher_->GetEthTokenMetadata(
        contract, token_id, chain_id, interface_id,
        base::BindLambdaForTesting(
            [&](const std::string& url, const std::string& response,
                mojom::ProviderError error, const std::string& error_message) {
              CompareJSON(response, expected_response);
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetSolTokenMetadata(const std::string& chain_id,
                               const std::string& token_mint_address,
                               const std::string& expected_response,
                               mojom::SolanaProviderError expected_error,
                               const std::string& expected_error_message) {
    base::RunLoop loop;
    nft_metadata_fetcher_->GetSolTokenMetadata(
        chain_id, token_mint_address,
        base::BindLambdaForTesting([&](const std::string& token_url,
                                       const std::string& response,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          CompareJSON(response, expected_response);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          loop.Quit();
        }));
    loop.Run();
  }

  void SetInterceptor(const GURL& expected_url, const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, expected_url, content](const network::ResourceRequest& request) {
          EXPECT_EQ(request.url, expected_url);
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetInvalidJsonInterceptor() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "Answer is 42");
        }));
  }

  void SetHTTPRequestTimeoutInterceptor() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "",
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
  }

  void SetTokenMetadataInterceptor(
      const std::string& interface_id,
      const std::string& chain_id,
      const std::string& supports_interface_provider_response,
      const std::string& token_uri_provider_response = "",
      const std::string& metadata_response = "",
      net::HttpStatusCode supports_interface_status = net::HTTP_OK,
      net::HttpStatusCode token_uri_status = net::HTTP_OK,
      net::HttpStatusCode metadata_status = net::HTTP_OK) {
    GURL network_url =
        GetNetworkURL(GetPrefs(), chain_id, mojom::CoinType::ETH);
    ASSERT_TRUE(network_url.is_valid());
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, interface_id, supports_interface_provider_response,
         token_uri_provider_response, metadata_response,
         supports_interface_status, token_uri_status, metadata_status,
         network_url](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          if (request.method ==
              "POST") {  // An eth_call, either to supportsInterface or tokenURI
            std::string_view request_string(request.request_body->elements()
                                                ->at(0)
                                                .As<network::DataElementBytes>()
                                                .AsStringPiece());
            bool is_supports_interface_req =
                request_string.find(GetFunctionHash(
                    "supportsInterface(bytes4)")) != std::string::npos;
            if (is_supports_interface_req) {
              ASSERT_NE(request_string.find(interface_id.substr(2)),
                        std::string::npos);
              EXPECT_EQ(request.url.spec(), network_url);
              url_loader_factory_.AddResponse(
                  network_url.spec(), supports_interface_provider_response,
                  supports_interface_status);
              return;
            } else {
              std::string function_hash;
              if (interface_id == kERC721MetadataInterfaceId) {
                function_hash = GetFunctionHash("tokenURI(uint256)");
              } else {
                function_hash = GetFunctionHash("uri(uint256)");
              }
              ASSERT_NE(request_string.find(function_hash), std::string::npos);
              url_loader_factory_.AddResponse(network_url.spec(),
                                              token_uri_provider_response,
                                              token_uri_status);
              return;
            }
          } else {  // A HTTP GET to fetch the metadata json from the web
            url_loader_factory_.AddResponse(request.url.spec(),
                                            metadata_response, metadata_status);
            return;
          }
        }));
  }

  void SetSolTokenMetadataInterceptor(
      const GURL& expected_rpc_url,
      const std::string& get_account_info_response,
      const GURL& expected_metadata_url,
      const std::string& metadata_response) {
    ASSERT_TRUE(expected_rpc_url.is_valid());
    ASSERT_TRUE(expected_metadata_url.is_valid());
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, expected_rpc_url, get_account_info_response, expected_metadata_url,
         metadata_response](const network::ResourceRequest& request) {
          url_loader_factory_.AddResponse(expected_rpc_url.spec(),
                                          get_account_info_response);
          url_loader_factory_.AddResponse(expected_metadata_url.spec(),
                                          metadata_response);
        }));
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  // NftMetadataFetcher nft_metadata_fetcher_;
  std::unique_ptr<NftMetadataFetcher> nft_metadata_fetcher_;
};

TEST_F(NftMetadataFetcherUnitTest, FetchMetadata) {
  // Invalid URL yields internal error
  TestFetchMetadata(GURL("invalid url"), "",
                    static_cast<int>(mojom::JsonRpcError::kInternalError),
                    l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Unsupported scheme yields internal error
  TestFetchMetadata(GURL("file://host/path"), "",
                    static_cast<int>(mojom::JsonRpcError::kInternalError),
                    l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Data URL with unsupported mime type yields parsing error
  TestFetchMetadata(
      GURL("data:text/"
           "csv;base64,eyJpbWFnZV91cmwiOiAgImh0dHBzOi8vZXhhbXBsZS5jb20ifQ=="),
      "", static_cast<int>(mojom::JsonRpcError::kParsingError),
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid URL but that results in HTTP timeout yields internal error
  SetHTTPRequestTimeoutInterceptor();
  TestFetchMetadata(GURL("https://example.com"), "",
                    static_cast<int>(mojom::JsonRpcError::kInternalError),
                    l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Valid URL but invalid json response yields parsing error
  SetInvalidJsonInterceptor();
  TestFetchMetadata(GURL("https://example.com"), "",
                    static_cast<int>(mojom::JsonRpcError::kParsingError),
                    l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // All valid yields response json set via interceptor
  GURL url = GURL("https://example.com");
  const std::string& metadata_json =
      R"({"image_url":"https://example.com/image.jpg"})";
  SetInterceptor(url, metadata_json);
  TestFetchMetadata(url, metadata_json,
                    static_cast<int>(mojom::ProviderError::kSuccess), "");
}

TEST_F(NftMetadataFetcherUnitTest, GetEthTokenMetadata) {
  // Decoded result is `https://invisiblefriends.io/api/1817`
  const std::string https_token_uri_response = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000002468747470733a2f2f696e76697369626c65667269656e64732e696f2f6170692f3138313700000000000000000000000000000000000000000000000000000000"
  })";

  // Decoded result is `http://invisiblefriends.io/api/1`
  const std::string http_token_uri_response = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000020687474703a2f2f696e76697369626c65667269656e64732e696f2f6170692f31"
  })";

  // Decoded result is
  // `data:application/json;base64,eyJhdHRyaWJ1dGVzIjoiIiwiZGVzY3JpcHRpb24iOiJOb24gZnVuZ2libGUgbGlvbiIsImltYWdlIjoiZGF0YTppbWFnZS9zdmcreG1sO2Jhc2U2NCxQSE4yWnlCNGJXeHVjejBpYUhSMGNEb3ZMM2QzZHk1M015NXZjbWN2TWpBd01DOXpkbWNpSUhacFpYZENiM2c5SWpBZ01DQTFNREFnTlRBd0lqNDhjR0YwYUNCa1BTSWlMejQ4TDNOMlp6ND0iLCJuYW1lIjoiTkZMIn0=`
  // Decoded base64 is `{"attributes":"","description":"Non fungible
  // lion","image":"data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA1MDAgNTAwIj4d8cGF0aCBkPSIiLz48L3N2Zz4=","name":"NFL"}`
  const std::string data_token_uri_response = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000135646174613a6170706c69636174696f6e2f6a736f6e3b6261736536342c65794a686448527961574a316447567a496a6f69496977695a47567a59334a7063485270623234694f694a4f623234675a6e56755a326c696247556762476c7662694973496d6c745957646c496a6f695a474630595470706257466e5a53397a646d6372654731734f324a68633255324e43785153453479576e6c434e474a586548566a656a4270595568534d474e4562335a4d4d32517a5a486b314d3031354e585a6a62574e3254577042643031444f58706b62574e7053556861634670595a454e694d326335535770425a3031445154464e5245466e546c524264306c714e44686a5230597759554e436131425453576c4d656a513454444e4f4d6c70364e4430694c434a755957316c496a6f69546b5a4d496e303d0000000000000000000000"
  })";

  // Decoded result is
  // `data:application/json;base64,eyJuYW1lIjoiTkZMIiwgImRlc2NyaXB0aW9uIjoiTm9uIGZ1bmdpYmxlIGxpb24iLCAiYXR0cmlidXRlcyI6IiIsICJpbWFnZSI6IiI=`
  // Decoded base64 is `{"name":"NFL", "description":"Non fungible lion",
  // "attributes":"", "image":""`
  const std::string data_token_uri_response_invalid_json = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":"0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000085646174613a6170706c69636174696f6e2f6a736f6e3b6261736536342c65794a755957316c496a6f69546b5a4d49697767496d526c63324e796158423061573975496a6f69546d397549475a31626d6470596d786c49477870623234694c43416959585230636d6c696458526c637949364969497349434a706257466e5a5349364969493d000000000000000000000000000000000000000000000000000000"
  })";

  // Decoded result is `data:application/json;base64,`
  const std::string data_token_uri_response_empty_string = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":"0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001d646174613a6170706c69636174696f6e2f6a736f6e3b6261736536342c000000"
  })";

  // Decoded result is `true`
  const std::string interface_supported_response = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result": "0x0000000000000000000000000000000000000000000000000000000000000001"
  })";
  const std::string exceeds_limit_json = R"({
    "jsonrpc":"2.0",
    "id":1,
    "error": {
      "code":-32005,
      "message": "Request exceeds defined limit"
    }
  })";

  // Decoded result is `false`
  const std::string interface_not_supported_response = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000000"
  })";
  const std::string invalid_json =
      "It might make sense just to get some in case it catches on";

  // Invalid inputs
  // (1/3) Invalid contract address
  TestGetEthTokenMetadata(
      "", "0x1", mojom::kMainnetChainId, kERC721MetadataInterfaceId, "",
      mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // (2/3) Invalid token ID
  SetTokenMetadataInterceptor(
      kERC721MetadataInterfaceId, mojom::kMainnetChainId,
      interface_supported_response, https_token_uri_response,
      https_metadata_response);
  TestGetEthTokenMetadata(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "", mojom::kMainnetChainId,
      kERC721MetadataInterfaceId, "", mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // (3/3) Invalid chain ID
  TestGetEthTokenMetadata(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1", "",
      kERC721MetadataInterfaceId, "", mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Mismatched
  // (4/4) Unknown interfaceID
  SetTokenMetadataInterceptor(
      kERC721MetadataInterfaceId, mojom::kMainnetChainId,
      interface_supported_response, https_token_uri_response,
      https_metadata_response);
  TestGetEthTokenMetadata(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      mojom::kMainnetChainId, "invalid interface", "",
      mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Valid inputs
  // (1/3) HTTP URI
  SetTokenMetadataInterceptor(
      kERC721MetadataInterfaceId, mojom::kMainnetChainId,
      interface_supported_response, https_token_uri_response,
      https_metadata_response);
  TestGetEthTokenMetadata("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          https_metadata_response,
                          mojom::ProviderError::kSuccess, "");

  // (3/3) Data URI
  SetTokenMetadataInterceptor(
      kERC721MetadataInterfaceId, mojom::kMainnetChainId,
      interface_supported_response, data_token_uri_response);
  TestGetEthTokenMetadata("0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          R"({
        "attributes": "",
        "description": "Non fungible lion",
        "image": "data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA1MDAgNTAwIj48cGF0aCBkPSIiLz48L3N2Zz4=",
        "name": "NFL"
      })",
                          mojom::ProviderError::kSuccess, "");

  // Invalid supportsInterface response
  // (1/4) Timeout
  SetTokenMetadataInterceptor(
      kERC721MetadataInterfaceId, mojom::kMainnetChainId,
      interface_supported_response, https_token_uri_response, "",
      net::HTTP_REQUEST_TIMEOUT);
  TestGetEthTokenMetadata("0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          "", mojom::ProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // (2/4) Invalid JSON
  SetTokenMetadataInterceptor(kERC721MetadataInterfaceId,
                              mojom::kMainnetChainId, invalid_json);
  TestGetEthTokenMetadata("0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          "", mojom::ProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // (3/4) Request exceeds provider limit
  SetTokenMetadataInterceptor(kERC721MetadataInterfaceId,
                              mojom::kMainnetChainId, exceeds_limit_json);
  TestGetEthTokenMetadata("0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          "", mojom::ProviderError::kLimitExceeded,
                          "Request exceeds defined limit");

  // (4/4) Interface not supported
  SetTokenMetadataInterceptor(kERC721MetadataInterfaceId,
                              mojom::kMainnetChainId,
                              interface_not_supported_response);
  TestGetEthTokenMetadata(
      "0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d", "0x719",
      mojom::kMainnetChainId, kERC721MetadataInterfaceId, "",
      mojom::ProviderError::kMethodNotSupported,
      l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR));

  // Invalid tokenURI response (6 total)
  // (1/6) Timeout
  SetTokenMetadataInterceptor(
      kERC721MetadataInterfaceId, mojom::kMainnetChainId,
      interface_supported_response, https_token_uri_response, "", net::HTTP_OK,
      net::HTTP_REQUEST_TIMEOUT);
  TestGetEthTokenMetadata("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          "", mojom::ProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // (2/6) Invalid Provider JSON
  SetTokenMetadataInterceptor(kERC721MetadataInterfaceId,
                              mojom::kMainnetChainId,
                              interface_supported_response, invalid_json);
  TestGetEthTokenMetadata("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          "", mojom::ProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // (3/6) Invalid JSON in data URI
  SetTokenMetadataInterceptor(
      kERC721MetadataInterfaceId, mojom::kMainnetChainId,
      interface_supported_response, data_token_uri_response_invalid_json);
  TestGetEthTokenMetadata("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          "", mojom::ProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // (4/6) Empty string as JSON in data URI
  SetTokenMetadataInterceptor(
      kERC721MetadataInterfaceId, mojom::kMainnetChainId,
      interface_supported_response, data_token_uri_response_empty_string);
  TestGetEthTokenMetadata("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          "", mojom::ProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // (5/6) Request exceeds limit
  SetTokenMetadataInterceptor(kERC721MetadataInterfaceId,
                              mojom::kMainnetChainId,
                              interface_supported_response, exceeds_limit_json);
  TestGetEthTokenMetadata("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          "", mojom::ProviderError::kLimitExceeded,
                          "Request exceeds defined limit");

  // (6/6) URI scheme is not suported (HTTP)
  SetTokenMetadataInterceptor(
      kERC721MetadataInterfaceId, mojom::kMainnetChainId,
      interface_supported_response, http_token_uri_response);
  TestGetEthTokenMetadata("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          "", mojom::ProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Invalid metadata response (2 total)
  // (1/2) Timeout
  SetTokenMetadataInterceptor(
      kERC721MetadataInterfaceId, mojom::kMainnetChainId,
      interface_supported_response, https_token_uri_response,
      https_metadata_response, net::HTTP_OK, net::HTTP_OK,
      net::HTTP_REQUEST_TIMEOUT);
  TestGetEthTokenMetadata("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                          mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                          "", mojom::ProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // ERC1155
  SetTokenMetadataInterceptor(
      kERC1155MetadataInterfaceId, mojom::kMainnetChainId,
      interface_supported_response, https_token_uri_response,
      https_metadata_response);
  TestGetEthTokenMetadata("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                          mojom::kMainnetChainId, kERC1155MetadataInterfaceId,
                          https_metadata_response,
                          mojom::ProviderError::kSuccess, "");
}

TEST_F(NftMetadataFetcherUnitTest, GetSolTokenMetadata) {
  // Valid inputs should yield metadata JSON (happy case)
  std::string get_account_info_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.3",
        "slot": 161038284
      },
      "value": {
        "data": [
          "BGUN5hJf2zSue3S0I/fCq16UREt5NxP6mQdaq4cdGPs3Q8PG/R6KFUSgce78Nwk9Frvkd9bMbvTIKCRSDy88nZQgAAAAU1BFQ0lBTCBTQVVDRQAAAAAAAAAAAAAAAAAAAAAAAAAKAAAAAAAAAAAAAAAAAMgAAABodHRwczovL2JhZmtyZWlmNHd4NTR3anI3cGdmdWczd2xhdHIzbmZudHNmd25ndjZldXNlYmJxdWV6cnhlbmo2Y2s0LmlwZnMuZHdlYi5saW5rP2V4dD0AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOgDAQIAAABlDeYSX9s0rnt0tCP3wqtelERLeTcT+pkHWquHHRj7NwFiDUmu+U8sXOOZQXL36xmknL+Zzd/z3uw2G0ERMo8Eth4BAgABAf8BAAEBoivvbAzLh2kD2cSu6IQIqGQDGeoh/UEDizyp6mLT1tUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
          "base64"
        ],
        "executable": false,
        "lamports": 5616720,
        "owner": "metaqbxxUerdq28cj1RbAWkYQm3ybzjb6a8bt518x1s",
        "rentEpoch": 361
      }
    },
    "id": 1
  })";
  const std::string valid_metadata_response = R"({
    "attributes": [
      {
        "trait_type": "hair",
        "value": "green & blue"
      },
      {
        "trait_type": "pontus",
        "value": "no"
      }
    ],
    "description": "",
    "external_url": "",
    "image": "https://bafkreiagsgqhjudpta6trhjuv5y2n2exsrhbkkprl64tvg2mftjsdm3vgi.ipfs.dweb.link?ext=png",
    "name": "SPECIAL SAUCE",
    "properties": {
      "category": "image",
      "creators": [
        {
          "address": "7oUUEdptZnZVhSet4qobU9PtpPfiNUEJ8ftPnrC6YEaa",
          "share": 98
        },
        {
          "address": "tsU33UT3K2JTfLgHUo7hdzRhRe4wth885cqVbM8WLiq",
          "share": 2
        }
      ],
      "files": [
        {
          "type": "image/png",
          "uri": "https://bafkreiagsgqhjudpta6trhjuv5y2n2exsrhbkkprl64tvg2mftjsdm3vgi.ipfs.dweb.link?ext=png"
        }
      ],
      "maxSupply": 0
    },
    "seller_fee_basis_points": 1000,
    "symbol": ""
  })";
  auto network_url = GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata(
      mojom::kSolanaMainnet, "5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh",
      valid_metadata_response, mojom::SolanaProviderError::kSuccess, "");

  // Invalid token_mint_address yields internal error.
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata(mojom::kSolanaMainnet, "Invalid", "",
                          mojom::SolanaProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Non 200 getAccountInfo response of yields internal server error.
  SetHTTPRequestTimeoutInterceptor();
  TestGetSolTokenMetadata(mojom::kSolanaMainnet,
                          "5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Invalid getAccountInfo response JSON yields internal error
  SetSolTokenMetadataInterceptor(
      network_url, "Invalid json response",
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata(mojom::kSolanaMainnet,
                          "5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Valid response JSON, invalid account info (missing result.value.owner
  // field) info yields parse error
  get_account_info_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.3",
        "slot": 161038284
      },
      "value": {
        "data": [
          "BGUN5hJf2zSue3S0I/fCq16UREt5NxP6mQdaq4cdGPs3Q8PG/R6KFUSgce78Nwk9Frvkd9bMbvTIKCRSDy88nZQgAAAAU1BFQ0lBTCBTQVVDRQAAAAAAAAAAAAAAAAAAAAAAAAAKAAAAAAAAAAAAAAAAAMgAAABodHRwczovL2JhZmtyZWlmNHd4NTR3anI3cGdmdWczd2xhdHIzbmZudHNmd25ndjZldXNlYmJxdWV6cnhlbmo2Y2s0LmlwZnMuZHdlYi5saW5rP2V4dD0AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOgDAQIAAABlDeYSX9s0rnt0tCP3wqtelERLeTcT+pkHWquHHRj7NwFiDUmu+U8sXOOZQXL36xmknL+Zzd/z3uw2G0ERMo8Eth4BAgABAf8BAAEBoivvbAzLh2kD2cSu6IQIqGQDGeoh/UEDizyp6mLT1tUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
          "base64"
        ],
        "executable": false,
        "lamports": 5616720,
        "rentEpoch": 361
      }
    },
    "id": 1
  })";
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata(mojom::kSolanaMainnet,
                          "5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid response JSON, parsable account info, but invalid account info data
  // (invalid base64) yields parse error
  get_account_info_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.3",
        "slot": 161038284
      },
      "value": {
        "data": [
          "*Invalid Base64*",
          "base64"
        ],
        "executable": false,
        "lamports": 5616720,
        "owner": "metaqbxxUerdq28cj1RbAWkYQm3ybzjb6a8bt518x1s",
        "rentEpoch": 361
      }
    },
    "id": 1
  })";
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata(mojom::kSolanaMainnet,
                          "5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid response JSON, parsable account info, invalid account info data
  // (valid base64, but invalid borsh encoded metadata) yields parse error
  get_account_info_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.3",
        "slot": 161038284
      },
      "value": {
        "data": [
          "d2hvb3BzIQ==",
          "base64"
        ],
        "executable": false,
        "lamports": 5616720,
        "owner": "metaqbxxUerdq28cj1RbAWkYQm3ybzjb6a8bt518x1s",
        "rentEpoch": 361
      }
    },
    "id": 1
  })";
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata(mojom::kSolanaMainnet,
                          "5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid response JSON, parsable account info, invalid account info data
  // (valid base64, valid borsh encoding, but when decoded the URI is not a
  // valid URI)
  get_account_info_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.3",
        "slot": 161038284
      },
      "value": {
        "data": [
          "BGUN5hJf2zSue3S0I/fCq16UREt5NxP6mQdaq4cdGPs3Q8PG/R6KFUSgce78Nwk9Frvkd9bMbvTIKCRSDy88nZQgAAAAU1BFQ0lBTCBTQVVDRQAAAAAAAAAAAAAAAAAAAAAAAAAKAAAAAAAAAAAAAAAAAAsAAABpbnZhbGlkIHVybOgDAQIAAABlDeYSX9s0rnt0tCP3wqtelERLeTcT+pkHWquHHRj7NwFiDUmu+U8sXOOZQXL36xmknL+Zzd/z3uw2G0ERMo8Eth4BAgABAf8BAAEBoivvbAzLh2kD2cSu6IQIqGQDGeoh/UEDizyp6mLT1tUA",
          "base64"
        ],
        "executable": false,
        "lamports": 5616720,
        "owner": "metaqbxxUerdq28cj1RbAWkYQm3ybzjb6a8bt518x1s",
        "rentEpoch": 361
      }
    },
    "id": 1
  })";
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata(mojom::kSolanaMainnet,
                          "5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
}

TEST_F(NftMetadataFetcherUnitTest, DecodeMetadataUri) {
  // Valid borsh encoding and URI yields expected URI
  std::vector<uint8_t> uri_borsh_encoded = {
      4, 101, 13, 230, 18, 95, 219, 52, 174, 123, 116, 180, 35, 247, 194, 171,
      94, 148, 68, 75, 121, 55, 19, 250, 153, 7, 90, 171, 135, 29, 24, 251, 55,
      67, 195, 198, 253, 30, 138, 21, 68, 160, 113, 238, 252, 55, 9, 61, 22,
      187, 228, 119, 214, 204, 110, 244, 200, 40, 36, 82, 15, 47, 60, 157, 148,
      32, 0, 0, 0, 83, 80, 69, 67, 73, 65, 76, 32, 83, 65, 85, 67, 69, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0,  // the next four bytes encode length of the URI string
                         // (200)
      200, 0, 0, 0, 104, 116, 116, 112, 115, 58, 47, 47, 98, 97, 102, 107, 114,
      101, 105, 102, 52, 119, 120, 53, 52, 119, 106, 114, 55, 112, 103, 102,
      117, 103, 51, 119, 108, 97, 116, 114, 51, 110, 102, 110, 116, 115, 102,
      119, 110, 103, 118, 54, 101, 117, 115, 101, 98, 98, 113, 117, 101, 122,
      114, 120, 101, 110, 106, 54, 99, 107, 52, 46, 105, 112, 102, 115, 46, 100,
      119, 101, 98, 46, 108, 105, 110, 107, 63, 101, 120, 116, 61, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 232, 3, 0, 0, 1, 1, 255, 1, 0, 1, 1, 162, 43,
      239, 108, 12, 203, 135, 105, 3, 217, 196, 174, 232, 132, 8, 168, 100, 3,
      25, 234, 33, 253, 65, 3, 139, 60, 169, 234, 98, 211, 214, 213, 0};
  auto uri = nft_metadata_fetcher_->DecodeMetadataUri(uri_borsh_encoded);
  ASSERT_TRUE(uri);
  EXPECT_EQ(uri.value().spec(),
            "https://"
            "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
            "dweb.link/?ext=");

  // Iterate over each possible prefix of the uri_borsh_encoded and call
  // DecodeMetadataUri on each prefix and verify that DecodeMetadataUri returns
  // nullopt
  size_t position_of_last_uri_byte =
      /* metadata.key*/ 1 + /* metadata.update_authority */ 32 +
      /* metadata.mint */ 32 + /* metadata.name length */ 4 +
      /* metadata.name value */ 32 + /* metadata.symbol length */ 4 +
      /* metadata.symbol value */ 10 + /* metadata.uri.length*/ 4 +
      /* metadata.uri value */ 200;
  for (size_t i = 0; i <= position_of_last_uri_byte; i++) {
    std::vector<uint8_t> uri_borsh_encoded_prefix(
        uri_borsh_encoded.begin(), uri_borsh_encoded.begin() + i);
    uri = nft_metadata_fetcher_->DecodeMetadataUri(uri_borsh_encoded_prefix);
    EXPECT_FALSE(uri);
  }

  // Invalid borsh encoding due to incorrect claimed length of metadata URI
  // string (too large) fails to decode (out of bounds check)
  uri_borsh_encoded = {
      4, 101, 13, 230, 18, 95, 219, 52, 174, 123, 116, 180, 35, 247, 194, 171,
      94, 148, 68, 75, 121, 55, 19, 250, 153, 7, 90, 171, 135, 29, 24, 251, 55,
      67, 195, 198, 253, 30, 138, 21, 68, 160, 113, 238, 252, 55, 9, 61, 22,
      187, 228, 119, 214, 204, 110, 244, 200, 40, 36, 82, 15, 47, 60, 157, 148,
      32, 0, 0, 0, 83, 80, 69, 67, 73, 65, 76, 32, 83, 65, 85, 67, 69, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0,
      // next four bytes encode the URI of the string, which have been overrided
      // to be incorrect (too large)
      255, 255, 255, 0, 104, 116, 116, 112, 115, 58, 47, 47, 98, 97, 102, 107,
      114, 101, 105, 102, 52, 119, 120, 53, 52, 119, 106, 114, 55, 112, 103,
      102, 117, 103, 51, 119, 108, 97, 116, 114, 51, 110, 102, 110, 116, 115,
      102, 119, 110, 103, 118, 54, 101, 117, 115, 101, 98, 98, 113, 117, 101,
      122, 114, 120, 101, 110, 106, 54, 99, 107, 52, 46, 105, 112, 102, 115, 46,
      100, 119, 101, 98, 46, 108, 105, 110, 107, 63, 101, 120, 116, 61, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 232, 3, 0, 0, 1, 1, 255, 1, 0, 1, 1, 162,
      43, 239, 108, 12, 203, 135, 105, 3, 217, 196, 174, 232, 132, 8, 168, 100,
      3, 25, 234, 33, 253, 65, 3, 139, 60, 169, 234, 98, 211, 214, 213, 0};
  uri = nft_metadata_fetcher_->DecodeMetadataUri(uri_borsh_encoded);
  ASSERT_FALSE(uri);

  // Valid borsh encoding, but invalid URI is parsed but yields empty URI
  auto uri_borsh_encoded2 = base::Base64Decode(
      "BGUN5hJf2zSue3S0I/fCq16UREt5NxP6mQdaq4cdGPs3Q8PG/"
      "R6KFUSgce78Nwk9Frvkd9bMbvTIKCRSDy88nZQgAAAAU1BFQ0lBTCBTQVVDRQAAAAAAAAAAA"
      "AAAAAAAAAAAAAAKAAAAAAAAAAAAAAAAAAsAAABpbnZhbGlkIHVybOgDAQIAAABlDeYSX9s0r"
      "nt0tCP3wqtelERLeTcT+pkHWquHHRj7NwFiDUmu+U8sXOOZQXL36xmknL+Zzd/"
      "z3uw2G0ERMo8Eth4BAgABAf8BAAEBoivvbAzLh2kD2cSu6IQIqGQDGeoh/"
      "UEDizyp6mLT1tUA");
  ASSERT_TRUE(uri_borsh_encoded2);
  uri = nft_metadata_fetcher_->DecodeMetadataUri(*uri_borsh_encoded2);
  ASSERT_TRUE(uri);
  EXPECT_EQ(uri.value().spec(), "");

  // Invalid borsh encoding is not parsed
  uri_borsh_encoded2 = base::Base64Decode("d2hvb3BzIQ==");
  ASSERT_TRUE(uri_borsh_encoded2);
  ASSERT_FALSE(nft_metadata_fetcher_->DecodeMetadataUri(*uri_borsh_encoded2));
}

}  // namespace brave_wallet
