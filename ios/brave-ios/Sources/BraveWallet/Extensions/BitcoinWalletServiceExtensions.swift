// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

enum BTCBalanceType {
  case total
  case available
  case pending
}

extension BraveWalletBitcoinWalletService {

  /// - Parameters:
  ///     - accounts: A list of `BraveWallet.AccountInfo`
  ///
  /// - Returns: A dictionary of `BraveWallet.AccountInfo.uniqueKey` as key and associated `BraveWallet.BitcoinAccountInfo`
  func fetchBitcoinAccountInfo(
    accounts: [BraveWallet.AccountInfo]
  ) async -> [String: BraveWallet.BitcoinAccountInfo] {
    await withTaskGroup(
      of: [String: BraveWallet.BitcoinAccountInfo].self,
      body: { group in
        for account in accounts {
          group.addTask {
            guard account.coin == .btc, account.address.isEmpty else { return [:] }
            if let bitcoinAccount = await self.bitcoinAccountInfo(accountId: account.accountId) {
              return [account.accountId.uniqueKey: bitcoinAccount]
            }
            return [:]
          }
        }
        return await group.reduce(
          into: [String: BraveWallet.BitcoinAccountInfo](),
          { partialResult, new in
            partialResult.merge(with: new)
          }
        )
      }
    )
  }

  /// - Parameters:
  ///     - accountId: A list of `BraveWallet.AccountId`
  ///     - type: `BTCBalanceType` which indicates which btc balance it should return
  /// - Returns: The BTC balance of the given `BraveWallet.AccountId` in `Double`; Will return a nil if there is an issue fetching balance.
  func fetchBTCBalance(accountId: BraveWallet.AccountId, type: BTCBalanceType) async -> Double? {
    guard let btcBalance = await self.balance(accountId: accountId).0
    else { return nil }

    let balanceString: String
    switch type {
    case .total:
      balanceString = String(btcBalance.totalBalance)
    case .available:
      balanceString = String(btcBalance.availableBalance)
    case .pending:
      balanceString = String(btcBalance.pendingBalance)
    }
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 8))
    if let valueString = formatter.decimalString(
      for: balanceString,
      radix: .decimal,
      decimals: 8
    ) {
      return Double(valueString)
    } else {
      return nil
    }
  }
}
