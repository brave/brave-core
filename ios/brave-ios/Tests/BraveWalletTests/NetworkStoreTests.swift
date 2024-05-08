// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Preferences
import XCTest

@testable import BraveWallet

@MainActor class NetworkStoreTests: XCTestCase {

  private var cancellables: Set<AnyCancellable> = .init()

  private func setupServices() -> (
    BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService,
    BraveWallet.TestBraveWalletService, BraveWallet.TestSwapService
  ) {
    let currentNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let currentChainId = currentNetwork.chainId
    let allNetworks: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = [
      .eth: [.mockMainnet, .mockGoerli, .mockSepolia, .mockPolygon, .mockCustomNetwork],
      .sol: [.mockSolana, .mockSolanaTestnet],
      .fil: [.mockFilecoinMainnet, .mockFilecoinTestnet],
      .btc: [.mockBitcoinMainnet],
    ]

    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { $0(false) }
    keyringService._allAccounts = { completion in
      completion(
        .init(
          accounts: [.previewAccount],
          selectedAccount: .previewAccount,
          ethDappSelectedAccount: .previewAccount,
          solDappSelectedAccount: nil
        )
      )
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
    rpcService._hiddenNetworks = { $1([]) }
    rpcService._addHiddenNetwork = { $2(true) }
    rpcService._removeHiddenNetwork = { $2(true) }

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
    keyringService._allAccounts = { completion in
      completion(
        .init(
          accounts: [.previewAccount, .mockFilAccount],
          selectedAccount: .previewAccount,
          ethDappSelectedAccount: .previewAccount,
          solDappSelectedAccount: nil
        )
      )
    }

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

    // Verify `supportedKeyrings` is checked (Mainnet account exists, no testnet account)
    let selectFilecoinMainnetError = await store.setSelectedChain(
      .mockFilecoinTestnet,
      isForOrigin: false
    )
    XCTAssertEqual(
      selectFilecoinMainnetError,
      .selectedChainHasNoAccounts,
      "Expected chain has no accounts error"
    )
    XCTAssertNotEqual(
      store.defaultSelectedChainId,
      BraveWallet.NetworkInfo.mockFilecoinTestnet.chainId
    )
  }

  func testUpdateChainList() async {
    let (keyringService, rpcService, walletService, swapService) = setupServices()
    rpcService._hiddenNetworks = { coin, completion in
      if coin == .eth {
        completion(
          [
            BraveWallet.NetworkInfo.mockGoerli.chainId,
            BraveWallet.NetworkInfo.mockSepolia.chainId,
            BraveWallet.NetworkInfo.mockPolygon.chainId,
          ]
        )
      } else {
        completion([])
      }
    }
    rpcService._network = { coin, _, completion in
      switch coin {
      case .eth:
        completion(.mockMainnet)
      case .sol:
        completion(.mockSolana)
      case .fil:
        completion(.mockFilecoinMainnet)
      case .btc:
        completion(.mockBitcoinMainnet)
      case .zec:
        fallthrough
      @unknown default:
        completion(.mockMainnet)
      }
    }

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
      .mockFilecoinTestnet,
      .mockBitcoinMainnet,
    ]

    let expectedCustomChains: [BraveWallet.NetworkInfo] = [
      .mockCustomNetwork
    ]

    let expectedHiddenChains: [BraveWallet.NetworkInfo] = [
      .mockGoerli,
      .mockSepolia,
      .mockPolygon,
    ]

    let expectedDefaultNetworks: [BraveWallet.CoinType: BraveWallet.NetworkInfo] = [
      .eth: .mockMainnet,
      .sol: .mockSolana,
      .fil: .mockFilecoinMainnet,
      .btc: .mockBitcoinMainnet,
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

    let defaultNetworksExpectation = expectation(description: "networkStore-defaultNetworks")
    store.$defaultNetworks
      .dropFirst()
      .collect(4)
      .sink { defaultNetworks in
        defer { defaultNetworksExpectation.fulfill() }
        guard let lastUpdatedDefaultNetworks = defaultNetworks.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedDefaultNetworks.count, expectedDefaultNetworks.count)
        for coin in lastUpdatedDefaultNetworks.keys {
          XCTAssertEqual(
            lastUpdatedDefaultNetworks[coin]?.chainId,
            expectedDefaultNetworks[coin]?.chainId
          )
        }
      }
      .store(in: &cancellables)

    let hiddenChainsExpectation = expectation(description: "networkStore-hiddenChains")
    store.$hiddenChains
      .dropFirst()
      .sink { hiddenChains in
        defer { hiddenChainsExpectation.fulfill() }
        XCTAssertEqual(hiddenChains.count, expectedHiddenChains.count)
        XCTAssertTrue(expectedHiddenChains.allSatisfy(expectedHiddenChains.contains(_:)))
      }
      .store(in: &cancellables)

    await store.updateChainList()

    await fulfillment(
      of: [
        allChainsExpectation, customChainsExpectation,
        hiddenChainsExpectation, defaultNetworksExpectation,
      ],
      timeout: 1
    )
  }
}

extension BraveWallet.NetworkInfo {
  fileprivate static var mockCustomNetwork: BraveWallet.NetworkInfo = .init(
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
