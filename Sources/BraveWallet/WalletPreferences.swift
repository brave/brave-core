// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import struct Shared.Strings

extension Preferences {
  public final class Wallet {
    public enum WalletType: Int, Identifiable, CaseIterable {
      case none
      case brave
      
      public var id: Int {
        rawValue
      }
      
      public var name: String {
        switch self {
        case .none:
          return Strings.Wallet.walletTypeNone
        case .brave:
          return Strings.Wallet.braveWallet
        }
      }
    }
    /// The default wallet to use for Ethereum to be communicate with web3
    public static let defaultEthWallet = Option<Int>(key: "wallet.default-wallet", default: WalletType.brave.rawValue)
    /// The default wallet to use for Solana to be communicate with web3
    public static let defaultSolWallet = Option<Int>(key: "wallet.default-sol-wallet", default: WalletType.brave.rawValue)
    /// Whether or not webpages can use the Ethereum/Solana Provider API to communicate with users Ethereum/Solana wallet
    public static let allowDappProviderAccountRequests: Option<Bool> = .init(
      key: "wallet.allow-eth-provider-account-requests",
      default: true
    )
    /// The option to display web3 notification
    public static let displayWeb3Notifications = Option<Bool>(key: "wallet.display-web3-notifications", default: true)
    /// The option to determine if we show or hide test networks in network lists
    public static let showTestNetworks = Option<Bool>(key: "wallet.show-test-networks", default: false)
    /// The option for users to turn off aurora popup
    public static let showAuroraPopup = Option<Bool>(key: "wallet.show-aurora-popup", default: true)
  }
}
