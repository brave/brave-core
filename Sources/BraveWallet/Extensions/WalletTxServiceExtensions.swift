// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

extension BraveWalletTxService {
  
  // Fetches all pending transactions for all given keyrings
  func pendingTransactions(
    networksForCoin: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]],
    for keyrings: [BraveWallet.KeyringInfo]
  ) async -> [BraveWallet.TransactionInfo] {
    await allTransactions(networksForCoin: networksForCoin, for: keyrings)
      .filter { $0.txStatus == .unapproved }
  }
  
  // Fetches all transactions for all given keyrings
  func allTransactions(
    networksForCoin: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]],
    for keyrings: [BraveWallet.KeyringInfo]
  ) async -> [BraveWallet.TransactionInfo] {
    return await withTaskGroup(
      of: [BraveWallet.TransactionInfo].self,
      body: { @MainActor group in
        for keyring in keyrings {
          guard let keyringCoin = keyring.coin,
                let networksForKeyringCoin = networksForCoin[keyringCoin] else {
            continue
          }
          for info in keyring.accountInfos {
            for network in networksForKeyringCoin where network.supportedKeyrings.contains(keyring.id.rawValue as NSNumber) {
              group.addTask { @MainActor in
                await self.allTransactionInfo(info.coin, chainId: network.chainId, from: info.accountId)
              }
            }
          }
        }
        var allTx: [BraveWallet.TransactionInfo] = []
        for await transactions in group {
          allTx.append(contentsOf: transactions)
        }
        return allTx
      }
    )
  }
  
  // Fetches all transactions for a given AccountInfo
  func allTransactions(
    networks: [BraveWallet.NetworkInfo],
    for accountInfo: BraveWallet.AccountInfo
  ) async -> [BraveWallet.TransactionInfo] {
    return await withTaskGroup(
      of: [BraveWallet.TransactionInfo].self,
      body: { @MainActor group in
        for network in networks where network.supportedKeyrings.contains(accountInfo.accountId.keyringId.rawValue as NSNumber) {
          group.addTask { @MainActor in
            await self.allTransactionInfo(accountInfo.coin, chainId: network.chainId, from: accountInfo.accountId)
          }
        }
        var allTx: [BraveWallet.TransactionInfo] = []
        for await transactions in group {
          allTx.append(contentsOf: transactions)
        }
        return allTx
      }
    )
  }
}
