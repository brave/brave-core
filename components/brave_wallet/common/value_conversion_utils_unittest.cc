/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/value_conversion_utils.h"

#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

using ::testing::ElementsAreArray;

namespace brave_wallet {

namespace {

void TestValueToBlockchainTokenFailCases(const base::Value::Dict& value,
                                         const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    auto invalid_value = value.Clone();
    invalid_value.Remove(key);
    EXPECT_FALSE(
        ValueToBlockchainToken(invalid_value, "0x1", mojom::CoinType::ETH))
        << "ValueToBlockchainToken should fail if " << key << " not exists";
  }
}

constexpr char kNetworkDataValue[] = R"({
      "chainId": "0x5",
      "chainName": "Goerli",
      "activeRpcEndpointIndex": 3,
      "rpcUrls": [
        "ftp://bar/",
        "ftp://localhost/",
        "http://bar/",
        "http://localhost/",
        "http://127.0.0.1/",
        "https://goerli.infura.io/v3/INSERT_API_KEY_HERE",
        "https://second.infura.io/",
        []
      ],
      "iconUrls": [
        "ftp://bar/",
        "ftp://localhost/",
        "http://bar/",
        "http://localhost/",
        "http://127.0.0.1/",
        "https://xdaichain.com/fake/example/url/xdai.svg",
        "https://xdaichain.com/fake/example/url/xdai.png",
        {}
      ],
      "nativeCurrency": {
        "name": "Goerli ETH",
        "symbol": "gorETH",
        "decimals": 18
      },
      "blockExplorerUrls": [
        "ftp://bar/",
        "ftp://localhost/",
        "http://bar/",
        "http://localhost/",
        "http://127.0.0.1/",
        "https://goerli.etherscan.io",
        2
      ],
      "is_eip1559": true
    })";

}  // namespace

TEST(ValueConversionUtilsUnitTest, ParseEip3085Payload) {
  {
    auto value = base::test::ParseJson(kNetworkDataValue);
    mojom::NetworkInfoPtr chain = ParseEip3085Payload(value);
    ASSERT_TRUE(chain);
    EXPECT_EQ("0x5", chain->chain_id);
    EXPECT_EQ("Goerli", chain->chain_name);
    EXPECT_EQ(0, chain->active_rpc_endpoint_index);
    EXPECT_EQ(chain->rpc_endpoints,
              std::vector<GURL>(
                  {GURL("http://localhost/"), GURL("http://127.0.0.1/"),
                   GURL("https://goerli.infura.io/v3/INSERT_API_KEY_HERE"),
                   GURL("https://second.infura.io/")}));
    EXPECT_EQ(
        chain->block_explorer_urls,
        std::vector<std::string>({"http://localhost/", "http://127.0.0.1/",
                                  "https://goerli.etherscan.io"}));
    EXPECT_EQ("Goerli ETH", chain->symbol_name);
    EXPECT_EQ("gorETH", chain->symbol);
    EXPECT_EQ(18, chain->decimals);
    EXPECT_EQ(chain->icon_urls,
              std::vector<std::string>(
                  {"http://localhost/", "http://127.0.0.1/",
                   "https://xdaichain.com/fake/example/url/xdai.svg",
                   "https://xdaichain.com/fake/example/url/xdai.png"}));

    EXPECT_EQ(chain->coin, mojom::CoinType::ETH);
    EXPECT_FALSE(chain->is_eip1559);
  }
  {
    mojom::NetworkInfoPtr chain = ParseEip3085Payload(base::test::ParseJson(R"({
      "chainId": "0x5"
    })"));
    ASSERT_TRUE(chain);
    EXPECT_EQ("0x5", chain->chain_id);
    ASSERT_TRUE(chain->chain_name.empty());
    ASSERT_EQ(0, chain->active_rpc_endpoint_index);
    ASSERT_TRUE(chain->rpc_endpoints.empty());
    ASSERT_TRUE(chain->icon_urls.empty());
    ASSERT_TRUE(chain->block_explorer_urls.empty());
    ASSERT_TRUE(chain->symbol_name.empty());
    ASSERT_TRUE(chain->symbol.empty());
    ASSERT_EQ(chain->coin, mojom::CoinType::ETH);
    ASSERT_FALSE(chain->is_eip1559);
    EXPECT_EQ(chain->decimals, 0);
  }

  {
    mojom::NetworkInfoPtr chain =
        ParseEip3085Payload(base::test::ParseJson(R"({})"));
    ASSERT_FALSE(chain);
  }
  {
    mojom::NetworkInfoPtr chain =
        ParseEip3085Payload(base::test::ParseJson(R"([])"));
    ASSERT_FALSE(chain);
  }
}

TEST(ValueConversionUtilsUnitTest, ValueToNetworkInfoTest) {
  {
    auto value = base::test::ParseJson(kNetworkDataValue);
    mojom::NetworkInfoPtr chain = ValueToNetworkInfo(value);
    ASSERT_TRUE(chain);
    EXPECT_EQ("0x5", chain->chain_id);
    EXPECT_EQ("Goerli", chain->chain_name);
    EXPECT_EQ(3, chain->active_rpc_endpoint_index);
    EXPECT_EQ(
        chain->rpc_endpoints,
        std::vector<GURL>(
            {GURL("ftp://bar/"), GURL("ftp://localhost/"), GURL("http://bar/"),
             GURL("http://localhost/"), GURL("http://127.0.0.1/"),
             GURL("https://goerli.infura.io/v3/INSERT_API_KEY_HERE"),
             GURL("https://second.infura.io/")}));
    EXPECT_EQ(chain->block_explorer_urls,
              std::vector<std::string>({"ftp://bar/", "ftp://localhost/",
                                        "http://bar/", "http://localhost/",
                                        "http://127.0.0.1/",
                                        "https://goerli.etherscan.io"}));
    EXPECT_EQ("Goerli ETH", chain->symbol_name);
    EXPECT_EQ("gorETH", chain->symbol);
    EXPECT_EQ(18, chain->decimals);
    EXPECT_EQ(chain->icon_urls,
              std::vector<std::string>(
                  {"ftp://bar/", "ftp://localhost/", "http://bar/",
                   "http://localhost/", "http://127.0.0.1/",
                   "https://xdaichain.com/fake/example/url/xdai.svg",
                   "https://xdaichain.com/fake/example/url/xdai.png"}));
    EXPECT_EQ(chain->coin, mojom::CoinType::ETH);
    EXPECT_THAT(chain->supported_keyrings,
                ElementsAreArray({mojom::KeyringId::kDefault}));
    EXPECT_TRUE(chain->is_eip1559);
  }
  {
    mojom::NetworkInfoPtr chain =
        ValueToNetworkInfo(base::test::ParseJson(R"({"chainId": "0x5" })"));
    ASSERT_TRUE(chain);
    EXPECT_EQ("0x5", chain->chain_id);
    ASSERT_TRUE(chain->chain_name.empty());
    ASSERT_EQ(0, chain->active_rpc_endpoint_index);
    ASSERT_TRUE(chain->rpc_endpoints.empty());
    ASSERT_TRUE(chain->icon_urls.empty());
    ASSERT_TRUE(chain->block_explorer_urls.empty());
    ASSERT_TRUE(chain->symbol_name.empty());
    ASSERT_TRUE(chain->symbol.empty());
    ASSERT_EQ(chain->coin, mojom::CoinType::ETH);
    EXPECT_THAT(chain->supported_keyrings,
                ElementsAreArray({mojom::KeyringId::kDefault}));
    ASSERT_FALSE(chain->is_eip1559);
    EXPECT_EQ(chain->decimals, 0);
  }

  {
    mojom::NetworkInfoPtr chain =
        ValueToNetworkInfo(base::test::ParseJson(R"({})"));
    ASSERT_FALSE(chain);
  }
  {
    mojom::NetworkInfoPtr chain =
        ValueToNetworkInfo(base::test::ParseJson(R"([])"));
    ASSERT_FALSE(chain);
  }
}

TEST(ValueConversionUtilsUnitTest, NetworkInfoToValueTest) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1();
  base::Value::Dict value = NetworkInfoToValue(chain);
  EXPECT_EQ(*value.FindString("chainId"), chain.chain_id);
  EXPECT_EQ(*value.FindString("chainName"), chain.chain_name);
  EXPECT_EQ(*value.FindStringByDottedPath("nativeCurrency.name"),
            chain.symbol_name);
  EXPECT_EQ(*value.FindStringByDottedPath("nativeCurrency.symbol"),
            chain.symbol);
  EXPECT_EQ(*value.FindIntByDottedPath("nativeCurrency.decimals"),
            chain.decimals);
  EXPECT_EQ(value.FindBool("is_eip1559").value(), false);
  auto* rpc_urls = value.FindList("rpcUrls");
  for (const auto& entry : *rpc_urls) {
    ASSERT_TRUE(base::Contains(chain.rpc_endpoints, entry.GetString()));
  }

  for (const auto& entry : *value.FindList("iconUrls")) {
    ASSERT_TRUE(base::Contains(chain.icon_urls, entry.GetString()));
  }
  auto* blocked_urls = value.FindList("blockExplorerUrls");
  for (const auto& entry : *blocked_urls) {
    ASSERT_TRUE(base::Contains(chain.block_explorer_urls, entry.GetString()));
  }

  auto result = ValueToNetworkInfo(base::Value(value.Clone()));
  ASSERT_TRUE(result->Equals(chain));

  {
    mojom::NetworkInfo test_chain = GetTestNetworkInfo1();

    test_chain.coin = mojom::CoinType::ETH;
    auto eth_value = NetworkInfoToValue(test_chain);
    EXPECT_EQ(eth_value.FindInt("coin"),
              static_cast<int>(mojom::CoinType::ETH));
    EXPECT_TRUE(eth_value.FindBool("is_eip1559"));

    test_chain.coin = mojom::CoinType::FIL;
    auto fil_value = NetworkInfoToValue(test_chain);
    EXPECT_EQ(fil_value.FindInt("coin"),
              static_cast<int>(mojom::CoinType::FIL));
    EXPECT_FALSE(fil_value.FindBool("is_eip1559"));

    test_chain.coin = mojom::CoinType::SOL;
    auto sol_value = NetworkInfoToValue(test_chain);
    EXPECT_EQ(sol_value.FindInt("coin"),
              static_cast<int>(mojom::CoinType::SOL));
    EXPECT_FALSE(sol_value.FindBool("is_eip1559"));

    test_chain.coin = mojom::CoinType::BTC;
    auto btc_value = NetworkInfoToValue(test_chain);
    EXPECT_EQ(btc_value.FindInt("coin"),
              static_cast<int>(mojom::CoinType::BTC));
    EXPECT_FALSE(btc_value.FindBool("is_eip1559"));

    EXPECT_TRUE(AllCoinsTested());
  }

  {
    auto data_value = base::test::ParseJson(kNetworkDataValue);
    auto value_network = ValueToNetworkInfo(data_value);
    EXPECT_EQ(value_network->coin, mojom::CoinType::ETH);
    EXPECT_THAT(value_network->supported_keyrings,
                ElementsAreArray({mojom::KeyringId::kDefault}));

    data_value.GetDict().Set("coin", static_cast<int>(mojom::CoinType::ETH));
    value_network = ValueToNetworkInfo(data_value);
    EXPECT_EQ(value_network->coin, mojom::CoinType::ETH);
    EXPECT_THAT(value_network->supported_keyrings,
                ElementsAreArray({mojom::KeyringId::kDefault}));

    data_value.GetDict().Set("coin", static_cast<int>(mojom::CoinType::SOL));
    value_network = ValueToNetworkInfo(data_value);
    EXPECT_EQ(value_network->coin, mojom::CoinType::SOL);
    EXPECT_THAT(value_network->supported_keyrings,
                ElementsAreArray({mojom::KeyringId::kSolana}));

    data_value.GetDict().Set("coin", static_cast<int>(mojom::CoinType::FIL));
    value_network = ValueToNetworkInfo(data_value);
    EXPECT_EQ(value_network->coin, mojom::CoinType::FIL);
    EXPECT_THAT(value_network->supported_keyrings,
                ElementsAreArray({mojom::KeyringId::kFilecoinTestnet}));

    data_value.GetDict().Set("coin", static_cast<int>(mojom::CoinType::BTC));
    value_network = ValueToNetworkInfo(data_value);
    EXPECT_EQ(value_network->coin, mojom::CoinType::BTC);
    EXPECT_THAT(value_network->supported_keyrings,
                ElementsAreArray({mojom::KeyringId::kBitcoin84Testnet}));

    EXPECT_TRUE(AllCoinsTested());
  }
}

TEST(ValueConversionUtilsUnitTest, ValueToBlockchainToken) {
  auto json_value = base::test::ParseJsonDict(R"({
      "address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
      "name": "Basic Attention Token",
      "symbol": "BAT",
      "logo": "bat.png",
      "is_erc20": true,
      "is_erc721": false,
      "is_erc1155": false,
      "is_nft": false,
      "is_spam": false,
      "decimals": 18,
      "visible": true,
      "token_id": "",
      "coingecko_id": ""
  })");

  mojom::BlockchainTokenPtr expected_token = mojom::BlockchainToken::New(
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "Basic Attention Token",
      "bat.png", true, false, false, false, false, "BAT", 18, true, "", "",
      "0x1", mojom::CoinType::ETH);

  mojom::BlockchainTokenPtr token =
      ValueToBlockchainToken(json_value, "0x1", mojom::CoinType::ETH);
  EXPECT_EQ(token, expected_token);

  // Test input value with required keys.
  TestValueToBlockchainTokenFailCases(
      json_value, {"address", "name", "symbol", "is_erc20", "is_erc721",
                   "decimals", "visible"});

  // Test input value with optional keys.
  base::Value::Dict optional_value = json_value.Clone();
  optional_value.Remove("logo");
  optional_value.Remove("token_id");
  optional_value.Remove("coingecko_id");
  expected_token->logo = "";
  token = ValueToBlockchainToken(optional_value, "0x1", mojom::CoinType::ETH);
  EXPECT_EQ(token, expected_token);

  json_value = base::test::ParseJsonDict(R"({
      "address": "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d",
      "name": "Crypto Kitties",
      "symbol": "CK",
      "logo": "CryptoKitties-Kitty-13733.svg",
      "is_erc20": false,
      "is_erc721": true,
      "is_erc1155": false,
      "is_nft": true,
      "is_spam": true,
      "decimals": 0,
      "visible": true
  })");

  expected_token = mojom::BlockchainToken::New(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "Crypto Kitties",
      "CryptoKitties-Kitty-13733.svg", false, true, false, true, true, "CK", 0,
      true, "", "", "0x1", mojom::CoinType::ETH);

  token = ValueToBlockchainToken(json_value, "0x1", mojom::CoinType::ETH);
  EXPECT_EQ(token, expected_token);

  // Test is_erc1155 is parsed
  json_value = base::test::ParseJsonDict(R"({
      "address": "0x28472a58A490c5e09A238847F66A68a47cC76f0f",
      "name": "ADIDAS",
      "symbol": "ADIDAS",
      "logo": "adidas.png",
      "is_erc20": false,
      "is_erc721": false,
      "is_erc1155": true,
      "is_nft": true,
      "is_spam": false,
      "decimals": 0,
      "visible": true
  })");

  expected_token = mojom::BlockchainToken::New(
      "0x28472a58A490c5e09A238847F66A68a47cC76f0f", "ADIDAS", "adidas.png",
      false, false, true, true, false, "ADIDAS", 0, true, "", "", "0x1",
      mojom::CoinType::ETH);

  token = ValueToBlockchainToken(json_value, "0x1", mojom::CoinType::ETH);
  EXPECT_EQ(token, expected_token);
}

TEST(ValueConversionUtilsUnitTest, PermissionRequestResponseToValue) {
  url::Origin origin = url::Origin::Create(GURL("https://brave.com"));
  std::vector<std::string> accounts{
      "0xA99D71De40D67394eBe68e4D0265cA6C9D421029"};
  base::Value::List value = PermissionRequestResponseToValue(origin, accounts);

  // [{
  //   "caveats":[
  //     {
  //       "name":"primaryAccountOnly",
  //        "type":"limitResponseLength",
  //        "value":1
  //     }, {
  //       "name":"exposedAccounts",
  //       "type":"filterResponse",
  //       "value": ["0xA99D71De40D67394eBe68e4D0265cA6C9D421029"]
  //     }
  //   ],
  //   "context":[
  //     "https://github.com/MetaMask/rpc-cap"
  //   ],
  //   "date":1.637594791027276e+12,
  //   "id":"2485c0da-2131-4801-9918-26e8de929a29",
  //   "invoker":"https://brave.com",
  //   "parentCapability":"eth_accounts"
  // }]"

  ASSERT_EQ(value.size(), 1UL);

  auto& param0 = value[0].GetDict();
  auto* caveats = param0.FindList("caveats");
  ASSERT_NE(caveats, nullptr);
  ASSERT_EQ(caveats->size(), 2UL);

  auto& caveats0 = (*caveats)[0].GetDict();
  std::string* name = caveats0.FindString("name");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(*name, "primaryAccountOnly");
  std::string* type = caveats0.FindString("type");
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(*type, "limitResponseLength");
  absl::optional<int> primary_accounts_only_value = caveats0.FindInt("value");
  ASSERT_NE(primary_accounts_only_value, absl::nullopt);
  EXPECT_EQ(*primary_accounts_only_value, 1);

  auto& caveats1 = (*caveats)[1].GetDict();
  name = caveats1.FindString("name");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(*name, "exposedAccounts");
  type = caveats1.FindString("type");
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(*type, "filterResponse");
  auto* exposed_accounts = caveats1.FindList("value");
  ASSERT_NE(exposed_accounts, nullptr);
  ASSERT_EQ(exposed_accounts->size(), 1UL);
  EXPECT_EQ((*exposed_accounts)[0],
            base::Value("0xa99d71de40d67394ebe68e4d0265ca6c9d421029"));

  auto* context = param0.FindList("context");
  ASSERT_NE(context, nullptr);
  ASSERT_EQ(context->size(), 1UL);
  EXPECT_EQ((*context)[0], base::Value("https://github.com/MetaMask/rpc-cap"));

  absl::optional<double> date = param0.FindDouble("date");
  ASSERT_NE(date, absl::nullopt);

  std::string* id = param0.FindString("id");
  ASSERT_NE(id, nullptr);

  std::string* invoker = param0.FindString("invoker");
  ASSERT_NE(invoker, nullptr);
  EXPECT_EQ(*invoker, "https://brave.com");

  std::string* parent_capability = param0.FindString("parentCapability");
  ASSERT_NE(parent_capability, nullptr);
  EXPECT_EQ(*parent_capability, "eth_accounts");
}

TEST(ValueConversionUtilsUnitTest, GetFirstValidChainURL) {
  std::vector<GURL> urls = {
      GURL("https://goerli.infura.io/v3/${INFURA_API_KEY}"),
      GURL("https://goerli.alchemy.io/v3/${ALCHEMY_API_KEY}"),
      GURL("https://goerli.apikey.io/v3/${API_KEY}"),
      GURL("https://goerli.apikey.io/v3/${PULSECHAIN_API_KEY}"),
      GURL("wss://goerli.infura.io/v3/")};

  // Uses the first URL when a good URL is not available
  auto index = GetFirstValidChainURLIndex(urls);
  EXPECT_EQ(index, 0);

  urls.emplace_back("https://goerli.infura.io/v3/rpc");
  urls.emplace_back("https://goerli.infura.io/v3/rpc2");
  // Uses the first HTTP(S) URL without a variable when possible
  index = GetFirstValidChainURLIndex(urls);
  EXPECT_EQ(index, 5);

  // Empty URL spec list returns 0
  EXPECT_EQ(GetFirstValidChainURLIndex(std::vector<GURL>()), 0);
}

}  // namespace brave_wallet
