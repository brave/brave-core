// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import BraveCore

struct SiteConnection: Equatable, Identifiable {
  let url: String
  let connectedAddresses: [String]
  
  var id: String { url }
}

extension Array where Element == SiteConnection {
  /// Filters the array by checking if the given `text` exists in the `url` or `connectedAccountAddresses`
  func filter(by text: String) -> [SiteConnection] {
    filter { siteConnections in
      guard !text.isEmpty else { return true }
      let filterText = text.lowercased()
      return siteConnections.url.contains(filterText) || siteConnections.connectedAddresses.contains(where: { $0.caseInsensitiveCompare(filterText) == .orderedSame })
    }
  }
}

class ManageSiteConnectionsStore: ObservableObject {
  @Published var siteConnections: [SiteConnection] = []
  
  var keyringStore: KeyringStore
  
  init(keyringStore: KeyringStore) {
    self.keyringStore = keyringStore
  }
  
  /// Fetch all site connections with 1+ accounts connected
  func fetchSiteConnections() {
    let domains = Domain.allDomainsWithEthereumPermissions()
    let connections = domains.map {
      SiteConnection(
        url: $0.url ?? "",
        connectedAddresses: ($0.wallet_permittedAccounts ?? "")
          .split(separator: ",")
          .map(String.init)
      )
    }
    self.siteConnections = connections
  }
  
  /// Remove permissions for all accounts on each given `SiteConnection`s
  func removeAllPermissions(from siteConnectionsToRemove: [SiteConnection]) {
    siteConnectionsToRemove.forEach { siteConnection in
      guard let url = URL(string: siteConnection.url) else { return }
      Domain.setEthereumPermissions(forUrl: url, accounts: siteConnection.connectedAddresses, grant: false)
      if let index = self.siteConnections.firstIndex(where: { $0.id.caseInsensitiveCompare(siteConnection.id) == .orderedSame }) {
        self.siteConnections.remove(at: index)
      }
    }
  }
  
  /// Remove permissions from the given `accounts` for the given `url`
  func removePermissions(from accounts: [String], url: URL) {
    if let index = siteConnections.firstIndex(where: { $0.url == url.absoluteString }),
        let siteConnection = siteConnections[safe: index] {
      let updatedConnectedAddresses = siteConnection.connectedAddresses.filter { !accounts.contains($0) }
      let updatedSiteConnection = SiteConnection(url: siteConnection.url, connectedAddresses: updatedConnectedAddresses)
      
      var updatedSiteConnections = siteConnections
      updatedSiteConnections.remove(at: index)
      if !updatedConnectedAddresses.isEmpty {
        updatedSiteConnections.insert(updatedSiteConnection, at: index)
      }
      self.siteConnections = updatedSiteConnections
    }
    Domain.setEthereumPermissions(forUrl: url, accounts: accounts, grant: false)
  }
  
  func accountInfo(for address: String) -> BraveWallet.AccountInfo? {
    keyringStore.allAccounts.first(where: { $0.address == address })
  }
}
