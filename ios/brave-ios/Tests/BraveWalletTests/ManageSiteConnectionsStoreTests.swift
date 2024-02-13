// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
@testable import Data
import TestHelpers
@testable import BraveWallet

class ManageSiteConnectionsStoreTests: CoreDataTestCase {

  let compound = URL(string: "https://compound.finance")!
  let polygon = URL(string: "https://wallet.polygon.technology")!
  let jupiter = URL(string: "https://jup.ag")!
  let walletAccount = "0x4D60d71F411AB671f614eD0ec5B71bEedB46287d"
  let walletAccount2 = "0x4d60d71f411ab671f614ed0ec5b71beedb46287d"
  let solanaAccount = "8uMdzdJ6Mtsh2vFhJJGCfwo8PZrnZZ3qbT85wbo1o4cd"
  var cancellables: Set<AnyCancellable> = .init()
  
  override func setUp() {
    super.setUp()
    let compondDomain = Domain.getOrCreate(forUrl: compound, persistent: true)
    let polygonDomain = Domain.getOrCreate(forUrl: polygon, persistent: true)
    let jupiterDomain = Domain.getOrCreate(forUrl: jupiter, persistent: true)

    // add permissions for `compound` Domain
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(forUrl: compound, coin: .eth, accounts: [walletAccount, walletAccount2], grant: true)
    }
    XCTAssertTrue(compondDomain.walletPermissions(for: .eth, account: walletAccount))
    // add permissions for `polygon` Domain
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(forUrl: polygon, coin: .eth, accounts: [walletAccount, walletAccount2], grant: true)
    }
    XCTAssertTrue(polygonDomain.walletPermissions(for: .eth, account: walletAccount))
    // add permissions for `jupiter` Domain
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(forUrl: jupiter, coin: .sol, accounts: [solanaAccount], grant: true)
    }
    XCTAssertTrue(jupiterDomain.walletPermissions(for: .sol, account: solanaAccount))
  }
  
  func testFetchSiteConnections() {
    // Domains added with Ethereum permissions in `setUp()`
    let store = ManageSiteConnectionsStore(keyringStore: .previewStore)
    
    // verify `siteConnections` is updated when `fetchSiteConnections()` is called
    let siteConnectionsException = expectation(description: "siteConnections")
    XCTAssertTrue(store.siteConnections.isEmpty)  // Initial state
    store.$siteConnections
      .dropFirst()
      .first()
      .sink { siteConnections in
        defer { siteConnectionsException.fulfill() }
        XCTAssertEqual(siteConnections[0], .init(url: self.polygon.absoluteString, connectedAddresses: [self.walletAccount, self.walletAccount2], coin: .eth))
        XCTAssertEqual(siteConnections[1], .init(url: self.compound.absoluteString, connectedAddresses: [self.walletAccount, self.walletAccount2], coin: .eth))
        XCTAssertEqual(siteConnections[2], .init(url: self.jupiter.absoluteString, connectedAddresses: [self.solanaAccount], coin: .sol))
      }.store(in: &cancellables)
    
    store.fetchSiteConnections()

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
  
  /// Test `removeAllPermissions(from:)` will remove all permissions from the given `SiteConnections` array
  func testRemoveAllPermissions() {
    // Domains added with Ethereum permissions in `setUp()`
    let store = ManageSiteConnectionsStore(keyringStore: .previewStore)
    store.fetchSiteConnections()
    XCTAssertEqual(store.siteConnections.count, 3)
    
    // remove permissions
    let siteConnectionToRemove = store.siteConnections[0]
    backgroundSaveAndWaitForExpectation {
      store.removeAllPermissions(from: [siteConnectionToRemove])
    }
    DataController.viewContext.refreshAllObjects()
    
    // verify `siteConnections` is updated
    XCTAssertEqual(store.siteConnections.count, 2)
    XCTAssertNotEqual(store.siteConnections[0].url, siteConnectionToRemove.url)
    // verify `Domain` data is removed
    let domain = Domain.getOrCreate(forUrl: URL(string: siteConnectionToRemove.url)!, persistent: true)
    XCTAssertFalse(domain.walletPermissions(for: .eth, account: walletAccount))
    XCTAssertFalse(domain.walletPermissions(for: .eth, account: walletAccount2))
  }
  
  /// Test `removePermissions(from:url:)` will remove the given account permissions for the given url
  func testRemovePermissions() {
    // Domains added with Ethereum permissions in `setUp()`
    let store = ManageSiteConnectionsStore(keyringStore: .previewStore)
    store.fetchSiteConnections()
    XCTAssertEqual(store.siteConnections.count, 3)
    
    // remove some account permissions but not all
    let siteConnectionToRemoveAccount = store.siteConnections[1]
    XCTAssertEqual(siteConnectionToRemoveAccount.connectedAddresses.count, 2)
    backgroundSaveAndWaitForExpectation {
      store.removePermissions(for: .eth, from: [walletAccount], url: URL(string: siteConnectionToRemoveAccount.url)!)
    }
    DataController.viewContext.refreshAllObjects()
    
    // verify `siteConnections` is updated to remove specific accounts from the `SiteConnection`
    XCTAssertEqual(store.siteConnections.count, 3)
    XCTAssertEqual(store.siteConnections[1].connectedAddresses.count, 1)
    XCTAssertFalse(store.siteConnections[1].connectedAddresses.contains(walletAccount))
    
    // verify `Domain` data is updated
    let domain = Domain.getOrCreate(forUrl: URL(string: siteConnectionToRemoveAccount.url)!, persistent: true)
    XCTAssertFalse(domain.walletPermissions(for: .eth, account: walletAccount))
  }
  
  /// Test `removePermissions(from:url:)` will remove the `SiteConnection` from `siteConnections` when the last connected account is removed
  func testRemovePermissionsLastPermission() {
    // Domains added with Ethereum permissions in `setUp()`
    let store = ManageSiteConnectionsStore(keyringStore: .previewStore)
    store.fetchSiteConnections()
    XCTAssertEqual(store.siteConnections.count, 3)
    
    // remove `walletAccount` permissions for this `SiteConnection`
    let siteConnectionToRemove = store.siteConnections[0]
    XCTAssertEqual(siteConnectionToRemove.connectedAddresses.count, 2)
    backgroundSaveAndWaitForExpectation {
      store.removePermissions(for: .eth, from: [walletAccount], url: URL(string: siteConnectionToRemove.url)!)
    }
    DataController.viewContext.refreshAllObjects()
    
    // remove `walletAccount2` permissions for this `SiteConnection`
    XCTAssertEqual(store.siteConnections[0].connectedAddresses.count, 1)
    backgroundSaveAndWaitForExpectation {
      store.removePermissions(for: .eth, from: [walletAccount2], url: URL(string: siteConnectionToRemove.url)!)
    }
    DataController.viewContext.refreshAllObjects()
    
    // verify `siteConnections` is updated to remove the `SiteConnection` as all `connectedAddresses` are removed
    XCTAssertEqual(store.siteConnections.count, 2)
    XCTAssertNotEqual(store.siteConnections[0].url, siteConnectionToRemove.url)
    
    // verify `Domain` data is removed
    let domain = Domain.getOrCreate(forUrl: URL(string: siteConnectionToRemove.url)!, persistent: true)
    XCTAssertFalse(domain.walletPermissions(for: .eth, account: walletAccount))
  }
}
