// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Combine
import BraveCore
import Preferences
@testable import BraveWallet

@MainActor class NetworkStoreTests: XCTestCase {
  
  override func setUp() {
    Preferences.Wallet.showTestNetworks.value = true
  }
  override func tearDown() {
    Preferences.Wallet.showTestNetworks.reset()
  }
  
  private var cancellables: Set<AnyCancellable> = .init()
  
  private func setupServices() -> (BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService, BraveWallet.TestBraveWalletService, BraveWallet.TestSwapService) {
    let currentNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let currentChainId = currentNetwork.chainId
    let allNetworks: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = [
      .eth: [.mockMainnet, .mockGoerli, .mockSepolia, .mockPolygon, .mockCustomNetwork],
      .sol: [.mockSolana, .mockSolanaTestnet],
      .fil: [.mockFilecoinMainnet, .mockFilecoinTestnet]
    ]
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { $0(false) }
    keyringService._allAccounts = { completion in
      completion(.init(
        accounts: [.previewAccount],
        selectedAccount: .previewAccount,
        ethDappSelectedAccount: .previewAccount,
        solDappSelectedAccount: nil
      ))
    }
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._chainIdForOrigin = { $2(currentChainId) }
    rpcService._network = { $2(currentNetwork) }
    rpcService._allNetworks = { coinType, completion in
      completion(allNetworks[coinType, default: []])
    }
    rpcService._setNetwork = { chainId, coin, origin, completion in
      completion(true)
    }
    rpcService._customNetworks = { $1([BraveWallet.NetworkInfo.mockCustomNetwork.chainId]) }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._ensureSelectedAccountForChain = { coin, chainId, completion in
      completion(BraveWallet.AccountInfo.previewAccount.accountId)
    }
    
    let swapService = BraveWallet.TestSwapService()
    swapService._isSwapSupported = { $1(true) }
    
    return (keyringService, rpcService, walletService, swapService)
  }
  
  func testSetSelectedNetwork() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let store = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager()
    )
    await store.setup()
    
    XCTAssertNotEqual(store.defaultSelectedChainId, BraveWallet.NetworkInfo.mockGoerli.chainId)
    let error = await store.setSelectedChain(.mockGoerli, isForOrigin: false)
    XCTAssertNil(error, "Expected success, accounts exist for ethereum")
    XCTAssertEqual(store.defaultSelectedChainId, BraveWallet.NetworkInfo.mockGoerli.chainId)
  }
  
  func testSetSelectedNetworkSameNetwork() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let store = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager()
    )
    await store.setup()
    
    XCTAssertEqual(store.defaultSelectedChainId, BraveWallet.NetworkInfo.mockMainnet.chainId)
    let error = await store.setSelectedChain(.mockMainnet, isForOrigin: false)
    XCTAssertEqual(error, .chainAlreadySelected, "Expected chain already selected error")
    XCTAssertEqual(store.defaultSelectedChainId, BraveWallet.NetworkInfo.mockMainnet.chainId)
  }
  
  /// Test `setSelectedChain` will call `setNetwork` with the store's `origin: URLOrigin?` value.
  func testSetSelectedNetworkWithOrigin() async {
    let origin: URLOrigin = .init(url: URL(string: "https://brave.com")!)
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    rpcService._setNetwork = { chainId, coin, origin, completion in
      XCTAssertEqual(origin, origin)
      completion(true)
    }
    
    let store = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager(),
      origin: origin
    )
    await store.setup()
    
    XCTAssertNotEqual(store.selectedChainIdForOrigin, BraveWallet.NetworkInfo.mockGoerli.chainId)
    let error = await store.setSelectedChain(.mockGoerli, isForOrigin: true)
    XCTAssertNil(error, "Expected success")
    XCTAssertEqual(store.selectedChainIdForOrigin, BraveWallet.NetworkInfo.mockGoerli.chainId)
  }
  
  func testSetSelectedNetworkNoAccounts() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let store = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager()
    )
    await store.setup()
    
    let error = await store.setSelectedChain(.mockSolana, isForOrigin: false)
    XCTAssertEqual(error, .selectedChainHasNoAccounts, "Expected chain has no accounts error")
    XCTAssertNotEqual(store.defaultSelectedChainId, BraveWallet.NetworkInfo.mockSolana.chainId)
    
    let selectFilecoinMainnetError = await store.setSelectedChain(.mockFilecoinMainnet, isForOrigin: false)
    XCTAssertEqual(selectFilecoinMainnetError, .selectedChainHasNoAccounts, "Expected chain has no accounts error")
    XCTAssertNotEqual(store.defaultSelectedChainId, BraveWallet.NetworkInfo.mockFilecoinMainnet.chainId)
  }
  
  func testUpdateChainList() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let store = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager()
    )
    
    let expectedAllChains: [BraveWallet.NetworkInfo] = [
      .mockSolana,
      .mockSolanaTestnet,
      .mockMainnet,
      .mockGoerli,
      .mockSepolia,
      .mockPolygon,
      .mockCustomNetwork,
      .mockFilecoinMainnet,
      .mockFilecoinTestnet
    ]
    
    let expectedCustomChains: [BraveWallet.NetworkInfo] = [
      .mockCustomNetwork
    ]
    
    // wait for all chains to populate
    let allChainsExpectation = expectation(description: "networkStore-allChains")
    store.$allChains
      .dropFirst()
      .sink { allChains in
        defer { allChainsExpectation.fulfill() }
        XCTAssertEqual(allChains.count, expectedAllChains.count)
        XCTAssertTrue(allChains.allSatisfy(expectedAllChains.contains(_:)))
      }
      .store(in: &cancellables)
    
    // wait for all chains to populate
    let customChainsExpectation = expectation(description: "networkStore-customChains")
    store.$customChains
      .dropFirst()
      .sink { customChains in
        defer { customChainsExpectation.fulfill() }
        XCTAssertEqual(customChains, expectedCustomChains)
      }
      .store(in: &cancellables)
    
    await store.setup()
    
    await fulfillment(of: [allChainsExpectation, customChainsExpectation], timeout: 1)
  }
}

private extension BraveWallet.NetworkInfo {
  static var mockCustomNetwork: BraveWallet.NetworkInfo = .init(
    chainId: "0x987654321",
    chainName: "Custom Test Network",
    blockExplorerUrls: [],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [],
    symbol: "TEST",
    symbolName: "TEST",
    decimals: 18,
    coin: .eth,
    supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:)),
    isEip1559: false
  )
}
