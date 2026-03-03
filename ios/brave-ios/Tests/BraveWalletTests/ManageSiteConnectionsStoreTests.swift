// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import TestHelpers
import XCTest

@testable import BraveWallet
@testable import Data

class ManageSiteConnectionsStoreTests: CoreDataTestCase {

  let compound = URL(string: "https://compound.finance")!
  let polygon = URL(string: "https://wallet.polygon.technology")!
  let jupiter = URL(string: "https://jup.ag")!
  let cardano = URL(string: "https://cardano.org")!
  let walletAccount: BraveWallet.AccountInfo = .mockEthAccount
  let walletAccount2 =
    (BraveWallet.AccountInfo.mockEthAccount.copy() as! BraveWallet.AccountInfo)
    .then {
      $0.address = "mock_eth_id_2"
      $0.accountId.address = $0.address
      $0.accountId.uniqueKey = $0.address
      $0.name = "Ethereum Account 2"
    }
  let solanaAccount: BraveWallet.AccountInfo = .mockSolAccount
  let cardanoAccount: BraveWallet.AccountInfo = .mockAdaAccount
  var cancellables: Set<AnyCancellable> = .init()

  override func setUp() {
    super.setUp()
    let compondDomain = Domain.getOrCreate(forUrl: compound, persistent: true)
    let polygonDomain = Domain.getOrCreate(forUrl: polygon, persistent: true)
    let jupiterDomain = Domain.getOrCreate(forUrl: jupiter, persistent: true)
    let cardanoDomain = Domain.getOrCreate(forUrl: cardano, persistent: true)

    // add permissions for `compound` Domain
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(
        forUrl: compound,
        coin: .eth,
        accounts: [walletAccount, walletAccount2].compactMap(\.dAppPermissionId),
        grant: true
      )
    }
    XCTAssertTrue(
      compondDomain.walletPermissions(for: .eth, account: walletAccount.dAppPermissionId!)
    )
    // add permissions for `polygon` Domain
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(
        forUrl: polygon,
        coin: .eth,
        accounts: [walletAccount, walletAccount2].compactMap(\.dAppPermissionId),
        grant: true
      )
    }
    DataController.viewContext.refreshAllObjects()
    XCTAssertTrue(
      polygonDomain.walletPermissions(for: .eth, account: walletAccount.dAppPermissionId!)
    )
    // add permissions for `jupiter` Domain
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(
        forUrl: jupiter,
        coin: .sol,
        accounts: [solanaAccount].compactMap(\.dAppPermissionId),
        grant: true
      )
    }
    DataController.viewContext.refreshAllObjects()
    XCTAssertTrue(
      jupiterDomain.walletPermissions(for: .sol, account: solanaAccount.dAppPermissionId!)
    )
    // add permissions for `cardano` Domain
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(
        forUrl: cardano,
        coin: .ada,
        accounts: [cardanoAccount].compactMap(\.dAppPermissionId),
        grant: true
      )
    }
    DataController.viewContext.refreshAllObjects()
    XCTAssertTrue(
      cardanoDomain.walletPermissions(for: .ada, account: cardanoAccount.dAppPermissionId!)
    )
  }

  func setupService() -> BraveWalletKeyringService {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._allAccounts = { completion in
      completion(
        .init(
          accounts: [
            self.walletAccount,
            self.walletAccount2,
            self.solanaAccount,
            self.cardanoAccount,
          ],
          selectedAccount: self.walletAccount,
          ethDappSelectedAccount: nil,
          solDappSelectedAccount: nil,
          adaDappSelectedAccount: nil
        )
      )
    }
    keyringService._isWalletCreated = { $0(true) }
    keyringService._isLocked = { $0(false) }
    keyringService._isWalletBackedUp = { $0(true) }
    keyringService._addObserver = { _ in }

    return keyringService
  }

  func testFetchSiteConnections() {
    let keyringService = setupService()
    let store = ManageSiteConnectionsStore(
      keyringStore: .init(
        keyringService: keyringService,
        walletService: MockBraveWalletService(),
        rpcService: MockJsonRpcService(),
        walletP3A: TestBraveWalletP3A()
      )
    )
    // Domains added with Ethereum permissions in `setUp()`
    let accountsExpectation = expectation(description: "accounts loaded")
    store.fetchAllAccountsForUnitTesting {
      accountsExpectation.fulfill()
    }
    waitForExpectations(timeout: 1)

    // verify `siteConnections` is updated when `fetchSiteConnections()` is called
    let siteConnectionsExpectation = expectation(description: "siteConnections")
    XCTAssertTrue(store.siteConnections.isEmpty)  // Initial state
    store.$siteConnections
      .dropFirst()
      .first()
      .sink { siteConnections in
        defer { siteConnectionsExpectation.fulfill() }
        XCTAssertEqual(
          siteConnections[0],
          .init(
            url: self.polygon.absoluteString,
            connectedAccounts: [self.walletAccount, self.walletAccount2],
            coin: .eth
          )
        )
        XCTAssertEqual(
          siteConnections[1],
          .init(
            url: self.compound.absoluteString,
            connectedAccounts: [self.walletAccount, self.walletAccount2],
            coin: .eth
          )
        )
        XCTAssertEqual(
          siteConnections[2],
          .init(
            url: self.jupiter.absoluteString,
            connectedAccounts: [self.solanaAccount],
            coin: .sol
          )
        )
        XCTAssertEqual(
          siteConnections[3],
          .init(
            url: self.cardano.absoluteString,
            connectedAccounts: [self.cardanoAccount],
            coin: .ada
          )
        )
      }.store(in: &cancellables)
    store.fetchSiteConnections()

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `removeAllPermissions(from:)` will remove all permissions from the given `SiteConnections` array
  func testRemoveAllPermissions() {
    let keyringService = setupService()
    let store = ManageSiteConnectionsStore(
      keyringStore: .init(
        keyringService: keyringService,
        walletService: MockBraveWalletService(),
        rpcService: MockJsonRpcService(),
        walletP3A: TestBraveWalletP3A()
      )
    )
    // Domains added with Ethereum permissions in `setUp()`
    let accountsExpectation = expectation(description: "accounts loaded")
    store.fetchAllAccountsForUnitTesting {
      accountsExpectation.fulfill()
    }
    waitForExpectations(timeout: 1)

    store.fetchSiteConnections()
    XCTAssertEqual(store.siteConnections.count, 4)

    // remove permissions
    let siteConnectionToRemove = store.siteConnections[0]
    backgroundSaveAndWaitForExpectation {
      store.removeAllPermissions(from: [siteConnectionToRemove])
    }
    DataController.viewContext.refreshAllObjects()

    // verify `siteConnections` is updated
    XCTAssertEqual(store.siteConnections.count, 3)
    XCTAssertNotEqual(store.siteConnections[0].url, siteConnectionToRemove.url)
    // verify `Domain` data is removed
    let domain = Domain.getOrCreate(
      forUrl: URL(string: siteConnectionToRemove.url)!,
      persistent: true
    )
    DataController.viewContext.refreshAllObjects()
    XCTAssertFalse(domain.walletPermissions(for: .eth, account: walletAccount.dAppPermissionId!))
    XCTAssertFalse(domain.walletPermissions(for: .eth, account: walletAccount2.dAppPermissionId!))
  }

  /// Test `removePermissions(from:url:)` will remove the given account permissions for the given url
  func testRemovePermissions() {
    let keyringService = setupService()
    let store = ManageSiteConnectionsStore(
      keyringStore: .init(
        keyringService: keyringService,
        walletService: MockBraveWalletService(),
        rpcService: MockJsonRpcService(),
        walletP3A: TestBraveWalletP3A()
      )
    )
    // Domains added with Ethereum permissions in `setUp()`
    let accountsExpectation = expectation(description: "accounts loaded")
    store.fetchAllAccountsForUnitTesting {
      accountsExpectation.fulfill()
    }
    waitForExpectations(timeout: 1)

    store.fetchSiteConnections()
    XCTAssertEqual(store.siteConnections.count, 4)

    // remove some account permissions but not all
    let siteConnectionToRemoveAccount = store.siteConnections[1]
    XCTAssertEqual(siteConnectionToRemoveAccount.connectedAccounts.count, 2)
    backgroundSaveAndWaitForExpectation {
      store.removePermissions(
        for: .eth,
        from: [self.walletAccount].compactMap(\.dAppPermissionId),
        url: URL(string: siteConnectionToRemoveAccount.url)!
      )
    }
    DataController.viewContext.refreshAllObjects()

    // verify `siteConnections` is updated to remove specific accounts from the `SiteConnection`
    XCTAssertEqual(store.siteConnections.count, 4)
    XCTAssertEqual(store.siteConnections[1].connectedAccounts.count, 1)
    XCTAssertFalse(store.siteConnections[1].connectedAccounts.contains(walletAccount))

    // verify `Domain` data is updated
    let domain = Domain.getOrCreate(
      forUrl: URL(string: siteConnectionToRemoveAccount.url)!,
      persistent: true
    )
    DataController.viewContext.refreshAllObjects()
    XCTAssertFalse(domain.walletPermissions(for: .eth, account: walletAccount.dAppPermissionId!))
  }

  /// Test `removePermissions(from:url:)` will remove the `SiteConnection` from `siteConnections` when the last connected account is removed
  func testRemovePermissionsLastPermission() {
    let keyringService = setupService()
    let store = ManageSiteConnectionsStore(
      keyringStore: .init(
        keyringService: keyringService,
        walletService: MockBraveWalletService(),
        rpcService: MockJsonRpcService(),
        walletP3A: TestBraveWalletP3A()
      )
    )
    // Domains added with Ethereum permissions in `setUp()`
    let accountsExpectation = expectation(description: "accounts loaded")
    store.fetchAllAccountsForUnitTesting {
      accountsExpectation.fulfill()
    }
    waitForExpectations(timeout: 1)

    store.fetchSiteConnections()
    XCTAssertEqual(store.siteConnections.count, 4)

    // remove `walletAccount` permissions for this `SiteConnection`
    let siteConnectionToRemove = store.siteConnections[0]
    XCTAssertEqual(siteConnectionToRemove.connectedAccounts.count, 2)
    backgroundSaveAndWaitForExpectation {
      store.removePermissions(
        for: .eth,
        from: [self.walletAccount].compactMap(\.dAppPermissionId),
        url: URL(string: siteConnectionToRemove.url)!
      )
    }
    DataController.viewContext.refreshAllObjects()
    XCTAssertEqual(store.siteConnections[0].connectedAccounts.count, 1)

    // remove `walletAccount2` permissions for this `SiteConnection`
    backgroundSaveAndWaitForExpectation {
      store.removePermissions(
        for: .eth,
        from: [self.walletAccount2].compactMap(\.dAppPermissionId),
        url: URL(string: siteConnectionToRemove.url)!
      )
    }
    DataController.viewContext.refreshAllObjects()

    // verify `siteConnections` is updated to remove the `SiteConnection` as all `connectedAddresses` are removed
    XCTAssertEqual(store.siteConnections.count, 3)
    XCTAssertNotEqual(store.siteConnections[0].url, siteConnectionToRemove.url)

    // verify `Domain` data is removed
    let domain = Domain.getOrCreate(
      forUrl: URL(string: siteConnectionToRemove.url)!,
      persistent: true
    )
    DataController.viewContext.refreshAllObjects()
    XCTAssertFalse(domain.walletPermissions(for: .eth, account: walletAccount.dAppPermissionId!))
  }
}
