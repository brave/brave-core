// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

extension BraveWalletTxService {
  
  // Fetches all pending transactions for all given keyrings. This will return transactions from all networks.
  func pendingTransactions(
    for keyrings: [BraveWallet.KeyringInfo]
  ) async -> [BraveWallet.TransactionInfo] {
    await allTransactions(for: keyrings).filter { $0.txStatus == .unapproved }
  }

  // Fetches all transactions for all given keyrings. This will return transactions from all networks.
  func allTransactions(
    for keyrings: [BraveWallet.KeyringInfo]
  ) async -> [BraveWallet.TransactionInfo] {
    return await withTaskGroup(
      of: [BraveWallet.TransactionInfo].self,
      body: { @MainActor group in
        for keyring in keyrings {
          for info in keyring.accountInfos {
            group.addTask { @MainActor in
              await self.allTransactionInfo(info.coin, chainId: nil, from: info.address)
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
  
  // Fetches all pending transactions for all given keyrings
  func pendingTransactions(
    chainIdsForCoin: [BraveWallet.CoinType: [String]],
    for keyrings: [BraveWallet.KeyringInfo]
  ) async -> [BraveWallet.TransactionInfo] {
    await allTransactions(chainIdsForCoin: chainIdsForCoin, for: keyrings)
      .filter { $0.txStatus == .unapproved }
  }
  
  // Fetches all transactions for all given keyrings
  func allTransactions(
    chainIdsForCoin: [BraveWallet.CoinType: [String]],
    for keyrings: [BraveWallet.KeyringInfo]
  ) async -> [BraveWallet.TransactionInfo] {
    return await withTaskGroup(
      of: [BraveWallet.TransactionInfo].self,
      body: { @MainActor group in
        for keyring in keyrings {
          guard let keyringCoin = keyring.coin,
                let chainIdsForKeyringCoin = chainIdsForCoin[keyringCoin] else {
            continue
          }
          for chainId in chainIdsForKeyringCoin {
            for info in keyring.accountInfos {
              group.addTask { @MainActor in
                await self.allTransactionInfo(info.coin, chainId: chainId, from: info.address)
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
}
