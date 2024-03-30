// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation

struct SiteConnection: Equatable, Identifiable {
  let url: String
  let connectedAddresses: [String]
  let coin: BraveWallet.CoinType

  var id: String { url }
}

extension Array where Element == SiteConnection {
  /// Filters the array by checking if the given `text` exists in the `url` or `connectedAccountAddresses` and match the given
  /// coin type `coin`
  func filter(by coin: BraveWallet.CoinType, text: String) -> [SiteConnection] {
    filter { siteConnections in
      guard !text.isEmpty else {
        return siteConnections.coin == coin
      }
      let filterText = text.lowercased()
      return
        (siteConnections.url.contains(filterText)
        || siteConnections.connectedAddresses.contains(where: {
          $0.caseInsensitiveCompare(filterText) == .orderedSame
        })) && siteConnections.coin == coin
    }
  }
}

class ManageSiteConnectionsStore: ObservableObject, WalletObserverStore {
  @Published var siteConnections: [SiteConnection] = []

  var keyringStore: KeyringStore

  var isObserving: Bool = false

  init(keyringStore: KeyringStore) {
    self.keyringStore = keyringStore
  }

  /// Fetch all site connections with 1+ accounts connected
  func fetchSiteConnections() {
    var connections = [SiteConnection]()
    // only coin types support dapps have site connection screen
    for coin in WalletConstants.supportedCoinTypes(.dapps) {
      let domains = Domain.allDomainsWithWalletPermissions(for: coin)
      connections.append(
        contentsOf: domains.map {
          var connectedAddresses = [String]()
          if let urlString = $0.url,
            let url = URL(string: urlString),
            let addresses = Domain.walletPermissions(forUrl: url, coin: coin)
          {
            connectedAddresses = addresses
          }
          return SiteConnection(
            url: $0.url ?? "",
            connectedAddresses: connectedAddresses,
            coin: coin
          )
        }
      )
    }
    self.siteConnections = connections
  }

  /// Remove permissions for all accounts on each given `SiteConnection`s
  func removeAllPermissions(from siteConnectionsToRemove: [SiteConnection]) {
    siteConnectionsToRemove.forEach { siteConnection in
      guard let url = URL(string: siteConnection.url) else { return }
      Domain.setWalletPermissions(
        forUrl: url,
        coin: siteConnection.coin,
        accounts: siteConnection.connectedAddresses,
        grant: false
      )
      if let index = self.siteConnections.firstIndex(where: {
        $0.id.caseInsensitiveCompare(siteConnection.id) == .orderedSame
          && $0.coin == siteConnection.coin
      }) {
        self.siteConnections.remove(at: index)
      }
    }
  }

  /// Remove permissions from the given `accounts` for the given `url`
  func removePermissions(for coin: BraveWallet.CoinType, from accounts: [String], url: URL) {
    if let index = siteConnections.firstIndex(where: {
      $0.url == url.absoluteString && $0.coin == coin
    }),
      let siteConnection = siteConnections[safe: index]
    {
      let updatedConnectedAddresses = siteConnection.connectedAddresses.filter {
        !accounts.contains($0)
      }
      let updatedSiteConnection = SiteConnection(
        url: siteConnection.url,
        connectedAddresses: updatedConnectedAddresses,
        coin: coin
      )

      var updatedSiteConnections = siteConnections
      updatedSiteConnections.remove(at: index)
      if !updatedConnectedAddresses.isEmpty {
        updatedSiteConnections.insert(updatedSiteConnection, at: index)
      }
      self.siteConnections = updatedSiteConnections
    }
    Domain.setWalletPermissions(forUrl: url, coin: coin, accounts: accounts, grant: false)
  }

  func accountInfo(for address: String) -> BraveWallet.AccountInfo? {
    keyringStore.allAccounts.first(where: { $0.address == address })
  }
}
