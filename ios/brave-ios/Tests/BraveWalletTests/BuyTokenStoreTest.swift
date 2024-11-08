// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Foundation
import XCTest

@testable import BraveWallet

class BuyTokenStoreTests: XCTestCase {
  private var cancellables: Set<AnyCancellable> = []

  private func setupServices(
    selectedNetwork: BraveWallet.NetworkInfo = .mockMainnet
  ) -> (
    BraveWallet.TestBlockchainRegistry, BraveWallet.TestKeyringService,
    BraveWallet.TestJsonRpcService, BraveWallet.TestBraveWalletService,
    BraveWallet.TestAssetRatioService,
    BraveWallet.TestBitcoinWalletService
  ) {
    let mockTokenList: [BraveWallet.BlockchainToken] = [
      .init(
        contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        name: "Basic Attention Token",
        logo: "",
        isCompressed: false,
        isErc20: true,
        isErc721: false,
        isErc1155: false,
        splTokenProgram: .unsupported,
        isNft: false,
        isSpam: false,
        symbol: "BAT",
        decimals: 18,
        visible: true,
        tokenId: "",
        coingeckoId: "",
        chainId: BraveWallet.MainnetChainId,
        coin: .eth,
        isShielded: false
      ),
      .init(
        contractAddress: "0xB8c77482e45F1F44dE1745F52C74426C631bDD52",
        name: "BNB",
        logo: "",
        isCompressed: false,
        isErc20: true,
        isErc721: false,
        isErc1155: false,
        splTokenProgram: .unsupported,
        isNft: false,
        isSpam: false,
        symbol: "BNB",
        decimals: 18,
        visible: true,
        tokenId: "",
        coingeckoId: "",
        chainId: "",
        coin: .eth,
        isShielded: false
      ),
      .init(
        contractAddress: "0xdac17f958d2ee523a2206206994597c13d831ec7",
        name: "Tether USD",
        logo: "",
        isCompressed: false,
        isErc20: true,
        isErc721: false,
        isErc1155: false,
        splTokenProgram: .unsupported,
        isNft: false,
        isSpam: false,
        symbol: "USDT",
        decimals: 6,
        visible: true,
        tokenId: "",
        coingeckoId: "",
        chainId: "",
        coin: .eth,
        isShielded: false
      ),
      .init(
        contractAddress: "0x57f1887a8bf19b14fc0df6fd9b2acc9af147ea85",
        name: "Ethereum Name Service",
        logo: "",
        isCompressed: false,
        isErc20: false,
        isErc721: true,
        isErc1155: false,
        splTokenProgram: .unsupported,
        isNft: false,
        isSpam: false,
        symbol: "ENS",
        decimals: 1,
        visible: true,
        tokenId: "",
        coingeckoId: "",
        chainId: "",
        coin: .eth,
        isShielded: false
      ),
      .init(
        contractAddress: "0xad6d458402f60fd3bd25163575031acdce07538d",
        name: "DAI Stablecoin",
        logo: "",
        isCompressed: false,
        isErc20: true,
        isErc721: false,
        isErc1155: false,
        splTokenProgram: .unsupported,
        isNft: false,
        isSpam: false,
        symbol: "DAI",
        decimals: 18,
        visible: false,
        tokenId: "",
        coingeckoId: "",
        chainId: "",
        coin: .eth,
        isShielded: false
      ),
      .init(
        contractAddress: "0x7D1AfA7B718fb893dB30A3aBc0Cfc608AaCfeBB0",
        name: "MATIC",
        logo: "",
        isCompressed: false,
        isErc20: true,
        isErc721: false,
        isErc1155: false,
        splTokenProgram: .unsupported,
        isNft: false,
        isSpam: false,
        symbol: "MATIC",
        decimals: 18,
        visible: true,
        tokenId: "",
        coingeckoId: "",
        chainId: "",
        coin: .eth,
        isShielded: false
      ),
    ]
    let mockOnRampCurrencies: [BraveWallet.OnRampCurrency] = [
      .mockUSD,
      .mockEuro,
      .mockCAD,
      .mockGBP,
    ]
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._buyTokens = { $2(mockTokenList) }
    blockchainRegistry._onRampCurrencies = { $0(mockOnRampCurrencies) }

    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
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

    let rpcService = MockJsonRpcService()
    rpcService._network = { $2(selectedNetwork) }

    let walletService = BraveWallet.TestBraveWalletService()

    let buyURL = "https://crypto.sardine.ai/"
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._buyUrlV1 = { _, _, _, _, _, _, completion in
      completion(buyURL, nil)
    }

    let bitcoinWalletService = BraveWallet.TestBitcoinWalletService()

    return (
      blockchainRegistry, keyringService,
      rpcService, walletService,
      assetRatioService, bitcoinWalletService
    )
  }

  @MainActor func testPrefilledToken() async {
    let (
      blockchainRegistry, keyringService,
      rpcService, walletService,
      assetRatioService, bitcoinWalletService
    ) = setupServices()
    var store = BuyTokenStore(
      blockchainRegistry: blockchainRegistry,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil
    )
    XCTAssertNil(store.selectedBuyToken)

    store = BuyTokenStore(
      blockchainRegistry: blockchainRegistry,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .previewToken
    )

    await store.updateInfo()
    XCTAssertEqual(
      store.selectedBuyToken?.symbol.lowercased(),
      BraveWallet.BlockchainToken.previewToken.symbol.lowercased()
    )
  }

  /// Test that given a `prefilledToken` that is not on the current network, the `BuyTokenStore` will switch networks to the `chainId` of the token.
  @MainActor func testPrefilledTokenSwitchNetwork() async {
    var selectedNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let (
      blockchainRegistry, keyringService,
      rpcService, walletService,
      assetRatioService, bitcoinWalletService
    ) = setupServices()
    rpcService._network = { coin, origin, completion in
      completion(selectedNetwork)
    }
    // simulate network switch when `setNetwork` is called
    rpcService._setNetwork = { chainId, coin, origin, completion in
      // verify network switched to SolanaMainnet
      XCTAssertEqual(chainId, BraveWallet.SolanaMainnet)
      selectedNetwork = coin == .eth ? .mockMainnet : .mockSolana
      completion(true)
    }

    let store = BuyTokenStore(
      blockchainRegistry: blockchainRegistry,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: .mockSolToken  // not on mainnet
    )
    await store.updateInfo()
    XCTAssertEqual(
      store.selectedBuyToken?.symbol.lowercased(),
      BraveWallet.BlockchainToken.mockSolToken.symbol.lowercased()
    )
  }

  func testBuyDisabledForTestNetwork() {
    let (
      blockchainRegistry, keyringService,
      rpcService, walletService,
      assetRatioService, bitcoinWalletService
    ) = setupServices(selectedNetwork: .mockSepolia)
    let store = BuyTokenStore(
      blockchainRegistry: blockchainRegistry,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil
    )

    let isSelectedNetworkSupportedExpectation = expectation(
      description: "buyTokenStore-isSelectedNetworkSupported"
    )
    store.$isSelectedNetworkSupported
      .dropFirst()  // initial/default value
      .sink { isSelectedNetworkSupported in
        defer { isSelectedNetworkSupportedExpectation.fulfill() }
        XCTAssertFalse(isSelectedNetworkSupported)
      }
      .store(in: &cancellables)
    wait(for: [isSelectedNetworkSupportedExpectation], timeout: 2)
  }

  func testBuyEnabledForNonTestNetwork() {
    let (
      blockchainRegistry, keyringService,
      rpcService, walletService,
      assetRatioService, bitcoinWalletService
    ) = setupServices(selectedNetwork: .mockMainnet)
    let store = BuyTokenStore(
      blockchainRegistry: blockchainRegistry,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil
    )

    let isSelectedNetworkSupportedExpectation = expectation(
      description: "buyTokenStore-isSelectedNetworkSupported"
    )
    store.$isSelectedNetworkSupported
      .dropFirst()  // initial/default value
      .sink { isSelectedNetworkSupported in
        defer { isSelectedNetworkSupportedExpectation.fulfill() }
        XCTAssertTrue(isSelectedNetworkSupported)
      }
      .store(in: &cancellables)
    wait(for: [isSelectedNetworkSupportedExpectation], timeout: 2)
  }

  @MainActor
  func testOrderedSupportedBuyOptions() async {
    let (
      _, keyringService,
      rpcService, walletService,
      assetRatioService, bitcoinWalletService
    ) = setupServices()
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._buyTokens = {
      if $0 == .ramp {
        $2([.mockSolToken])
      } else {
        $2([.mockUSDCToken])
      }
    }
    blockchainRegistry._onRampCurrencies = {
      $0([
        .init(
          currencyCode: "usd",
          currencyName: "United States Dollar",
          providers: [.init(value: 0)]
        )
      ])
    }

    let store = BuyTokenStore(
      blockchainRegistry: blockchainRegistry,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil
    )

    await store.updateInfo()

    XCTAssertEqual(store.orderedSupportedBuyOptions.count, isTestRunningInUSRegion ? 5 : 4)
    XCTAssertNotNil(store.orderedSupportedBuyOptions.first)
    XCTAssertEqual(store.orderedSupportedBuyOptions.first, .ramp)
    XCTAssertNotNil(store.orderedSupportedBuyOptions[safe: 1])
    XCTAssertEqual(store.orderedSupportedBuyOptions[safe: 1], .sardine)
    XCTAssertNotNil(store.orderedSupportedBuyOptions[safe: 2])
    XCTAssertEqual(store.orderedSupportedBuyOptions[safe: 2], .transak)
    if isTestRunningInUSRegion {
      XCTAssertNotNil(store.orderedSupportedBuyOptions[safe: 3])
      XCTAssertEqual(store.orderedSupportedBuyOptions[safe: 3], .stripe)
      XCTAssertNotNil(store.orderedSupportedBuyOptions[safe: 4])
      XCTAssertEqual(store.orderedSupportedBuyOptions[safe: 4], .coinbase)
    } else {
      XCTAssertNotNil(store.orderedSupportedBuyOptions[safe: 3])
      XCTAssertEqual(store.orderedSupportedBuyOptions[safe: 3], .coinbase)
    }
  }

  @MainActor
  func testAllTokens() async {
    let selectedNetwork: BraveWallet.NetworkInfo = .mockSolana
    let (
      _, keyringService,
      rpcService, walletService,
      assetRatioService, bitcoinWalletService
    ) = setupServices(selectedNetwork: selectedNetwork)
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._buyTokens = {
      if $0 == .ramp {
        $2([.mockSolToken, .mockSpdToken])
      } else {
        $2([.mockSpdToken])
      }
    }
    blockchainRegistry._onRampCurrencies = {
      $0([
        .init(
          currencyCode: "usd",
          currencyName: "United States Dollar",
          providers: [.init(value: 0)]
        )
      ])
    }

    let store = BuyTokenStore(
      blockchainRegistry: blockchainRegistry,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil
    )

    await store.updateInfo()

    XCTAssertEqual(store.allTokens.count, 2)
    for token in store.allTokens {
      XCTAssertEqual(token.chainId, selectedNetwork.chainId)
    }
  }

  /// Test `supportedProviders` will only return supported `OnRampProvider`s for the `selectedCurrency`.
  @MainActor func testSupportedProvidersSelectedCurrency() async {
    let (
      blockchainRegistry, keyringService,
      rpcService, walletService,
      assetRatioService, bitcoinWalletService
    ) = setupServices()
    blockchainRegistry._onRampCurrencies = {
      $0([.mockUSD, .mockCAD, .mockGBP, .mockEuro])
    }

    let store = BuyTokenStore(
      blockchainRegistry: blockchainRegistry,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: nil
    )
    await store.updateInfo()

    // Test USD. Ramp, Sardine, Transak, Stripe (US locale only), Coinbase are all supported.
    store.selectedCurrency = .mockUSD
    // some providers are only available in the US. Check ourselves instead of swizzling `regionCode` / `region`.
    if isTestRunningInUSRegion {
      XCTAssertEqual(store.supportedProviders.count, 5)
      XCTAssertEqual(
        store.supportedProviders,
        [
          BraveWallet.OnRampProvider.ramp,
          BraveWallet.OnRampProvider.sardine,
          BraveWallet.OnRampProvider.transak,
          BraveWallet.OnRampProvider.stripe,
          BraveWallet.OnRampProvider.coinbase,
        ]
      )
    } else {
      // stripe only supported in en-us locale
      XCTAssertEqual(store.supportedProviders.count, 4)
      XCTAssertEqual(
        store.supportedProviders,
        [
          BraveWallet.OnRampProvider.ramp,
          BraveWallet.OnRampProvider.sardine,
          BraveWallet.OnRampProvider.transak,
          BraveWallet.OnRampProvider.coinbase,
        ]
      )
    }

    // Test CAD. Ramp, Sardine, Transak, Coinbase are supported. Stripe unsupported.
    store.selectedCurrency = .mockCAD
    XCTAssertEqual(store.supportedProviders.count, 4)
    XCTAssertEqual(
      store.supportedProviders,
      [
        BraveWallet.OnRampProvider.ramp,
        BraveWallet.OnRampProvider.sardine,
        BraveWallet.OnRampProvider.transak,
        BraveWallet.OnRampProvider.coinbase,
      ]
    )
    XCTAssertFalse(store.supportedProviders.contains(.stripe))

    // Test GBP. Ramp, Sardine, Coinbase supported. Transak, Stripe unsupported.
    store.selectedCurrency = .mockGBP
    XCTAssertEqual(store.supportedProviders.count, 3)
    XCTAssertEqual(
      store.supportedProviders,
      [
        BraveWallet.OnRampProvider.ramp,
        BraveWallet.OnRampProvider.sardine,
        BraveWallet.OnRampProvider.coinbase,
      ]
    )
    XCTAssertFalse(store.supportedProviders.contains(.transak))
    XCTAssertFalse(store.supportedProviders.contains(.stripe))
  }

  private var isTestRunningInUSRegion: Bool {
    Locale.current.safeRegionCode?.caseInsensitiveCompare("us") == .orderedSame
  }
}

extension BraveWallet.OnRampCurrency {
  fileprivate static var mockUSD: BraveWallet.OnRampCurrency {
    .init(
      currencyCode: "usd",
      currencyName: "United States Dollar",
      providers: WalletConstants.supportedOnRampProviders.map {
        .init(integerLiteral: $0.rawValue)
      }
    )
  }

  fileprivate static var mockCAD: BraveWallet.OnRampCurrency {
    .init(
      currencyCode: "cad",
      currencyName: "Canadian Dollar",
      providers: [
        BraveWallet.OnRampProvider.ramp,
        BraveWallet.OnRampProvider.sardine,
        BraveWallet.OnRampProvider.transak,
        BraveWallet.OnRampProvider.coinbase,
          // simulate stripe not supported for CAD
      ].map {
        .init(integerLiteral: $0.rawValue)
      }
    )
  }

  fileprivate static var mockGBP: BraveWallet.OnRampCurrency {
    .init(
      currencyCode: "gbp",
      currencyName: "British Pound Sterling",
      providers: [
        BraveWallet.OnRampProvider.ramp,
        BraveWallet.OnRampProvider.sardine,
        BraveWallet.OnRampProvider.coinbase,
          // simulate transak not supported for GBP
          // simulate stripe not supported for GBP
      ].map {
        .init(integerLiteral: $0.rawValue)
      }
    )
  }

  fileprivate static var mockEuro: BraveWallet.OnRampCurrency {
    .init(
      currencyCode: "eur",
      currencyName: "Euro",
      providers: WalletConstants.supportedOnRampProviders.map {
        .init(integerLiteral: $0.rawValue)
      }
    )
  }
}
