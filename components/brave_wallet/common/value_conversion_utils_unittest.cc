/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/value_conversion_utils.h"

#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

void TestValueToBlockchainTokenFailCases(const base::Value& value,
                                         const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    auto invalid_value = value.Clone();
    invalid_value.RemoveKey(key);
    EXPECT_FALSE(ValueToBlockchainToken(invalid_value))
        << "ValueToBlockchainToken should fail if " << key << " not exists";
  }
}

}  // namespace

TEST(ValueConversionUtilsUnitTest, ValueToEthNetworkInfoTest) {
  {
    mojom::NetworkInfoPtr chain =
        brave_wallet::ValueToEthNetworkInfo(base::JSONReader::Read(R"({
      "chainId": "0x5",
      "chainName": "Goerli",
      "rpcUrls": [
        "https://goerli.infura.io/v3/INSERT_API_KEY_HERE",
        "https://second.infura.io/"
      ],
      "iconUrls": [
        "https://xdaichain.com/fake/example/url/xdai.svg",
        "https://xdaichain.com/fake/example/url/xdai.png"
      ],
      "nativeCurrency": {
        "name": "Goerli ETH",
        "symbol": "gorETH",
        "decimals": 18
      },
      "blockExplorerUrls": ["https://goerli.etherscan.io"],
      "is_eip1559": true
    })")
                                                .value());
    ASSERT_TRUE(chain);
    EXPECT_EQ("0x5", chain->chain_id);
    EXPECT_EQ("Goerli", chain->chain_name);
    EXPECT_EQ(size_t(2), chain->rpc_urls.size());
    EXPECT_EQ("https://goerli.infura.io/v3/INSERT_API_KEY_HERE",
              chain->rpc_urls.front());
    EXPECT_EQ("https://second.infura.io/", chain->rpc_urls.back());
    EXPECT_EQ("https://goerli.etherscan.io",
              chain->block_explorer_urls.front());
    EXPECT_EQ("Goerli ETH", chain->symbol_name);
    EXPECT_EQ("gorETH", chain->symbol);
    EXPECT_EQ(18, chain->decimals);
    EXPECT_EQ("https://xdaichain.com/fake/example/url/xdai.svg",
              chain->icon_urls.front());
    EXPECT_EQ("https://xdaichain.com/fake/example/url/xdai.png",
              chain->icon_urls.back());
    ASSERT_EQ(chain->coin, mojom::CoinType::ETH);
    ASSERT_TRUE(chain->data);
    ASSERT_TRUE(chain->data->is_eth_data());
    EXPECT_TRUE(chain->data->get_eth_data()->is_eip1559);
  }
  {
    mojom::NetworkInfoPtr chain =
        brave_wallet::ValueToEthNetworkInfo(base::JSONReader::Read(R"({
      "chainId": "0x5"
    })")
                                                .value());
    ASSERT_TRUE(chain);
    EXPECT_EQ("0x5", chain->chain_id);
    ASSERT_TRUE(chain->chain_name.empty());
    ASSERT_TRUE(chain->rpc_urls.empty());
    ASSERT_TRUE(chain->icon_urls.empty());
    ASSERT_TRUE(chain->block_explorer_urls.empty());
    ASSERT_TRUE(chain->symbol_name.empty());
    ASSERT_TRUE(chain->symbol.empty());
    ASSERT_EQ(chain->coin, mojom::CoinType::ETH);
    ASSERT_FALSE(chain->data);
    EXPECT_EQ(chain->decimals, 0);
  }

  {
    mojom::NetworkInfoPtr chain =
        brave_wallet::ValueToEthNetworkInfo(base::JSONReader::Read(R"({
    })")
                                                .value());
    ASSERT_FALSE(chain);
  }
  {
    mojom::NetworkInfoPtr chain =
        brave_wallet::ValueToEthNetworkInfo(base::JSONReader::Read(R"([
          ])")
                                                .value());
    ASSERT_FALSE(chain);
  }
}

TEST(ValueConversionUtilsUnitTest, EthNetworkInfoToValueTest) {
  mojom::NetworkInfo chain(
      "chain_id", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11, mojom::CoinType::ETH,
      mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true)));
  base::Value value = brave_wallet::EthNetworkInfoToValue(chain.Clone());
  EXPECT_EQ(*value.FindStringKey("chainId"), chain.chain_id);
  EXPECT_EQ(*value.FindStringKey("chainName"), chain.chain_name);
  EXPECT_EQ(*value.FindStringPath("nativeCurrency.name"), chain.symbol_name);
  EXPECT_EQ(*value.FindStringPath("nativeCurrency.symbol"), chain.symbol);
  EXPECT_EQ(*value.FindIntPath("nativeCurrency.decimals"), chain.decimals);
  EXPECT_EQ(value.FindBoolKey("is_eip1559").value(), true);
  for (const auto& entry : value.FindListKey("rpcUrls")->GetList()) {
    ASSERT_NE(std::find(chain.rpc_urls.begin(), chain.rpc_urls.end(),
                        entry.GetString()),
              chain.rpc_urls.end());
  }

  for (const auto& entry : value.FindListKey("iconUrls")->GetList()) {
    ASSERT_NE(std::find(chain.icon_urls.begin(), chain.icon_urls.end(),
                        entry.GetString()),
              chain.icon_urls.end());
  }
  auto* blocked_urls = value.FindListKey("blockExplorerUrls");
  for (const auto& entry : blocked_urls->GetList()) {
    ASSERT_NE(std::find(chain.block_explorer_urls.begin(),
                        chain.block_explorer_urls.end(), entry.GetString()),
              chain.block_explorer_urls.end());
  }

  auto result = ValueToEthNetworkInfo(value);
  ASSERT_TRUE(result->Equals(chain));
}

TEST(ValueConversionUtilsUnitTest, ValueToBlockchainToken) {
  absl::optional<base::Value> json_value = base::JSONReader::Read(R"({
      "contract_address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
      "name": "Basic Attention Token",
      "symbol": "BAT",
      "logo": "bat.png",
      "is_erc20": true,
      "is_erc721": false,
      "decimals": 18,
      "visible": true,
      "token_id": "",
      "coingecko_id": ""
  })");
  ASSERT_TRUE(json_value);

  mojom::BlockchainTokenPtr expected_token = mojom::BlockchainToken::New(
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "Basic Attention Token",
      "bat.png", true, false, "BAT", 18, true, "", "");

  mojom::BlockchainTokenPtr token = ValueToBlockchainToken(json_value.value());
  EXPECT_EQ(token, expected_token);

  // Test input value with required keys.
  TestValueToBlockchainTokenFailCases(
      json_value.value(), {"contract_address", "name", "symbol", "is_erc20",
                           "is_erc721", "decimals", "visible"});

  // Test input value with optional keys.
  base::Value optional_value = json_value.value().Clone();
  optional_value.RemoveKey("logo");
  optional_value.RemoveKey("token_id");
  optional_value.RemoveKey("coingecko_id");
  expected_token->logo = "";
  token = ValueToBlockchainToken(optional_value);
  EXPECT_EQ(token, expected_token);
}

TEST(ValueConversionUtilsUnitTest, PermissionRequestResponseToValue) {
  url::Origin origin = url::Origin::Create(GURL("https://brave.com"));
  std::vector<std::string> accounts{
      "0xA99D71De40D67394eBe68e4D0265cA6C9D421029"};
  base::Value value =
      PermissionRequestResponseToValue(origin.Serialize(), accounts);

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

  base::ListValue* list_value;
  ASSERT_TRUE(value.GetAsList(&list_value));
  ASSERT_EQ(list_value->GetList().size(), 1UL);

  base::Value& param0 = list_value->GetList()[0];
  base::Value* caveats = param0.FindListPath("caveats");
  ASSERT_NE(caveats, nullptr);
  ASSERT_EQ(caveats->GetList().size(), 2UL);

  base::Value& caveats0 = caveats->GetList()[0];
  std::string* name = caveats0.FindStringKey("name");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(*name, "primaryAccountOnly");
  std::string* type = caveats0.FindStringKey("type");
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(*type, "limitResponseLength");
  absl::optional<int> primary_accounts_only_value =
      caveats0.FindIntKey("value");
  ASSERT_NE(primary_accounts_only_value, absl::nullopt);
  EXPECT_EQ(*primary_accounts_only_value, 1);

  base::Value& caveats1 = caveats->GetList()[1];
  name = caveats1.FindStringKey("name");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(*name, "exposedAccounts");
  type = caveats1.FindStringKey("type");
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(*type, "filterResponse");
  base::Value* exposed_accounts = caveats1.FindListKey("value");
  ASSERT_NE(exposed_accounts, nullptr);
  ASSERT_EQ(exposed_accounts->GetList().size(), 1UL);
  EXPECT_EQ(exposed_accounts->GetList()[0],
            base::Value("0xA99D71De40D67394eBe68e4D0265cA6C9D421029"));

  base::Value* context = param0.FindListPath("context");
  ASSERT_NE(context, nullptr);
  ASSERT_EQ(context->GetList().size(), 1UL);
  EXPECT_EQ(context->GetList()[0],
            base::Value("https://github.com/MetaMask/rpc-cap"));

  absl::optional<double> date = param0.FindDoubleKey("date");
  ASSERT_NE(date, absl::nullopt);

  std::string* id = param0.FindStringKey("id");
  ASSERT_NE(id, nullptr);

  std::string* invoker = param0.FindStringKey("invoker");
  ASSERT_NE(invoker, nullptr);
  EXPECT_EQ(*invoker, "https://brave.com");

  std::string* parent_capability = param0.FindStringKey("parentCapability");
  ASSERT_NE(parent_capability, nullptr);
  EXPECT_EQ(*parent_capability, "eth_accounts");
}

}  // namespace brave_wallet
