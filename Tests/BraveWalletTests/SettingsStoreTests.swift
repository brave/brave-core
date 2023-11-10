// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Combine
import BraveCore
import Preferences
import BigNumber
@testable import BraveWallet

class SettingsStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()
  
  /// Sets up TestKeyringService, TestBraveWalletService and TestTxService with some default values.
  private func setupServices() -> (BraveWallet.TestKeyringService, BraveWallet.TestBraveWalletService, BraveWallet.TestJsonRpcService, BraveWallet.TestTxService, IpfsAPI) {
    let mockUserAssets: [BraveWallet.BlockchainToken] = [.previewToken.copy(asVisibleAsset: true)]
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { $0(false) }
    keyringService._setAutoLockMinutes = { _, _ in }
    keyringService._autoLockMinutes = { $0(5) } // default is 5mins
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = { _, _, completion in
      completion(mockUserAssets)
    }
    walletService._addObserver = { _ in }
    walletService._setDefaultBaseCurrency = { _ in }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) } // default is USD
    walletService._nftDiscoveryEnabled = { _ in }
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._ensResolveMethod = { $0(.ask) }
    rpcService._setEnsResolveMethod = { _ in }
    rpcService._ensOffchainLookupResolveMethod = { $0(.ask) }
    rpcService._setEnsOffchainLookupResolveMethod = { _ in }
    rpcService._snsResolveMethod = { $0(.ask) }
    rpcService._setSnsResolveMethod = { _ in }
    rpcService._unstoppableDomainsResolveMethod = { $0(.ask) }
    rpcService._setUnstoppableDomainsResolveMethod = { _ in }
    
    let txService = BraveWallet.TestTxService()
    
    let ipfsApi = TestIpfsAPI()
    
    return (keyringService, walletService, rpcService, txService, ipfsApi)
  }
  
  /// Test `setup` will populate default values from keyring service / wallet service
  func testSetup() {
    let (keyringService, walletService, rpcService, txService, ipfsApi) = setupServices()
    keyringService._autoLockMinutes = { $0(1) }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.cad.code) }
    walletService._nftDiscoveryEnabled = { $0(false) }
    rpcService._ensResolveMethod = { $0(.disabled) }
    rpcService._ensOffchainLookupResolveMethod = { $0(.disabled) }
    rpcService._snsResolveMethod = { $0(.disabled) }
    rpcService._unstoppableDomainsResolveMethod = { $0(.disabled) }

    let sut = SettingsStore(
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      txService: txService,
      ipfsApi: ipfsApi,
      keychain: TestableKeychain()
    )
    
    let autoLockIntervalExpectation = expectation(description: "setup-autoLockInterval")
    sut.$autoLockInterval
      .dropFirst()
      .sink { autoLockInterval in
        defer { autoLockIntervalExpectation.fulfill() }
        XCTAssertEqual(autoLockInterval, .minute)
      }
      .store(in: &cancellables)
    
    let currencyCodeExpectation = expectation(description: "setup-currencyCode")
    sut.$currencyCode
      .dropFirst()
      .sink { currencyCode in
        defer { currencyCodeExpectation.fulfill() }
        XCTAssertEqual(currencyCode, CurrencyCode.cad)
      }
      .store(in: &cancellables)
    let nftDiscoveryExpectation = expectation(description: "setup-nftDiscovery")
    sut.$isNFTDiscoveryEnabled
      .dropFirst()
      .sink { isNFTDiscoveryEnabled in
        defer { nftDiscoveryExpectation.fulfill() }
        XCTAssertFalse(isNFTDiscoveryEnabled)
      }
      .store(in: &cancellables)
    
    let ensResolveMethodExpectation = expectation(description: "setup-ensResolveMethod")
    sut.$ensResolveMethod
      .dropFirst()
      .sink { ensResolveMethod in
        defer { ensResolveMethodExpectation.fulfill() }
        XCTAssertEqual(ensResolveMethod, .disabled)
      }
      .store(in: &cancellables)
    
    let ensOffchainResolveMethodExpectation = expectation(description: "setup-ensOffchainResolveMethod")
    sut.$ensOffchainResolveMethod
      .dropFirst()
      .sink { ensOffchainResolveMethod in
        defer { ensOffchainResolveMethodExpectation.fulfill() }
        XCTAssertEqual(ensOffchainResolveMethod, .disabled)
      }
      .store(in: &cancellables)
    
    let snsResolveMethodExpectation = expectation(description: "setup-snsResolveMethod")
    sut.$snsResolveMethod
      .dropFirst()
      .sink { snsResolveMethod in
        defer { snsResolveMethodExpectation.fulfill() }
        XCTAssertEqual(snsResolveMethod, .disabled)
      }
      .store(in: &cancellables)
    
    let udResolveMethodExpectation = expectation(description: "setup-udResolveMethod")
    sut.$udResolveMethod
      .dropFirst()
      .sink { udResolveMethod in
        defer { udResolveMethodExpectation.fulfill() }
        XCTAssertEqual(udResolveMethod, .disabled)
      }
      .store(in: &cancellables)
    
    sut.setup()
    
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test `reset()` will call `reset()` on wallet service, update web3 preferences to default values, and update autolock & currency code values.
  func testReset() {
    let (keyringService, walletService, rpcService, txService, ipfsApi) = setupServices()
    var keyringServiceAutolockMinutes: Int32 = 1
    keyringService._autoLockMinutes = { $0(keyringServiceAutolockMinutes) }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.cad.code) }
    
    let keychain = TestableKeychain()
    var resetPasswordInKeychainCalled = false
    keychain._resetPasswordInKeychain = { _ in
      resetPasswordInKeychainCalled = true
      return true
    }

    var walletServiceResetCalled = false
    walletService._reset = {
      walletServiceResetCalled = true
    }

    assert(
      Preferences.Wallet.WalletType.none.rawValue != Preferences.Wallet.defaultEthWallet.defaultValue,
      "Test assumes default wallet for eth value is not `none`")
    Preferences.Wallet.defaultEthWallet.value = Preferences.Wallet.WalletType.none.rawValue
    XCTAssertEqual(
      Preferences.Wallet.defaultEthWallet.value,
      Preferences.Wallet.WalletType.none.rawValue,
      "Failed to update default wallet for eth")
    Preferences.Wallet.allowEthProviderAccess.value = !Preferences.Wallet.allowEthProviderAccess.defaultValue
    XCTAssertEqual(
      Preferences.Wallet.allowEthProviderAccess.value,
      !Preferences.Wallet.allowEthProviderAccess.defaultValue,
      "Failed to update allow ethereum requests")
    
    assert(
      Preferences.Wallet.WalletType.none.rawValue != Preferences.Wallet.defaultSolWallet.defaultValue,
      "Test assumes default wallet for sol value is not `none`")
    Preferences.Wallet.defaultSolWallet.value = Preferences.Wallet.WalletType.none.rawValue
    XCTAssertEqual(
      Preferences.Wallet.defaultSolWallet.value,
      Preferences.Wallet.WalletType.none.rawValue,
      "Failed to update default wallet for sol")
    Preferences.Wallet.allowSolProviderAccess.value = !Preferences.Wallet.allowSolProviderAccess.defaultValue
    XCTAssertEqual(
      Preferences.Wallet.allowSolProviderAccess.value,
      !Preferences.Wallet.allowSolProviderAccess.defaultValue,
      "Failed to update allow solana requests")
    
    Preferences.Wallet.displayWeb3Notifications.value = !Preferences.Wallet.displayWeb3Notifications.defaultValue
    XCTAssertEqual(
      Preferences.Wallet.displayWeb3Notifications.value,
      !Preferences.Wallet.displayWeb3Notifications.defaultValue,
      "Failed to update display web3 notifications")

    let sut = SettingsStore(
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      txService: txService,
      ipfsApi: ipfsApi,
      keychain: keychain
    )
    
    sut.autoLockInterval = .minute
    sut.currencyCode = .cad

    // reset internally in services, mock reset here.
    keyringServiceAutolockMinutes = 5
    
    // Begin test
    sut.reset()
    
    // simulate service observation updates
    sut.autoLockMinutesChanged()
    sut.onDefaultBaseCurrencyChanged(CurrencyCode.usd.code)
    
    XCTAssertEqual(sut.autoLockInterval, .fiveMinutes)
    XCTAssertEqual(sut.currencyCode.code, CurrencyCode.usd.code)

    XCTAssert(
      walletServiceResetCalled,
      "WalletService reset() not called")
    XCTAssertEqual(
      Preferences.Wallet.defaultEthWallet.value,
      Preferences.Wallet.defaultEthWallet.defaultValue,
      "Default Wallet for eth was not reset to default")
    XCTAssertEqual(
      Preferences.Wallet.allowEthProviderAccess.value,
      Preferences.Wallet.allowEthProviderAccess.defaultValue,
      "Allow ethereum requests was not reset to default")
    XCTAssertEqual(
      Preferences.Wallet.defaultSolWallet.value,
      Preferences.Wallet.defaultSolWallet.defaultValue,
      "Default Wallet for sol was not reset to default")
    XCTAssertEqual(
      Preferences.Wallet.allowSolProviderAccess.value,
      Preferences.Wallet.allowSolProviderAccess.defaultValue,
      "Allow solana requests was not reset to default")
    XCTAssertEqual(
      Preferences.Wallet.displayWeb3Notifications.value,
      Preferences.Wallet.displayWeb3Notifications.defaultValue,
      "Display web3 notifications was not reset to default")
    XCTAssert(
      resetPasswordInKeychainCalled,
      "Reset password in keychain was not called")
    /// Testing against `Domain.clearAllEthereumPermissions` has proven flakey
    /// on CI, verified in `ManageSiteConnectionsStoreTests`, `DomainTests`.
  }

  /// Test `resetTransaction()` will call `reset()` on TxService
  func testResetTransaction() {
    let (keyringService, walletService, rpcService, txService, ipfsApi) = setupServices()
    var txServiceResetCalled = false
    txService._reset = {
      txServiceResetCalled = true
    }
    
    let sut = SettingsStore(
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      txService: txService,
      ipfsApi: ipfsApi,
      keychain: TestableKeychain()
    )
    
    sut.resetTransaction()
    
    XCTAssert(txServiceResetCalled, "TxService reset() not called")
  }
}
