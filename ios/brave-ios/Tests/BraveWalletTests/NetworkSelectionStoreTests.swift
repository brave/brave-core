// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Combine
import BraveCore
import Preferences
@testable import BraveWallet

@MainActor class NetworkSelectionStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()

  private let allNetworks: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = [
    .eth: [.mockMainnet, .mockGoerli, .mockSepolia, .mockPolygon],
    .sol: [.mockSolana, .mockSolanaTestnet],
    .fil: [.mockFilecoinTestnet]
  ]
  
  private func setupServices() -> (BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService, BraveWallet.TestBraveWalletService, BraveWallet.TestSwapService) {
    let currentNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let currentChainId = currentNetwork.chainId
    
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
    rpcService._allNetworks = { [weak self] coinType, completion in
      completion(self?.allNetworks[coinType, default: []] ?? [])
    }
    rpcService._setNetwork = { chainId, coin, origin, completion in
      completion(true)
    }
    rpcService._customNetworks = { $1([]) }
    
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
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager()
    )
    await networkStore.setup()
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    let success = await store.selectNetwork(.mockGoerli)
    XCTAssertTrue(success, "Expected success for selecting Goerli because we have ethereum accounts.")
  }
  
  /// Test `setSelectedChain` will call `setNetwork` with the `Mode.select(isForOrigin: true)`.
  func testSetSelectedNetworkPanel() async {
    let origin: URLOrigin = .init(url: URL(string: "https://brave.com")!)
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    rpcService._setNetwork = { chainId, coin, origin, completion in
      XCTAssertEqual(origin, origin)
      completion(true)
    }
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager(),
      origin: origin
    )
    await networkStore.setup()
    
    let store = NetworkSelectionStore(
      mode: .select(isForOrigin: true),
      networkStore: networkStore
    )
    let success = await store.selectNetwork(.mockGoerli)
    XCTAssertTrue(success, "Expected success for selecting Goerli because we have ethereum accounts.")
  }
  
  func testSetSelectedNetworkNoAccounts() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager()
    )
    await networkStore.setup()
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    let success = await store.selectNetwork(.mockSolana)
    XCTAssertFalse(success, "Expected failure for selecting Solana because we have no Solana accounts.")
    XCTAssertTrue(store.isPresentingNextNetworkAlert, "Expected to set isPresentingNextNetworkAlert to true to show alert asking user to create Solana account")
  }
  
  func testSelectedNetworkFormSelectionMode() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager()
    )
    await networkStore.setup()
    
    let store = NetworkSelectionStore(mode: .formSelection, networkStore: networkStore)
    let success = await store.selectNetwork(.mockGoerli)
    XCTAssertTrue(success, "Expected success for selecting Goerli")
    XCTAssertEqual(store.networkSelectionInForm, .mockGoerli)
  }
  
  func testAlertResponseCreateAccount() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager()
    )
    await networkStore.setup()
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    
    store.handleCreateAccountAlertResponse(shouldCreateAccount: true)
    
    XCTAssertFalse(store.isPresentingNextNetworkAlert, "Expected to set isPresentingNextNetworkAlert to false to hide alert")
    XCTAssertTrue(store.isPresentingAddAccount, "Expected to set isPresentingAddAccount to true to present add network")
  }
  
  func testAlertResponseDontCreateAccount() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager()
    )
    await networkStore.setup()
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    store.isPresentingNextNetworkAlert = true
    
    store.handleCreateAccountAlertResponse(shouldCreateAccount: false)
    
    XCTAssertFalse(store.isPresentingNextNetworkAlert, "Expected to set isPresentingNextNetworkAlert to false to hide alert")
    XCTAssertNil(store.nextNetwork, "Expected to reset nextNetwork to nil as user does not want to create an account")
  }
  
  func testDismissAddAccount() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    
    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager()
    )
    await networkStore.setup()
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    
    let didSwitchNetworks = await store.handleDismissAddAccount()
    XCTAssertFalse(didSwitchNetworks, "Expected to not switch networks as no account was created")
  }
  
  func testDismissAddAccountAfterCreation() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()

    var accountInfosDict: [BraveWallet.CoinType: [BraveWallet.AccountInfo]] = [
      .eth: [.mockEthAccount]
    ]
    
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { $0(false) }
    keyringService._allAccounts = { completion in
      completion(.init(
        accounts: accountInfosDict.values.flatMap { $0 },
        selectedAccount: accountInfosDict[.eth]?.first,
        ethDappSelectedAccount: accountInfosDict[.eth]?.first,
        solDappSelectedAccount: accountInfosDict[.sol]?.first
      ))
    }

    let networkStore = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: TestableWalletUserAssetManager()
    )
    await networkStore.setup()
    
    let store = NetworkSelectionStore(networkStore: networkStore)
    
    let success = await store.selectNetwork(.mockSolana)
    XCTAssertFalse(success, "Expected failure to select network due to no accounts")
    XCTAssertTrue(store.isPresentingNextNetworkAlert, "Expected to present next network alert")
    
    store.handleCreateAccountAlertResponse(shouldCreateAccount: true)
    
    XCTAssertFalse(store.isPresentingNextNetworkAlert, "Expected to set isPresentingNextNetworkAlert to false to hide alert")
    XCTAssertTrue(store.isPresentingAddAccount, "Expected to set isPresentingAddAccount to true to present add network")
    
    // simulate an account created
    accountInfosDict[.sol] = [.mockSolAccount]
    
    let didSwitchNetworks = await store.handleDismissAddAccount()
    XCTAssertTrue(didSwitchNetworks, "Expected to switch networks as an account was created")
    
    // create filecoin account
    let selectFilecoinMainnetSuccess = await store.selectNetwork(.mockFilecoinMainnet)
    XCTAssertFalse(selectFilecoinMainnetSuccess, "Expected failure to select network due to no accounts")
    XCTAssertTrue(store.isPresentingNextNetworkAlert, "Expected to present next network alert")
    
    store.handleCreateAccountAlertResponse(shouldCreateAccount: true)
    
    XCTAssertFalse(store.isPresentingNextNetworkAlert, "Expected to set isPresentingNextNetworkAlert to false to hide alert")
    XCTAssertTrue(store.isPresentingAddAccount, "Expected to set isPresentingAddAccount to true to present add network")
    
    // simulate an account created
    accountInfosDict[.fil] = [.mockFilAccount]
    
    let didSwitchToFilecoinMainnet = await store.handleDismissAddAccount()
    XCTAssertTrue(didSwitchToFilecoinMainnet, "Expected to switch networks as an account was created")
  }
}
