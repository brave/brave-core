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
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

namespace {

void TestValueToERCTokenFailCases(const base::Value& value,
                                  const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    auto invalid_value = value.Clone();
    invalid_value.RemoveKey(key);
    EXPECT_FALSE(ValueToERCToken(invalid_value))
        << "ValueToERCToken should fail if " << key << " not exists";
  }
}

}  // namespace

TEST(ValueConversionUtilsUnitTest, ValueToEthereumChainTest) {
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"({
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
    EXPECT_TRUE(chain->is_eip1559);
  }
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"({
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
    ASSERT_FALSE(chain->is_eip1559);
    EXPECT_EQ(chain->decimals, 0);
  }

  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"({
    })")
                                               .value());
    ASSERT_FALSE(chain);
  }
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"([
          ])")
                                               .value());
    ASSERT_FALSE(chain);
  }
}

TEST(ValueConversionUtilsUnitTest, EthereumChainToValueTest) {
  brave_wallet::mojom::EthereumChain chain(
      "chain_id", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11, true);
  base::Value value = brave_wallet::EthereumChainToValue(chain.Clone());
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

  auto result = brave_wallet::ValueToEthereumChain(value);
  ASSERT_TRUE(result->Equals(chain));
}

TEST(ValueConversionUtilsUnitTest, ValueToERCToken) {
  absl::optional<base::Value> json_value = base::JSONReader::Read(R"({
      "contract_address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
      "name": "Basic Attention Token",
      "symbol": "BAT",
      "logo": "bat.png",
      "is_erc20": true,
      "is_erc721": false,
      "decimals": 18,
      "visible": true,
      "token_id": ""
  })");
  ASSERT_TRUE(json_value);

  mojom::ERCTokenPtr expected_token = mojom::ERCToken::New(
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "Basic Attention Token",
      "bat.png", true, false, "BAT", 18, true, "");

  mojom::ERCTokenPtr token = ValueToERCToken(json_value.value());
  EXPECT_EQ(token, expected_token);

  // Test input value with required keys.
  TestValueToERCTokenFailCases(
      json_value.value(), {"contract_address", "name", "symbol", "is_erc20",
                           "is_erc721", "decimals", "visible"});

  // Test input value with optional keys.
  base::Value optional_value = json_value.value().Clone();
  optional_value.RemoveKey("logo");
  optional_value.RemoveKey("token_id");
  expected_token->logo = "";
  token = ValueToERCToken(optional_value);
  EXPECT_EQ(token, expected_token);
}

}  // namespace brave_wallet
