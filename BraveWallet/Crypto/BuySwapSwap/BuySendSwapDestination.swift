// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import struct Shared.Strings

/// Used to determine where a user is navigated to when they tap on a buy, send or swap button
enum BuySendSwapDestination: String, Identifiable, CaseIterable {
  case buy
  case send
  case swap
  
  var id: String {
    rawValue
  }
  
  var localizedTitle: String {
    switch self {
    case .buy:
      return Strings.Wallet.buy
    case .send:
      return Strings.Wallet.send
    case .swap:
      return Strings.Wallet.swap
    }
  }
  
  var localizedDescription: String {
    switch self {
    case .buy:
      return Strings.Wallet.buyDescription
    case .send:
      return Strings.Wallet.sendDescription
    case .swap:
      return Strings.Wallet.swapDescription
    }
  }
}
