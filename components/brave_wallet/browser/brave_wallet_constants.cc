/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"

#include <map>
#include <string>

#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/no_destructor.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "brave/components/constants/webui_url_constants.h"

namespace brave_wallet {

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

const base::flat_map<std::string, std::string>&
GetEthBalanceScannerContractAddresses() {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      contract_addresses(
          // Mainnet, Polygon, and Avalanche conctract addresses pulled from
          // https://github.com/MyCryptoHQ/eth-scan
          {{mojom::kMainnetChainId,
            "0x08A8fDBddc160A7d5b957256b903dCAb1aE512C5"},
           {mojom::kPolygonMainnetChainId,
            "0x08A8fDBddc160A7d5b957256b903dCAb1aE512C5"},
           {mojom::kAvalancheMainnetChainId,
            "0x08A8fDBddc160A7d5b957256b903dCAb1aE512C5"},
           // BSC, Optimism, and Arbitrum contract addresses pulled from
           // https://github.com/onyb/x/blob/75800edce88688dcfe59dd6b4a664087862369bb/core/evm/scanner/balances/EVMScanner.ts
           {mojom::kBnbSmartChainMainnetChainId,
            "0x53242a975aa7c607e17138b0e0231162e3e68593"},
           {mojom::kOptimismMainnetChainId,
            "0x9e5076DF494FC949aBc4461F4E57592B81517D81"},
           {mojom::kArbitrumMainnetChainId,
            "0xa3e7eb35e779f261ca604138d41d0258e995e97b"}});

  return *contract_addresses;
}

const std::vector<std::string>& GetEthSupportedNftInterfaces() {
  static base::NoDestructor<std::vector<std::string>> interfaces({
      kERC721InterfaceId,
      kERC1155InterfaceId,
  });

  return *interfaces;
}

const std::string GetAssetRatioBaseURL() {
  std::string ratios_url =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kAssetRatioDevUrl);
  if (ratios_url.empty()) {
    ratios_url = "https://ratios.wallet.brave.com";
  }

  return ratios_url;
}

// See https://api-docs.ankr.com/reference/post_ankr-getaccountbalance-1
// for full list.
const base::flat_map<std::string, std::string>& GetAnkrBlockchains() {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      blockchains({{mojom::kArbitrumMainnetChainId, "arbitrum"},
                   {mojom::kAvalancheMainnetChainId, "avalanche"},
                   {mojom::kBaseMainnetChainId, "base"},
                   {mojom::kBnbSmartChainMainnetChainId, "bsc"},
                   {mojom::kMainnetChainId, "eth"},
                   {mojom::kFantomMainnetChainId, "fantom"},
                   {mojom::kFlareMainnetChainId, "flare"},
                   {mojom::kGnosisChainId, "gnosis"},
                   {mojom::kOptimismMainnetChainId, "optimism"},
                   {mojom::kPolygonMainnetChainId, "polygon"},
                   {mojom::kPolygonZKEVMChainId, "polygon_zkevm"},
                   {mojom::kRolluxMainnetChainId, "rollux"},
                   {mojom::kSyscoinMainnetChainId, "syscoin"},
                   {mojom::kZkSyncEraChainId, "zksync_era"}});

  return *blockchains;
}

// CSP Override for CSPDirectiveName::FrameSrc.
// On desktop, will return:
//   "frame-src chrome-untrusted://nft-display/
//   chrome-untrusted://line-chart-display/ chrome-untrusted://market-display/
//   chrome-untrusted://trezor-bridge/ chrome-untrusted://ledger-bridge/;"
// On mobile, will return:
//   "frame-src chrome-untrusted://nft-display/
//   chrome-untrusted://line-chart-display/ chrome-untrusted://market-display/;"
std::string GetWalletFrameSrcCSP() {
  std::string frameSrcCSP =
      base::JoinString({kCSPFrameSrcName, kUntrustedNftURL,
                        kUntrustedLineChartURL, kUntrustedMarketURL},
                       " ");
#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
  // Trezor & Ledger not supported on Android/iOS
  frameSrcCSP.append(std::string(" ") + mojom::kUntrustedTrezorBridgeURL);
  frameSrcCSP.append(std::string(" ") + mojom::kUntrustedLedgerBridgeURL);
#endif
  frameSrcCSP.append(";");
  return frameSrcCSP;
}

// CSP Override for CSPDirectiveName::ImgSrc.
// When is_panel is false, will return
//   "img-src 'self' data: chrome://resources chrome://erc-token-images
//   chrome://image;"
// When is_panel is true, will return:
//   "img-src 'self' data: chrome://resources chrome://erc-token-images
//   chrome://image chrome://favicon https://assets.cgproxy.brave.com;"
std::string GetWalletImgSrcCSP(bool is_panel) {
  std::string imgSrcCSP = base::JoinString(
      {kCSPImageSrcName, kCSPSelf, kCSPData, kCSPChromeResources,
       kCSPChromeErcTokenImages, kCSPChromeImage},
      " ");
  if (is_panel) {
    // DApp panels
    imgSrcCSP.append(std::string(" ") + kCSPChromeFavicon);
    // Need to load market iframe data. brave-browser/issues/31313
    imgSrcCSP.append(std::string(" ") + kCSPBraveCoinGeckoAssetsProxy);
  }
  imgSrcCSP.append(";");
  return imgSrcCSP;
}

}  // namespace brave_wallet
