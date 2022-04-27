// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared

extension Preferences {
  public final class Wallet {
    /// Whether or not webpages can use the Ethereum Provider API to communicate with users ethereum wallet
    public static let allowEthereumProviderAccountRequests: Option<Bool> = .init(
      key: "wallet.allow-eth-provider-account-requests",
      default: true
    )
  }
}
