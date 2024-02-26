// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

/// A permission request for a specific dapp
public struct WalletProviderAccountCreationRequest: Equatable {
  /// A users response type
  public enum Response {
    /// The user rejected the prompt by dismissing the screen
    case rejected
    /// The user created a new account
    case created
  }
  /// The origin that requested this permission
  let requestingOrigin: URLOrigin
  /// The type of request
  let coinType: BraveWallet.CoinType
  /// A handler to be called when the user rejects to create a new account
  let responseHandler: (Response) -> Void

  public static func == (lhs: Self, rhs: Self) -> Bool {
    lhs.requestingOrigin == rhs.requestingOrigin && lhs.coinType == rhs.coinType
  }
}

public class WalletProviderAccountCreationRequestManager {
  /// A shared instance of the account creation manager
  public static let shared: WalletProviderAccountCreationRequestManager = .init()

  private var requests: [WalletProviderAccountCreationRequest] = []

  public func hasPendingRequest(for origin: URLOrigin, coinType: BraveWallet.CoinType) -> Bool {
    requests.contains(where: { $0.requestingOrigin == origin && $0.coinType == coinType })
  }

  /// Returns a list of pending requests waiting for a given origin and coin types
  public func pendingRequests(
    for origin: URLOrigin,
    coinType: BraveWallet.CoinType
  ) -> [WalletProviderAccountCreationRequest] {
    requests.filter({ $0.requestingOrigin == origin && $0.coinType == coinType })
  }

  public func removeRequest(for origin: URLOrigin, coinType: BraveWallet.CoinType) {
    if let index = requests.firstIndex(where: {
      $0.requestingOrigin == origin && $0.coinType == coinType
    }) {
      requests.remove(at: index)
    }
  }

  public func removeRequest(for request: WalletProviderAccountCreationRequest) {
    if let index = requests.firstIndex(where: { $0 == request }) {
      requests.remove(at: index)
    }
  }

  public func beginRequest(
    for origin: URLOrigin,
    coinType: BraveWallet.CoinType,
    completion: (() -> Void)? = nil
  ) {
    let request = WalletProviderAccountCreationRequest(requestingOrigin: origin, coinType: coinType)
    { [weak self] response in
      guard let self = self else { return }
      self.removeRequest(for: origin, coinType: coinType)
      completion?()
    }
    requests.append(request)
  }

  public func firstPendingRequest(
    for origin: URLOrigin,
    coinTypes: [BraveWallet.CoinType]
  ) -> WalletProviderAccountCreationRequest? {
    requests.filter { $0.requestingOrigin == origin && coinTypes.contains($0.coinType) }.first
  }

  public func cancelAllPendingRequests(coins: [BraveWallet.CoinType]) {
    requests = requests.filter { !coins.contains($0.coinType) }
  }
}
