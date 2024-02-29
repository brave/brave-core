// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Strings
import SwiftUI

/// Used to determine where a user is navigated to when they tap on a buy, send, swap, deposit button
public struct WalletActionDestination: Identifiable, Equatable, Hashable {
  enum Kind: String, Identifiable, CaseIterable {
    case buy, send, swap, deposit

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
      case .deposit:
        return Strings.Wallet.deposit
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
      case .deposit:
        return Strings.Wallet.depositDescription
      }
    }
  }

  var kind: Kind
  var initialToken: BraveWallet.BlockchainToken?
  var initialAccount: BraveWallet.AccountInfo?
  public var id: String { kind.id }
}

private struct WalletActionDestinationKey: EnvironmentKey {
  static var defaultValue: Binding<WalletActionDestination?> {
    Binding(get: { nil }, set: { _ in })
  }
}

extension EnvironmentValues {
  /// The destination to set when the user wants to access the buy, send, swap or deposit widget from anywhere in the
  /// view hierarchy
  var walletActionDestination: Binding<WalletActionDestination?> {
    get { self[WalletActionDestinationKey.self] }
    set { self[WalletActionDestinationKey.self] = newValue }
  }
}
