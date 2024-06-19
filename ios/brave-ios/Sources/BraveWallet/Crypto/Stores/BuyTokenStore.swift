// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Foundation
import OrderedCollections

/// A store contains data for buying tokens
public class BuyTokenStore: ObservableObject, WalletObserverStore {
  /// The current selected token to buy. Default with nil value.
  @Published var selectedBuyToken: BraveWallet.BlockchainToken?
  /// The supported currencies for purchasing
  @Published var supportedCurrencies: [BraveWallet.OnRampCurrency] = []
  /// A boolean indicates if the current selected network supports `Buy`
  @Published var isSelectedNetworkSupported: Bool = false
  /// The amount user wishes to purchase
  @Published var buyAmount: String = ""
  /// The currency user wishes to purchase with
  @Published var selectedCurrency: BraveWallet.OnRampCurrency = .init()

  /// A map of list of available tokens to a certain on ramp provider
  var buyTokens: [BraveWallet.OnRampProvider: [BraveWallet.BlockchainToken]]
  /// A list of all available tokens for all providers
  var allTokens: [BraveWallet.BlockchainToken] = []

  /// The supported `OnRampProvider`s for the currently selected currency and device locale.
  var supportedProviders: OrderedSet<BraveWallet.OnRampProvider> {
    return OrderedSet(
      orderedSupportedBuyOptions
        .filter { provider in
          guard let tokens = buyTokens[provider],
            let selectedBuyToken = selectedBuyToken
          else { return false }
          // verify selected currency code is supported for this provider
          guard
            supportedCurrencies.contains(where: { supportedOnRampCurrency in
              guard
                supportedOnRampCurrency.providers.contains(.init(integerLiteral: provider.rawValue))
              else {
                return false
              }
              let selectedCurrencyCode = selectedCurrency.currencyCode
              return supportedOnRampCurrency.currencyCode.caseInsensitiveCompare(
                selectedCurrencyCode
              ) == .orderedSame
            })
          else {
            return false
          }
          // verify selected token is supported for this provider
          return tokens.includes(selectedBuyToken)
        }
    )
  }

  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let bitcoinWalletService: BraveWalletBitcoinWalletService
  private var selectedNetwork: BraveWallet.NetworkInfo = .init()
  private(set) var orderedSupportedBuyOptions: OrderedSet<BraveWallet.OnRampProvider> = []
  private var prefilledToken: BraveWallet.BlockchainToken?
  private var rpcServiceObserver: JsonRpcServiceObserver?
  private var keyringServiceObserver: KeyringServiceObserver?

  /// A map between chain id and gas token's symbol
  static let gasTokens: [String: [String]] = [
    BraveWallet.MainnetChainId: ["eth"],
    BraveWallet.OptimismMainnetChainId: ["eth"],
    BraveWallet.AuroraMainnetChainId: ["eth"],
    BraveWallet.PolygonMainnetChainId: ["matic"],
    BraveWallet.FantomMainnetChainId: ["ftm"],
    BraveWallet.CeloMainnetChainId: ["celo"],
    BraveWallet.BnbSmartChainMainnetChainId: ["bnb"],
    BraveWallet.SolanaMainnet: ["sol"],
    BraveWallet.FilecoinMainnet: ["fil"],
    BraveWallet.AvalancheMainnetChainId: ["avax", "avaxc"],
  ]

  var isObserving: Bool {
    rpcServiceObserver != nil && keyringServiceObserver != nil
  }

  public init(
    blockchainRegistry: BraveWalletBlockchainRegistry,
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    bitcoinWalletService: BraveWalletBitcoinWalletService,
    prefilledToken: BraveWallet.BlockchainToken?
  ) {
    self.blockchainRegistry = blockchainRegistry
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.bitcoinWalletService = bitcoinWalletService
    self.prefilledToken = prefilledToken
    self.buyTokens = WalletConstants.supportedOnRampProviders.reduce(
      into: [BraveWallet.OnRampProvider: [BraveWallet.BlockchainToken]]()
    ) {
      $0[$1] = []
    }

    self.setupObservers()

    Task {
      await updateInfo()
    }
  }

  func tearDown() {
    rpcServiceObserver = nil
    keyringServiceObserver = nil
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.rpcServiceObserver = JsonRpcServiceObserver(
      rpcService: rpcService,
      _chainChangedEvent: { [weak self] _, _, _ in
        Task { [self] in
          await self?.updateInfo()
        }
      }
    )
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _selectedWalletAccountChanged: { [weak self] _ in
        Task { @MainActor [self] in
          await self?.updateInfo()
        }
      }
    )
  }

  @MainActor private func validatePrefilledToken(on network: BraveWallet.NetworkInfo) async {
    guard let prefilledToken = self.prefilledToken else {
      return
    }
    if prefilledToken.coin == network.coin && prefilledToken.chainId == network.chainId {
      // valid for current network
      self.selectedBuyToken = prefilledToken
    } else {
      // need to try and select correct network.
      let allNetworksForTokenCoin = await rpcService.allNetworks()
      guard
        let networkForToken = allNetworksForTokenCoin.first(where: {
          $0.coin == prefilledToken.coin && $0.chainId == prefilledToken.chainId
        })
      else {
        // don't set prefilled token if it belongs to a network we don't know
        return
      }
      let success = await rpcService.setNetwork(
        chainId: networkForToken.chainId,
        coin: networkForToken.coin,
        origin: nil
      )
      if success {
        self.selectedNetwork = networkForToken
        self.selectedBuyToken = prefilledToken
      }
    }
    self.prefilledToken = nil
  }

  @MainActor
  func fetchBuyUrl(
    provider: BraveWallet.OnRampProvider,
    account: BraveWallet.AccountInfo
  ) async -> URL? {
    guard let token = selectedBuyToken else { return nil }

    let symbol: String
    let currencyCode: String
    switch provider {
    case .ramp:
      symbol = token.rampNetworkSymbol
      currencyCode = selectedCurrency.currencyCode
    case .stripe:
      symbol = token.symbol.lowercased()
      currencyCode = selectedCurrency.currencyCode.lowercased()
    default:
      symbol = token.symbol
      currencyCode = selectedCurrency.currencyCode
    }

    var accountAddress = account.address
    if account.coin == .btc,
      let bitcoinAccountInfo =
        await bitcoinWalletService.bitcoinAccountInfo(accountId: account.accountId)
    {
      accountAddress = bitcoinAccountInfo.nextChangeAddress.addressString
    }
    let (urlString, error) = await assetRatioService.buyUrlV1(
      provider: provider,
      chainId: selectedNetwork.chainId,
      address: accountAddress,
      symbol: symbol,
      amount: buyAmount,
      currencyCode: currencyCode
    )

    guard error == nil, let url = URL(string: urlString) else {
      return nil
    }

    return url
  }

  @MainActor
  private func fetchBuyTokens(network: BraveWallet.NetworkInfo) async {
    allTokens = []
    for provider in buyTokens.keys {
      let tokens = await blockchainRegistry.buyTokens(provider: provider, chainId: network.chainId)
      let sortedTokenList = tokens.sorted(by: {
        if $0.isGasToken, !$1.isGasToken {
          return true
        } else if !$0.isGasToken, $1.isGasToken {
          return false
        } else if $0.isBatToken, !$1.isBatToken {
          return true
        } else if !$0.isBatToken, $1.isBatToken {
          return false
        } else {
          return $0.symbol < $1.symbol
        }
      })
      buyTokens[provider] = sortedTokenList
    }

    for provider in orderedSupportedBuyOptions {
      if let tokens = buyTokens[provider] {
        for token in tokens where !allTokens.includes(token) {
          allTokens.append(token)
        }
      }
    }

    if selectedBuyToken == nil || selectedBuyToken?.chainId != network.chainId {
      selectedBuyToken = allTokens.first
    }
  }

  @MainActor
  func updateInfo() async {
    orderedSupportedBuyOptions = BraveWallet.OnRampProvider.allSupportedOnRampProviders

    guard let selectedAccount = await keyringService.allAccounts().selectedAccount else {
      assertionFailure("selectedAccount should never be nil.")
      return
    }
    selectedNetwork = await rpcService.network(coin: selectedAccount.coin, origin: nil)
    await validatePrefilledToken(on: selectedNetwork)  // selectedNetwork may change
    await fetchBuyTokens(network: selectedNetwork)

    // check if current selected network supports buy
    if WalletConstants.supportedTestNetworkChainIds.contains(selectedNetwork.chainId) {
      isSelectedNetworkSupported = false
    } else {
      isSelectedNetworkSupported = allTokens.contains(where: { token in
        return token.chainId.caseInsensitiveCompare(selectedNetwork.chainId) == .orderedSame
      })
    }

    // fetch all available currencies for on ramp providers
    supportedCurrencies = await blockchainRegistry.onRampCurrencies()
    if let usdCurrency = supportedCurrencies.first(where: {
      $0.currencyCode.caseInsensitiveCompare(CurrencyCode.usd.code) == .orderedSame
    }) {
      selectedCurrency = usdCurrency
    } else if let firstCurrency = supportedCurrencies.first {
      selectedCurrency = firstCurrency
    }
  }
}

extension BraveWallet.BlockchainToken {
  fileprivate var isGasToken: Bool {
    guard let gasTokensByChain = BuyTokenStore.gasTokens[chainId] else { return false }
    return gasTokensByChain.contains { $0.caseInsensitiveCompare(symbol) == .orderedSame }
  }

  fileprivate var isBatToken: Bool {
    // BAT/wormhole BAT/Avalanche C-Chain BAT
    return symbol.caseInsensitiveCompare("bat") == .orderedSame
      || symbol.caseInsensitiveCompare("wbat") == .orderedSame
      || symbol.caseInsensitiveCompare("bat.e") == .orderedSame
  }

  // a special symbol to fetch correct ramp.network buy url
  fileprivate var rampNetworkSymbol: String {
    if symbol.caseInsensitiveCompare("bat") == .orderedSame
      && chainId.caseInsensitiveCompare(BraveWallet.MainnetChainId) == .orderedSame
    {
      // BAT is the only token on Ethereum Mainnet with a prefix on Ramp.Network
      return "ETH_BAT"
    } else if chainId.caseInsensitiveCompare(BraveWallet.AvalancheMainnetChainId) == .orderedSame
      && contractAddress.isEmpty
    {
      // AVAX native token has no prefix
      return symbol
    } else {
      let rampNetworkPrefix: String
      switch chainId.lowercased() {
      case BraveWallet.MainnetChainId.lowercased(),
        BraveWallet.CeloMainnetChainId.lowercased():
        rampNetworkPrefix = ""
      case BraveWallet.AvalancheMainnetChainId.lowercased():
        rampNetworkPrefix = "AVAXC"
      case BraveWallet.BnbSmartChainMainnetChainId.lowercased():
        rampNetworkPrefix = "BSC"
      case BraveWallet.PolygonMainnetChainId.lowercased():
        rampNetworkPrefix = "MATIC"
      case BraveWallet.SolanaMainnet.lowercased():
        rampNetworkPrefix = "SOLANA"
      case BraveWallet.OptimismMainnetChainId.lowercased():
        rampNetworkPrefix = "OPTIMISM"
      case BraveWallet.FilecoinMainnet.lowercased():
        rampNetworkPrefix = "FILECOIN"
      default:
        rampNetworkPrefix = ""
      }

      return rampNetworkPrefix.isEmpty ? symbol : "\(rampNetworkPrefix)_\(symbol.uppercased())"
    }
  }
}
