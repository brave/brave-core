// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Combine
import BraveCore
@testable import BraveWallet

class NetworkStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()
  
  private func setupServices() -> (BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService, BraveWallet.TestBraveWalletService) {
    let currentNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let currentChainId = currentNetwork.chainId
    let currentSelectedCoin: BraveWallet.CoinType = .eth
    let allNetworks: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = [
      .eth: [.mockMainnet, .mockRinkeby, .mockRopsten, .mockPolygon, .mockCustomNetwork],
      .sol: [.mockSolana, .mockSolanaTestnet]
    ]
    
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
    rpcService._allNetworks = { coinType, completion in
      completion(allNetworks[coinType, default: []])
    }
    rpcService._setNetwork = { _, _, completion in
      completion(true)
    }
    rpcService._customNetworks = { $1([BraveWallet.NetworkInfo.mockCustomNetwork.chainId]) }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._selectedCoin = { $0(currentSelectedCoin) }
    
    return (keyringService, rpcService, walletService)
  }
  
  func testSetSelectedNetwork() async {
    let (keyringService, rpcService, walletService) = setupServices()
    
    let store = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService
    )
    
    let error = await store.setSelectedChain(.mockRopsten)
    XCTAssertNil(error, "Expected success, accounts exist for ethereum")
  }
  
  func testSetSelectedNetworkSameNetwork() async {
    let (keyringService, rpcService, walletService) = setupServices()
    
    let store = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService
    )
    
    let error = await store.setSelectedChain(.mockMainnet)
    XCTAssertEqual(error, .chainAlreadySelected, "Expected chain already selected error")
  }
  
  func testSetSelectedNetworkNoAccounts() async {
    let (keyringService, rpcService, walletService) = setupServices()
    
    let store = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService
    )
    
    let error = await store.setSelectedChain(.mockSolana)
    XCTAssertEqual(error, .selectedChainHasNoAccounts, "Expected chain has no accounts error")
  }
  
  func testUpdateChainList() {
    WalletDebugFlags.isSolanaEnabled = true
    let (keyringService, rpcService, walletService) = setupServices()
    
    let store = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService
    )
    
    let expectedAllChains: [BraveWallet.NetworkInfo] = [
      .mockSolana,
      .mockSolanaTestnet,
      .mockMainnet,
      .mockRinkeby,
      .mockRopsten,
      .mockPolygon,
      .mockCustomNetwork
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
        XCTAssertEqual(allChains, expectedAllChains)
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
    wait(for: [allChainsExpectation, customChainsExpectation], timeout: 10)
  }
}

private extension BraveWallet.NetworkInfo {
  static var mockCustomNetwork: BraveWallet.NetworkInfo = .init(
    chainId: "0x987654321",
    chainName: "Custom Test Network",
    blockExplorerUrls: [],
    iconUrls: [],
    rpcUrls: [],
    symbol: "TEST",
    symbolName: "TEST",
    decimals: 18,
    coin: .eth,
    data: nil
  )
}
