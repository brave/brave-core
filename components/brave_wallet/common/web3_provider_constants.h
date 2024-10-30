/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_WEB3_PROVIDER_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_WEB3_PROVIDER_CONSTANTS_H_

namespace brave_wallet {

inline constexpr char kConnectEvent[] = "connect";
inline constexpr char kDisconnectEvent[] = "disconnect";

namespace ethereum {
inline constexpr char kChainChangedEvent[] = "chainChanged";
inline constexpr char kAccountsChangedEvent[] = "accountsChanged";
inline constexpr char kMessageEvent[] = "message";
}  // namespace ethereum

namespace solana {
inline constexpr char kAccountChangedEvent[] = "accountChanged";
inline constexpr char kConnect[] = "connect";
inline constexpr char kDisconnect[] = "disconnect";
inline constexpr char kSignTransaction[] = "signTransaction";
inline constexpr char kSignAndSendTransaction[] = "signAndSendTransaction";
inline constexpr char kSignAllTransactions[] = "signAllTransactions";
inline constexpr char kSignMessage[] = "signMessage";
}  // namespace solana

inline constexpr char kEthAccounts[] = "eth_accounts";
inline constexpr char kEthCoinbase[] = "eth_coinbase";
inline constexpr char kEthRequestAccounts[] = "eth_requestAccounts";
inline constexpr char kEthSendTransaction[] = "eth_sendTransaction";
inline constexpr char kEthSignTransaction[] = "eth_signTransaction";
inline constexpr char kEthSendRawTransaction[] = "eth_sendRawTransaction";
inline constexpr char kEthGetBlockByNumber[] = "eth_getBlockByNumber";
inline constexpr char kEthBlockNumber[] = "eth_blockNumber";
inline constexpr char kEthSign[] = "eth_sign";
inline constexpr char kPersonalSign[] = "personal_sign";
inline constexpr char kPersonalEcRecover[] = "personal_ecRecover";
inline constexpr char kEthGetEncryptionPublicKey[] =
    "eth_getEncryptionPublicKey";
inline constexpr char kEthDecrypt[] = "eth_decrypt";
inline constexpr char kWalletWatchAsset[] = "wallet_watchAsset";
inline constexpr char kMetamaskWatchAsset[] = "metamask_watchAsset";
inline constexpr char kWeb3ClientVersion[] = "web3_clientVersion";
inline constexpr char kEthSubscribe[] = "eth_subscribe";
inline constexpr char kEthUnsubscribe[] = "eth_unsubscribe";
inline constexpr char kEthSubscribeNewHeads[] = "newHeads";
inline constexpr char kEthSubscribeLogs[] = "logs";

inline constexpr char kEthSignTypedDataV3[] = "eth_signTypedData_v3";
inline constexpr char kEthSignTypedDataV4[] = "eth_signTypedData_v4";
inline constexpr char kId[] = "id";
inline constexpr char kMethod[] = "method";
inline constexpr char kParams[] = "params";
inline constexpr char kAddEthereumChainMethod[] = "wallet_addEthereumChain";
inline constexpr char kSwitchEthereumChainMethod[] =
    "wallet_switchEthereumChain";
inline constexpr char kRequestPermissionsMethod[] = "wallet_requestPermissions";
inline constexpr char kGetPermissionsMethod[] = "wallet_getPermissions";

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_WEB3_PROVIDER_CONSTANTS_H_
