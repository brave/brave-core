// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

extension BraveWalletTxService {
  
  // Fetches all pending transactions for all given keyrings
  func pendingTransactions(
    for keyrings: [BraveWallet.KeyringInfo]
  ) async -> [BraveWallet.TransactionInfo] {
    return await withTaskGroup(
      of: [BraveWallet.TransactionInfo].self,
      body: { @MainActor group in
        for keyring in keyrings {
          for info in keyring.accountInfos {
            group.addTask { @MainActor in
              await self.allTransactionInfo(info.coin, from: info.address)
            }
          }
        }
        var allPendingTx: [BraveWallet.TransactionInfo] = []
        for await transactions in group {
          allPendingTx.append(contentsOf: transactions.filter { $0.txStatus == .unapproved })
        }
        return allPendingTx
      }
    )
  }
}
