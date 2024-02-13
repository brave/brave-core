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
    for accounts: [BraveWallet.AccountInfo]
  ) async -> [BraveWallet.TransactionInfo] {
    await allTransactions(networksForCoin: networksForCoin, for: accounts)
      .filter { $0.txStatus == .unapproved }
  }
  
  // Fetches all transactions for all given keyrings
  func allTransactions(
    networksForCoin: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]],
    for accounts: [BraveWallet.AccountInfo]
  ) async -> [BraveWallet.TransactionInfo] {
    return await withTaskGroup(
      of: [BraveWallet.TransactionInfo].self,
      body: { @MainActor group in
        for account in accounts {
          guard let networksForAccount = networksForCoin[account.coin] else { continue }
          for network in networksForAccount where network.supportedKeyrings.contains(account.keyringId.rawValue as NSNumber) {
            group.addTask { @MainActor in
              await self.allTransactionInfo(account.coin, chainId: network.chainId, from: account.accountId)
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
