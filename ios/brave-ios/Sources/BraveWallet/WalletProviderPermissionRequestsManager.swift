// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation

/// A permission request for a specific dapp
public struct WebpagePermissionRequest: Equatable {
  /// A users response type
  public enum Response {
    /// The user rejected the prompt by dismissing the screen
    case rejected
    /// The user granted access to a set of accounts
    case granted(accounts: [String])
  }
  /// The origin that requested this permission
  let requestingOrigin: URLOrigin
  /// The accounts being requested to connect
  let requestingAccounts: [String]
  /// The type of request
  let coinType: BraveWallet.CoinType
  /// A handler to be called when the user either approves or rejects the connection request
  let decisionHandler: (Response) -> Void
  /// A completion block from `BraveWalletProviderDelegate` that needs to be perfromed when
  /// wallet notificaiton has been ignored by users
  public var providerHandler: RequestPermissionsCallback?

  public static func == (lhs: Self, rhs: Self) -> Bool {
    lhs.requestingOrigin == rhs.requestingOrigin
      && lhs.coinType == rhs.coinType
      && lhs.requestingAccounts == rhs.requestingAccounts
  }
}

/// Handles dapp permission requests when connecting your wallet for the current session.
///
/// When a request begins you should first check if any pending requests exist for the origin using
/// ``hasPendingRequest(for:coinType)`` and if none exist, use ``beginRequest(for:coinType)`` to add one.
/// When the user completes the request you can call ``WebpagePermissionRequest.decisionHandler(_:)`` with
/// the users response.
public class WalletProviderPermissionRequestsManager {
  /// A shared instance of the permissions manager
  public static let shared: WalletProviderPermissionRequestsManager = .init()

  private var requests: [WebpagePermissionRequest] = []

  private init() {}

  /// Adds a permission request for a specific origin and coin type. Optionally you can be notified of the
  /// users response by providing a closure
  public func beginRequest(
    for origin: URLOrigin,
    accounts: [String],
    coinType: BraveWallet.CoinType,
    providerHandler: RequestPermissionsCallback?,
    completion: ((WebpagePermissionRequest.Response) -> Void)? = nil
  ) -> WebpagePermissionRequest {
    var request = WebpagePermissionRequest(
      requestingOrigin: origin,
      requestingAccounts: accounts,
      coinType: coinType
    ) { [weak self] decision in
      guard let self = self, let originURL = origin.url else { return }
      if case .granted(let accounts) = decision {
        Domain.setWalletPermissions(
          forUrl: originURL,
          coin: coinType,
          accounts: accounts,
          grant: true
        )
      }
      self.requests.removeAll(where: { $0.requestingOrigin == origin && $0.coinType == coinType })
      completion?(decision)
    }
    request.providerHandler = providerHandler
    requests.append(request)
    return request
  }

  public func hasPendingRequest(for origin: URLOrigin, coinType: BraveWallet.CoinType) -> Bool {
    requests.contains(where: { $0.requestingOrigin == origin && $0.coinType == coinType })
  }

  /// Returns a list of pending requests waiting for a given origin and coin types
  public func pendingRequests(
    for origin: URLOrigin,
    coinType: BraveWallet.CoinType
  ) -> [WebpagePermissionRequest] {
    requests.filter({ $0.requestingOrigin == origin && $0.coinType == coinType })
  }

  /// Returns the first available pending request for a given origin and for the given coin types.
  public func firstPendingRequest(
    for origin: URLOrigin,
    coinTypes: [BraveWallet.CoinType]
  ) -> WebpagePermissionRequest? {
    requests.filter { $0.requestingOrigin == origin && coinTypes.contains($0.coinType) }.first
  }

  /// Cancels an in-flight request without executing any decision
  public func cancelRequest(_ request: WebpagePermissionRequest) {
    requests.removeAll(where: { $0 == request })
  }

  /// Cancels all an in-flight requests without executing any decision
  public func cancelAllPendingRequests(for coins: [BraveWallet.CoinType]) {
    requests = requests.filter { !coins.contains($0.coinType) }
  }
}
