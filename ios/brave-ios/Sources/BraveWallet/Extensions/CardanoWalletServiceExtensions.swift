// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension BraveWalletCardanoWalletService {
  /// Fetch ADA balance for a given account.
  /// - Parameters:
  ///     - accountId: The `BraveWallet.AccountId` for the account
  /// - Returns: The ADA balance of the given `BraveWallet.AccountId` in formatted`Double`;
  func fetchADABalances(accountId: BraveWallet.AccountId) async -> Double {
    guard let adaBalance = await self.balance(accountId: accountId, tokenIdHex: nil).0
    else { return 0.0 }

    return Double(adaBalance.totalBalance) / 1_000_000
  }
}
