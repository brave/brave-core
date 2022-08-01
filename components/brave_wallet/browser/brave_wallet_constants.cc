/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"

namespace brave_wallet {

const std::vector<mojom::BlockchainToken>& GetWyreBuyTokens() {
  static base::NoDestructor<std::vector<mojom::BlockchainToken>> tokens(
      {{"", "Avalanche", "", false, false, "AVAX", 18, true, "", "",
        mojom::kAvalancheMainnetChainId, mojom::CoinType::ETH},
       {"0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "Basic Attention Token",
        "bat.png", true, false, "BAT", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x4829043Fc625FdA139523b2ceB99D95354e2b359", "Avalanche C-Chain",
        "avax.png", true, false, "AVAXC", 18, true, "", "",
        mojom::kAvalancheMainnetChainId, mojom::CoinType::ETH},
       {"", "Ethereum", "", false, false, "ETH", 18, true, "", "",
        mojom::kMainnetChainId, mojom::CoinType::ETH},
       {"0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48", "USD Coin", "usdc.png",
        true, false, "USDC", 6, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x6B175474E89094C44Da98b954EedeAC495271d0F", "DAI", "dai.png", true,
        false, "DAI", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x7Fc66500c84A76Ad7e9c93437bFc5Ac33E2DDaE9", "AAVE", "AAVE.png", true,
        false, "AAVE", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x4Fabb145d64652a948d72533023f6E7A623C7C53", "Binance USD", "busd.png",
        true, false, "BUSD", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0xc00e94Cb662C3520282E6f5717214004A7f26888", "Compound", "comp.png",
        true, false, "Comp", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0xD533a949740bb3306d119CC777fa900bA034cd52", "Curve", "curve.png",
        true, false, "CRV", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0xbf0f3ccb8fa385a287106fba22e6bb722f94d686", "Digital USD", "", true,
        false, "ZUSD", 6, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x056Fd409E1d7A124BD7017459dFEa2F387b6d5Cd", "Gemini Dollar",
        "gusd.png", true, false, "GUSD", 2, true, "", "",
        mojom::kMainnetChainId, mojom::CoinType::ETH},
       {"0xC08512927D12348F6620a698105e1BAac6EcD911", "Digital JPY", "", true,
        false, "GYEN", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x514910771AF9Ca656af840dff83E8264EcF986CA", "Chainlink",
        "chainlink.png", true, false, "LINK", 18, true, "", "",
        mojom::kMainnetChainId, mojom::CoinType::ETH},
       {"0x9f8F72aA9304c8B593d555F12eF6589cC3A579A2", "Maker", "mkr.png", true,
        false, "MKR", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0xC011a73ee8576Fb46F5E1c5751cA3B9Fe0af2a6F", "Synthetix",
        "synthetix.png", true, false, "SNX", 18, true, "", "",
        mojom::kMainnetChainId, mojom::CoinType::ETH},
       {"0x04Fa0d235C4abf4BcF4787aF4CF447DE572eF828", "UMA", "UMA.png", true,
        false, "UMA", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984", "Uniswap", "uni.png",
        true, false, "UNI", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174", "USD Coin", "usdc.png",
        true, false, "USDC", 6, true, "", "", mojom::kPolygonMainnetChainId,
        mojom::CoinType::ETH},
       {"0xB97EF9Ef8734C71904D8002F8b6Bc66Dd9c48a6E", "USD Coin", "usdc.png",
        true, false, "USDC", 6, true, "", "", mojom::kAvalancheMainnetChainId,
        mojom::CoinType::ETH},
       {"0x8e870d67f660d95d5be530380d0ec0bd388289e1", "Pax Dollar", "usdp.png",
        true, false, "PAX", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"", "Polygon", "", false, false, "MATIC", 18, true, "", "",
        mojom::kPolygonMainnetChainId, mojom::CoinType::ETH},
       {"0xA4Bdb11dc0a2bEC88d24A3aa1E6Bb17201112eBe", "Stably Dollar",
        "usds.png", true, false, "USDS", 6, true, "", "",
        mojom::kMainnetChainId, mojom::CoinType::ETH},
       {"0xdAC17F958D2ee523a2206206994597C13D831ec7", "Tether", "usdt.png",
        true, false, "USDT", 6, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x2260FAC5E5542a773Aa44fBCfeDf7C193bc2C599", "Wrapped Bitcoin",
        "wbtc.png", true, false, "WBTC", 8, true, "", "",
        mojom::kMainnetChainId, mojom::CoinType::ETH},
       {"0x0bc529c00C6401aEF6D220BE8C6Ea1667F6Ad93e", "Yearn.Finance",
        "yfi.png", true, false, "YFI", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x9043d4d51C9d2e31e3F169de4551E416970c27Ef", "Palm DAI", "pdai.png",
        true, false, "PDAI", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x03ab458634910AaD20eF5f1C8ee96F1D6ac54919", "Rai Reflex Index",
        "rai.png", true, false, "RAI", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH}});
  return *tokens;
}

const std::vector<mojom::BlockchainToken>& GetRampBuyTokens() {
  static base::NoDestructor<std::vector<mojom::BlockchainToken>> tokens(
      {{"", "Ethereum", "", false, false, "ETH", 18, true, "", "",
        mojom::kMainnetChainId, mojom::CoinType::ETH},
       {"", "BNB", "", true, false, "BNB", 18, true, "", "",
        mojom::kBinanceSmartChainMainnetChainId, mojom::CoinType::ETH},
       {"", "Avalanche", "", false, false, "AVAX", 18, true, "", "",
        mojom::kAvalancheMainnetChainId, mojom::CoinType::ETH},
       {"0x82030cdbd9e4b7c5bb0b811a61da6360d69449cc", "RealFevr", "", true,
        false, "FEVR", 18, true, "", "",
        mojom::kBinanceSmartChainMainnetChainId, mojom::CoinType::ETH},
       {"", "Filecoin", "", false, false, "FIL", 18, true, "", "",
        mojom::kFilecoinMainnet, mojom::CoinType::FIL},
       {"", "Celo", "", false, false, "CELO", 18, true, "", "",
        mojom::kCeloMainnetChainId, mojom::CoinType::ETH},
       {"0xD8763CBa276a3738E6DE85b4b3bF5FDed6D6cA73", "Celo Euro", "ceur.png",
        true, false, "CEUR", 18, true, "", "", mojom::kCeloMainnetChainId,
        mojom::CoinType::ETH},
       {"0x765DE816845861e75A25fCA122bb6898B8B1282a", "Celo Dollar", "cusd.png",
        true, false, "CUSD", 18, true, "", "", mojom::kCeloMainnetChainId,
        mojom::CoinType::ETH},
       {"0x6b175474e89094c44da98b954eedeac495271d0f", "DAI Stablecoin",
        "dai.png", true, false, "DAI", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"", "Ethereum", "", false, false, "ETH", 18, true, "", "",
        mojom::kOptimismMainnetChainId, mojom::CoinType::ETH},
       {"", "Polygon", "", false, false, "MATIC", 18, true, "", "",
        mojom::kPolygonMainnetChainId, mojom::CoinType::ETH},
       {"0x8f3cf7ad23cd3cadbd9735aff958023239c6a063", "DAI Stablecoin",
        "dai.png", true, false, "DAI", 18, true, "", "",
        mojom::kPolygonMainnetChainId, mojom::CoinType::ETH},
       {"0x7ceB23fD6bC0adD59E62ac25578270cFf1b9f619", "Ethereum", "eth.png",
        true, false, "ETH", 18, true, "", "", mojom::kPolygonMainnetChainId,
        mojom::CoinType::ETH},
       {"0xbbba073c31bf03b8acf7c28ef0738decf3695683", "Sandbox", "sand.png",
        true, false, "SAND", 18, true, "", "", mojom::kPolygonMainnetChainId,
        mojom::CoinType::ETH},
       {"0x0f5d2fb29fb7d3cfee444a200298f468908cc942", "Decentraland",
        "mana.png", true, false, "MANA", 18, true, "", "",
        mojom::kMainnetChainId, mojom::CoinType::ETH},
       {"0xa1c57f48f0deb89f569dfbe6e2b7f46d33606fd4", "Decentraland",
        "mana.png", true, false, "MANA", 18, true, "", "",
        mojom::kPolygonMainnetChainId, mojom::CoinType::ETH},
       {"0x2791bca1f2de4661ed88a30c99a7a9449aa84174", "USD Coin", "usdc.png",
        true, false, "USDC", 6, true, "", "", mojom::kPolygonMainnetChainId,
        mojom::CoinType::ETH},
       {"0x3Cef98bb43d732E2F285eE605a8158cDE967D219", "Basic Attention Token",
        "bat.png", true, false, "BAT", 18, true, "", "",
        mojom::kPolygonMainnetChainId, mojom::CoinType::ETH},
       {"", "Solana", "", false, false, "SOL", 9, true, "", "",
        mojom::kSolanaMainnet, mojom::CoinType::SOL},
       {"EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v", "USD Coin", "usdc.png",
        true, false, "USDC", 6, true, "", "", mojom::kSolanaMainnet,
        mojom::CoinType::ETH},
       {"Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB", "Tether", "usdt.png",
        false, false, "USDT", 6, true, "", "", mojom::kSolanaMainnet,
        mojom::CoinType::SOL},
       {"0xdac17f958d2ee523a2206206994597c13d831ec7", "Tether", "usdt.png",
        true, false, "USDT", 6, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48", "USD Coin", "usdc.png",
        true, false, "USDC", 6, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"0x0d8775f648430679a709e98d2b0cb6250d2887ef", "Basic Attention Token",
        "bat.png", true, false, "BAT", 18, true, "", "", mojom::kMainnetChainId,
        mojom::CoinType::ETH},
       {"EPeUFDgHRxs9xxEPVaL6kfGQvCon7jmAWKVUHuux1Tpz", "Basic Attention Token",
        "bat.png", true, false, "BAT", 18, true, "", "", mojom::kSolanaMainnet,
        mojom::CoinType::SOL}});
  return *tokens;
}

const std::vector<mojom::BlockchainToken>& GetSardineBuyTokens() {
  static base::NoDestructor<std::vector<mojom::BlockchainToken>> tokens({
      {"", "Ethereum", "", false, false, "ETH", 18, true, "", "",
       mojom::kMainnetChainId, mojom::CoinType::ETH},
      {"0x7Fc66500c84A76Ad7e9c93437bFc5Ac33E2DDaE9", "AAVE", "aave.png", true,
       false, "AAVE", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0x7Fc66500c84A76Ad7e9c93437bFc5Ac33E2DDaE9", "AAVE", "aave.png", true,
       false, "AAVE", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"", "Avalanche", "", false, false, "AVAX", 18, true, "", "",
       mojom::kAvalancheMainnetChainId, mojom::CoinType::ETH},
      {"0x0d8775f648430679a709e98d2b0cb6250d2887ef", "Basic Attention Token",
       "bat.png", true, false, "BAT", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0x4Fabb145d64652a948d72533023f6E7A623C7C53", "Binance USD", "BUSD.png",
       true, false, "BUSD", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0x4Fabb145d64652a948d72533023f6E7A623C7C53", "Binance USD", "busd.png",
       true, false, "BUSD", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0xc00e94Cb662C3520282E6f5717214004A7f26888", "Compound", "comp.png",
       true, false, "Comp", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0x6B175474E89094C44Da98b954EedeAC495271d0F", "DAI", "dai.png", true,
       false, "DAI", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0x0f5d2fb29fb7d3cfee444a200298f468908cc942", "Decentraland", "mana.png",
       true, false, "MANA", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0xf629cbd94d3791c9250152bd8dfbdf380e2a3b9c", "Enjin Coin", "enj.png",
       true, false, "ENJ", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"", "Fantom", "", false, false, "FTM", 18, true, "", "",
       mojom::kFantomMainnetChainId, mojom::CoinType::ETH},
      {"0xdeFA4e8a7bcBA345F687a2f1456F5Edd9CE97202", "Kyber Network",
       "kyber.png", true, false, "KNC", 18, true, "", "",
       mojom::kMainnetChainId, mojom::CoinType::ETH},
      {"0x9f8F72aA9304c8B593d555F12eF6589cC3A579A2", "Maker", "mkr.png", true,
       false, "MKR", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0xd26114cd6ee289accf82350c8d8487fedb8a0c07", "OMG Network", "omg.png",
       true, false, "OMG", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"", "Polygon", "", false, false, "MATIC", 18, true, "", "",
       mojom::kPolygonMainnetChainId, mojom::CoinType::ETH},
      {"0x45804880de22913dafe09f4980848ece6ecbaf78", "Pax Gold", "paxg.png",
       true, false, "PAXG", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0x8e870d67f660d95d5be530380d0ec0bd388289e1", "Pax Dollar", "usdp.png",
       true, false, "PAX", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0x95ad61b0a150d79219dcf64e1e6cc01f0b64c4ce", "Shiba Inu", "shib.png",
       true, false, "SHIB", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"", "Solana", "", false, false, "SOL", 9, true, "", "",
       mojom::kSolanaMainnet, mojom::CoinType::SOL},
      {"0xdAC17F958D2ee523a2206206994597C13D831ec7", "Tether", "usdt.png", true,
       false, "USDT", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0xc944e90c64b2c07662a292be6244bdf05cda44a7", "The Graph",
       "graphToken.png", true, false, "GRT", 18, true, "", "",
       mojom::kMainnetChainId, mojom::CoinType::ETH},
      {"0x3845badAde8e6dFF049820680d1F14bD3903a5d0", "The Sandbox", "sand.png",
       true, false, "SAND", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984", "Uniswap", "uni.png", true,
       false, "UNI", 18, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
      {"0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48", "USD Coin", "usdc.png",
       true, false, "USDC", 6, true, "", "", mojom::kMainnetChainId,
       mojom::CoinType::ETH},
  });
  return *tokens;
}

const std::vector<mojom::OnRampCurrency>& GetOnRampCurrenciesList() {
  static base::NoDestructor<std::vector<mojom::OnRampCurrency>> currencies({
      {"USD",
       "United States Dollar",
       {mojom::OnRampProvider::kWyre, mojom::OnRampProvider::kRamp}},
      {"EUR",
       "Euro",
       {mojom::OnRampProvider::kWyre, mojom::OnRampProvider::kRamp}},
      {"GBP",
       "British Pound Sterling",
       {mojom::OnRampProvider::kWyre, mojom::OnRampProvider::kRamp}},
  });

  return *currencies;
}

const std::string GetSardineNetworkName(const std::string& chain_id) {
  // key = chain_id, value = sardine_network_name
  static std::map<std::string, std::string> sardine_network_names = {
      {mojom::kMainnetChainId, "ethereum"},
      {mojom::kPolygonMainnetChainId, "ethereum"},
      {mojom::kAvalancheMainnetChainId, "avalanche"},
      {mojom::kFantomMainnetChainId, "fantom"},
      {mojom::kSolanaMainnet, "solana"}};
  auto sardine_network_pair = sardine_network_names.find(chain_id.c_str());

  if (sardine_network_pair == sardine_network_names.end()) {
    // not found
    return "";
  } else {
    return sardine_network_pair->second;
  }
}

const base::flat_map<std::string, std::string>& GetInfuraChainEndpoints() {
  static base::NoDestructor<base::flat_map<std::string, std::string>> endpoints(
      {{brave_wallet::mojom::kPolygonMainnetChainId,
        "https://mainnet-polygon.brave.com/"},
       {brave_wallet::mojom::kOptimismMainnetChainId,
        "https://mainnet-optimism.brave.com/"},
       {brave_wallet::mojom::kAuroraMainnetChainId,
        "https://mainnet-aurora.brave.com/"}});

  return *endpoints;
}

}  // namespace brave_wallet
