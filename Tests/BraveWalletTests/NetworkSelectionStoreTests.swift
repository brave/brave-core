// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Combine
import BraveCore
import BraveShared
@testable import BraveWallet

@MainActor class NetworkSelectionStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()

  private let allNetworks: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = [
    .eth: [.mockMainnet, .mockRinkeby, .mockRopsten, .mockPolygon],
    .sol: [.mockSolana, .mockSolanaTestnet]
  ]
  
  private func setupServices() -> (BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService, BraveWallet.TestBraveWalletService, BraveWallet.TestSwapService) {
    let currentNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let currentChainId = currentNetwork.chainId
    let currentSelectedCoin: BraveWallet.CoinType = .eth
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._keyringInfo = { keyringId, completion in
      let isEthereumKeyringId = keyringId == BraveWallet.CoinType.eth.keyringId
      let keyring: BraveWallet.KeyringInfo = .init(
        id: BraveWallet.DefaultKeyringId,
        isKeyringCreated: true,
        isLocked: false,
        isBackedUp: true,
        accountInfos: isEthereumKeyringId ? [.previewAccount] : []
      )
      completion(keyring)
    }
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { $0(false) }
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._chainId = { $1(currentChainId) }
    rpcService._network = { $1(currentNetwork) }
    rpcService._allNetworks = { [weak self] coinType, completion in
      completion(self?.allNetworks[coinType, default: []] ?? [])
    }
    rpcService._setNetwork = { _, _, completion in
      completion(true)
    }
    rpcService._customNetworks = { $1([]) }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._selectedCoin = { $0(currentSelectedCoin) }
    
    let swapService = BraveWallet.TestSwapService()
    swapService._isSwapSupported = { $1(true) }
    
    return (keyringService, rpcService, walletService, swapService)
  }
  
  func testUpdate() {
    Preferences.Wallet.showTestNetworks.value = false

    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService
    )
    
    // wait for all chains to populate in `NetworkStore`
    let allChainsException = expectation(description: "networkStore-allChains")
    networkStore.$allChains
      .dropFirst()
      .sink { allChains in
        allChainsException.fulfill()
      }
      .store(in: &cancellables)
    wait(for: [allChainsException], timeout: 1)
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    XCTAssertTrue(store.primaryNetworks.isEmpty, "Test setup failed, expected empty primary networks")
    XCTAssertTrue(store.secondaryNetworks.isEmpty, "Test setup failed, expected empty secondary networks")
    
    store.update()
    
    let expectedPrimaryNetworks: [NetworkSelectionStore.NetworkPresentation] = [
      .init(network: .mockSolana, subNetworks: [], isPrimaryNetwork: true),
      .init(network: .mockMainnet, subNetworks: [], isPrimaryNetwork: true)
    ]
    let expectedSecondaryNetworks: [NetworkSelectionStore.NetworkPresentation] = [
      .init(network: .mockPolygon, subNetworks: [], isPrimaryNetwork: false)
    ]
    XCTAssertEqual(store.primaryNetworks, expectedPrimaryNetworks, "Unexpected primary networks set")
    XCTAssertEqual(store.secondaryNetworks, expectedSecondaryNetworks, "Unexpected secondary networks set")
  }
  
  func testUpdateTestNetworksEnabled() {
    Preferences.Wallet.showTestNetworks.value = true
    
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService
    )
    
    // wait for all chains to populate in `NetworkStore`
    let allChainsException = expectation(description: "networkStore-allChains")
    networkStore.$allChains
      .dropFirst()
      .sink { allChains in
        allChainsException.fulfill()
      }
      .store(in: &cancellables)
    wait(for: [allChainsException], timeout: 1)
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    XCTAssertTrue(store.primaryNetworks.isEmpty, "Test setup failed, expected empty primary networks")
    XCTAssertTrue(store.secondaryNetworks.isEmpty, "Test setup failed, expected empty secondary networks")
    
    store.update()
    
    let expectedPrimaryNetworks: [NetworkSelectionStore.NetworkPresentation] = [
      .init(network: .mockSolana, subNetworks: [.mockSolana, .mockSolanaTestnet], isPrimaryNetwork: true),
      .init(network: .mockMainnet, subNetworks: [.mockMainnet, .mockRinkeby, .mockRopsten], isPrimaryNetwork: true)
    ]
    let expectedSecondaryNetworks: [NetworkSelectionStore.NetworkPresentation] = [
      .init(network: .mockPolygon, subNetworks: [], isPrimaryNetwork: false)
    ]
    XCTAssertEqual(store.primaryNetworks, expectedPrimaryNetworks, "Unexpected primary networks set")
    XCTAssertEqual(store.secondaryNetworks, expectedSecondaryNetworks, "Unexpected secondary networks set")
  }
  
  func testSetSelectedNetwork() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService
    )
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    let success = await store.selectNetwork(network: .mockRopsten)
    XCTAssertTrue(success, "Expected success for selecting Ropsten because we have ethereum accounts.")
    XCTAssertNil(store.detailNetwork, "Expected to reset detail network to nil to pop detail view")
  }
  
  func testSetSelectedNetworkNoAccounts() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService
    )
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    let success = await store.selectNetwork(network: .mockSolana)
    XCTAssertFalse(success, "Expected failure for selecting Solana because we have no Solana accounts.")
    XCTAssertTrue(store.isPresentingNextNetworkAlert, "Expected to set isPresentingNextNetworkAlert to true to show alert asking user to create Solana account")
    XCTAssertNil(store.detailNetwork, "Expected to reset detail network to nil to pop detail view")
  }
  
  func testAlertResponseCreateAccount() {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService
    )
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    store.detailNetwork = .init(network: .mockSolana, subNetworks: [.mockSolana], isPrimaryNetwork: true)
    
    store.handleCreateAccountAlertResponse(shouldCreateAccount: true)
    
    XCTAssertFalse(store.isPresentingNextNetworkAlert, "Expected to set isPresentingNextNetworkAlert to false to hide alert")
    XCTAssertTrue(store.isPresentingAddAccount, "Expected to set isPresentingAddAccount to true to present add network")
  }
  
  func testAlertResponseDontCreateAccount() {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService
    )
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    store.detailNetwork = .init(network: .mockSolana, subNetworks: [.mockSolana], isPrimaryNetwork: true)
    store.isPresentingNextNetworkAlert = true
    
    store.handleCreateAccountAlertResponse(shouldCreateAccount: false)
    
    XCTAssertFalse(store.isPresentingNextNetworkAlert, "Expected to set isPresentingNextNetworkAlert to false to hide alert")
    XCTAssertNil(store.nextNetwork, "Expected to reset nextNetwork to nil as user does not want to create an account")
  }
}
