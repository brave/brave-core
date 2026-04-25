// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

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
          for network in networksForAccount
          where network.supportedKeyrings.contains(account.keyringId.rawValue as NSNumber) {
            group.addTask { @MainActor in
              await self.allTransactionInfo(
                coinType: account.coin,
                chainId: network.chainId,
                from: account.accountId
              )
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
        for network in networks
        where network.supportedKeyrings.contains(
          accountInfo.accountId.keyringId.rawValue as NSNumber
        ) {
          group.addTask { @MainActor in
            await self.allTransactionInfo(
              coinType: accountInfo.coin,
              chainId: network.chainId,
              from: accountInfo.accountId
            )
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

  /// Handles the `TransactionAction` for a given transaction, returning error message if failed.
  @MainActor func handleTransactionFollowUpAction(
    _ action: TransactionFollowUpAction,
    transaction: BraveWallet.TransactionInfo
  ) async -> String? {
    switch action {
    case .retry:
      guard transaction.isRetriable else {
        return Strings.Wallet.unknownError
      }
      let (success, _, error) = await retryTransaction(
        coinType: transaction.coin,
        chainId: transaction.chainId,
        txMetaId: transaction.id
      )
      if success {
        return nil
      }
      return error
    case .cancel, .speedUp:
      guard transaction.isCancelOrSpeedUpTransactionSupported else {
        return Strings.Wallet.unknownError
      }
      let (success, _, error) = await speedupOrCancelTransaction(
        coinType: transaction.coin,
        chainId: transaction.chainId,
        txMetaId: transaction.id,
        cancel: action == .cancel
      )
      if success {
        return nil
      }
      return error
    }
  }
}
