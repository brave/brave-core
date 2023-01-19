/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_WEB3_PROVIDER_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_WEB3_PROVIDER_CONSTANTS_H_

#include <stdint.h>

namespace brave_wallet {

extern const char kConnectEvent[];
extern const char kDisconnectEvent[];

namespace ethereum {
extern const char kChainChangedEvent[];
extern const char kAccountsChangedEvent[];
extern const char kMessageEvent[];
}  // namespace ethereum

namespace solana {
extern const char kAccountChangedEvent[];
constexpr char kConnect[] = "connect";
constexpr char kDisconnect[] = "disconnect";
constexpr char kSignTransaction[] = "signTransaction";
constexpr char kSignAndSendTransaction[] = "signAndSendTransaction";
constexpr char kSignAllTransactions[] = "signAllTransactions";
constexpr char kSignMessage[] = "signMessage";
}  // namespace solana

constexpr char kEthAccounts[] = "eth_accounts";
constexpr char kEthCoinbase[] = "eth_coinbase";
constexpr char kEthRequestAccounts[] = "eth_requestAccounts";
constexpr char kEthSendTransaction[] = "eth_sendTransaction";
constexpr char kEthSignTransaction[] = "eth_signTransaction";
constexpr char kEthSendRawTransaction[] = "eth_sendRawTransaction";
constexpr char kEthGetBlockByNumber[] = "eth_getBlockByNumber";
constexpr char kEthBlockNumber[] = "eth_blockNumber";
constexpr char kEthSign[] = "eth_sign";
constexpr char kPersonalSign[] = "personal_sign";
constexpr char kPersonalEcRecover[] = "personal_ecRecover";
constexpr char kEthGetEncryptionPublicKey[] = "eth_getEncryptionPublicKey";
constexpr char kEthDecrypt[] = "eth_decrypt";
constexpr char kWalletWatchAsset[] = "wallet_watchAsset";
constexpr char kMetamaskWatchAsset[] = "metamask_watchAsset";
constexpr char kWeb3ClientVersion[] = "web3_clientVersion";
constexpr char kEthSubscribe[] = "eth_subscribe";
constexpr char kEthUnsubscribe[] = "eth_unsubscribe";
constexpr char kEthSubscribeNewHeads[] = "newHeads";
constexpr char kEthSubscribeLogs[] = "logs";

// We currently don't handle it until MetaMask point it to v3 or v4 other than
// v1 or v2
constexpr char kEthSignTypedData[] = "eth_signTypedData";
constexpr char kEthSignTypedDataV3[] = "eth_signTypedData_v3";
constexpr char kEthSignTypedDataV4[] = "eth_signTypedData_v4";
constexpr char kId[] = "id";
constexpr char kMethod[] = "method";
constexpr char kParams[] = "params";
constexpr char kAddEthereumChainMethod[] = "wallet_addEthereumChain";
constexpr char kSwitchEthereumChainMethod[] = "wallet_switchEthereumChain";
constexpr char kRequestPermissionsMethod[] = "wallet_requestPermissions";
constexpr char kGetPermissionsMethod[] = "wallet_getPermissions";

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_WEB3_PROVIDER_CONSTANTS_H_
