// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
import BraveShared
import Preferences
@testable import Data
import BraveShields
import TestHelpers

class DomainTests: CoreDataTestCase {
  let fetchRequest = NSFetchRequest<Domain>(entityName: String(describing: Domain.self))

  // Should match but with different schemes
  let url = URL(string: "http://example.com")!
  let urlHTTPS = URL(string: "https://example.com")!

  let url2 = URL(string: "http://brave.com")!
  let url2HTTPS = URL(string: "https://brave.com")!

  let compound = URL(string: "https://compound.finance")!
  let polygon = URL(string: "https://wallet.polygon.technology")!
  let walletEthAccount = "0x4D60d71F411AB671f614eD0ec5B71bEedB46287d"
  let walletEthAccount2 = "0x4d60d71f411ab671f614ed0ec5b71beedb46287d"
  
  let magicEden = URL(string: "https://magiceden.io")!
  let raydium = URL(string: "https://raydium.io")!
  let walletSolAccount = "8uMdzdJ6Mtsh2vGhJJGCfwo8PZrnZZ3qbT85wbo1o4xt"
  let walletSolAccount2 = "8umdzdJ6Mtsh2vFhJJGCfwo8PZrnZZ3ceT85wbo1o4dt"

  private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
    return NSEntityDescription.entity(forEntityName: String(describing: Domain.self), in: context)!
  }

  func testGetOrCreate() {
    XCTAssertNotNil(Domain.getOrCreate(forUrl: url, persistent: true))
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)

    // Try to add the same domain again, verify no new object is created
    XCTAssertNotNil(Domain.getOrCreate(forUrl: url, persistent: true))
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)

    // Add another domain, verify that second object is created
    XCTAssertNotNil(Domain.getOrCreate(forUrl: url2, persistent: true))
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 2)
  }

  func testGetOrCreateURLs() {
    // This also validates that the schemes are being correctly saved
    XCTAssertEqual(url.absoluteString, Domain.getOrCreate(forUrl: url, persistent: true).url)
    XCTAssertEqual(url2.absoluteString, Domain.getOrCreate(forUrl: url2, persistent: true).url)

    let url3 = URL(string: "https://brave.com")!
    let url4 = URL(string: "data://brave.com")!
    XCTAssertEqual(url3.absoluteString, Domain.getOrCreate(forUrl: url3, persistent: true).url)
    XCTAssertEqual(url4.absoluteString, Domain.getOrCreate(forUrl: url4, persistent: true).url)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 4)
  }

  @MainActor func testDefaultShieldSettings() {
    let domain = Domain.getOrCreate(forUrl: url, persistent: true)
    XCTAssertTrue(domain.isShieldExpected(BraveShield.AdblockAndTp, considerAllShieldsOption: true))
    XCTAssertFalse(domain.isShieldExpected(BraveShield.AllOff, considerAllShieldsOption: true))
    XCTAssertFalse(domain.isShieldExpected(BraveShield.NoScript, considerAllShieldsOption: true))
    XCTAssertTrue(domain.isShieldExpected(BraveShield.FpProtection, considerAllShieldsOption: true))

    XCTAssertEqual(domain.bookmarks?.count, 0)
    XCTAssertEqual(domain.url, url.domainURL.absoluteString)
  }

  @MainActor func testAllShieldsOff() {
    let domain = Domain.getOrCreate(forUrl: url, persistent: true)

    backgroundSaveAndWaitForExpectation {
      Domain.setBraveShield(forUrl: url, shield: .AllOff, isOn: true, isPrivateBrowsing: false)
    }

    XCTAssertFalse(domain.isShieldExpected(BraveShield.AdblockAndTp, considerAllShieldsOption: true))
    XCTAssertFalse(domain.isShieldExpected(BraveShield.AllOff, considerAllShieldsOption: true))
    XCTAssertFalse(domain.isShieldExpected(BraveShield.NoScript, considerAllShieldsOption: true))
    XCTAssertFalse(domain.isShieldExpected(BraveShield.FpProtection, considerAllShieldsOption: true))

    backgroundSaveAndWaitForExpectation {
      Domain.setBraveShield(forUrl: url, shield: .AllOff, isOn: false, isPrivateBrowsing: false)
    }

    XCTAssertTrue(domain.isShieldExpected(BraveShield.AdblockAndTp, considerAllShieldsOption: true))
    XCTAssertFalse(domain.isShieldExpected(BraveShield.AllOff, considerAllShieldsOption: true))
    XCTAssertFalse(domain.isShieldExpected(BraveShield.NoScript, considerAllShieldsOption: true))
    XCTAssertTrue(domain.isShieldExpected(BraveShield.FpProtection, considerAllShieldsOption: true))
  }

  /// Tests non-HTTPSE shields
  @MainActor func testNormalShieldSettings() {
    backgroundSaveAndWaitForExpectation {
      Domain.setBraveShield(forUrl: url2HTTPS, shield: .AdblockAndTp, isOn: false, isPrivateBrowsing: false)
    }
    
    let domain = Domain.getOrCreate(forUrl: url2HTTPS, persistent: true)
    // These should be the same in this situation
    XCTAssertFalse(domain.isShieldExpected(BraveShield.AdblockAndTp, considerAllShieldsOption: true))

    backgroundSaveAndWaitForExpectation {
      Domain.setBraveShield(forUrl: url2HTTPS, shield: .AdblockAndTp, isOn: true, isPrivateBrowsing: false)
    }

    domain.managedObjectContext?.refreshAllObjects()
    XCTAssertTrue(domain.isShieldExpected(BraveShield.AdblockAndTp, considerAllShieldsOption: true))
  }

  func testWalletEthDappPermission() {
    let compondDomain = Domain.getOrCreate(forUrl: compound, persistent: true)
    let polygonDomain = Domain.getOrCreate(forUrl: polygon, persistent: true)

    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(forUrl: compound, coin: .eth, accounts: [walletEthAccount], grant: true)
    }

    XCTAssertTrue(compondDomain.walletPermissions(for: .eth, account: walletEthAccount))

    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(forUrl: compound, coin: .eth, accounts: [walletEthAccount2], grant: true)
    }
    XCTAssertTrue(compondDomain.walletPermissions(for: .eth, account: walletEthAccount2))

    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(forUrl: compound, coin: .eth, accounts: [walletEthAccount], grant: false)
    }
    XCTAssertFalse(compondDomain.walletPermissions(for: .eth, account: walletEthAccount))

    XCTAssertNil(polygonDomain.wallet_permittedAccounts)
  }
  
  func testWalletSolanaDappPermission() {
    let magicEdenDomain = Domain.getOrCreate(forUrl: magicEden, persistent: true)
    let raydiumDomain = Domain.getOrCreate(forUrl: raydium, persistent: true)
    
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(forUrl: magicEden, coin: .sol, accounts: [walletSolAccount], grant: true)
    }
    
    XCTAssertTrue(magicEdenDomain.walletPermissions(for: .sol, account: walletSolAccount))
    XCTAssertFalse(magicEdenDomain.walletPermissions(for: .eth, account: walletSolAccount))
    
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(forUrl: magicEden, coin: .sol, accounts: [walletSolAccount2], grant: true)
    }
    XCTAssertTrue(magicEdenDomain.walletPermissions(for: .sol, account: walletSolAccount2))
    XCTAssertFalse(magicEdenDomain.walletPermissions(for: .eth, account: walletSolAccount2))
    
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(forUrl: magicEden, coin: .sol, accounts: [walletSolAccount2], grant: false)
    }
    XCTAssertFalse(magicEdenDomain.walletPermissions(for: .sol, account: walletSolAccount2))
    
    XCTAssertNil(raydiumDomain.wallet_solanaPermittedAcccounts)
  }
  
  func testClearAllWalletPermissions() {
    let compondDomain = Domain.getOrCreate(forUrl: compound, persistent: true)
    let raydiumDomain = Domain.getOrCreate(forUrl: raydium, persistent: true)
    
    // add permissions for `compound` Domain
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(forUrl: compound, coin: .eth, accounts: [walletEthAccount], grant: true)
    }
    XCTAssertTrue(compondDomain.walletPermissions(for: .eth, account: walletEthAccount))
    // add permissions for `raydium` Domain
    backgroundSaveAndWaitForExpectation {
      Domain.setWalletPermissions(forUrl: raydium, coin: .sol, accounts: [walletSolAccount], grant: true)
    }
    XCTAssertTrue(raydiumDomain.walletPermissions(for: .sol, account: walletSolAccount))
    // Remove all eth permissions
    backgroundSaveAndWaitForExpectation {
      Domain.clearAllWalletPermissions(for: .eth)
    }
    XCTAssertFalse(compondDomain.walletPermissions(for: .eth, account: walletEthAccount))
    // Remove all sol permissions
    backgroundSaveAndWaitForExpectation {
      Domain.clearAllWalletPermissions(for: .sol)
    }
    XCTAssertFalse(raydiumDomain.walletPermissions(for: .sol, account: walletSolAccount))
  }
}
