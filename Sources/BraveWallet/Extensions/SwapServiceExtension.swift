// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

extension BraveWalletSwapService {
  /// Helper function to determine if we support swaping for a given chainId & coin type
  /// Swap may be supported by `swapService` prior to being supported on iOS.
  func isiOSSwapSupported(chainId: String, coin: BraveWallet.CoinType) async -> Bool {
    if coin == .eth {
      return await isSwapSupported(chainId)
    } else {
      return false
    }
  }
}
