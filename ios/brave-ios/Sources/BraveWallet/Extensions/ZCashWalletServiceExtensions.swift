// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

extension BraveWalletZCashWalletService {
  /// - Parameters:
  ///     - accounts: A list of `BraveWallet.AccountInfo`
  ///
  /// - Returns: A dictionary of `BraveWallet.AccountInfo.uniqueKey` as key and associated `BraveWallet.ZCashAccountInfo`
  func fetchZcashAccountInfo(
    accounts: [BraveWallet.AccountInfo]
  ) async -> [String: BraveWallet.ZCashAccountInfo] {
    await withTaskGroup(
      of: [String: BraveWallet.ZCashAccountInfo].self,
      body: { group in
        for account in accounts {
          group.addTask {
            guard account.coin == .zec, account.address.isEmpty else { return [:] }
            if let zcashAccount = await self.zCashAccountInfo(accountId: account.accountId) {
              return [account.accountId.uniqueKey: zcashAccount]
            }
            return [:]
          }
        }
        return await group.reduce(
          into: [String: BraveWallet.ZCashAccountInfo](),
          { partialResult, new in
            partialResult.merge(with: new)
          }
        )
      }
    )
  }
}
