// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import OrderedCollections
import Combine

/// A store contains data for buying tokens
public class BuyTokenStore: ObservableObject {
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
  var buyTokens: [BraveWallet.OnRampProvider: [BraveWallet.BlockchainToken]] = [.ramp: [], .sardine: [], .transak: []]
  /// A list of all available tokens for all providers
  var allTokens: [BraveWallet.BlockchainToken] = []

  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private var selectedNetwork: BraveWallet.NetworkInfo = .init()
  private(set) var orderedSupportedBuyOptions: OrderedSet<BraveWallet.OnRampProvider> = []
  private var prefilledToken: BraveWallet.BlockchainToken?
  
  /// A map between chain id and gas token's symbol
  static let gasTokens: [String: [String]] = [
    BraveWallet.MainnetChainId: ["eth"],
    BraveWallet.OptimismMainnetChainId: ["eth"],
    BraveWallet.AuroraMainnetChainId: ["eth"],
    BraveWallet.PolygonMainnetChainId: ["matic"],
    BraveWallet.FantomMainnetChainId: ["ftm"],
    BraveWallet.CeloMainnetChainId: ["celo"],
    BraveWallet.BinanceSmartChainMainnetChainId: ["bnb"],
    BraveWallet.SolanaMainnet: ["sol"],
    BraveWallet.FilecoinMainnet: ["fil"],
    BraveWallet.AvalancheMainnetChainId: ["avax", "avaxc"]
  ]

  public init(
    blockchainRegistry: BraveWalletBlockchainRegistry,
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    prefilledToken: BraveWallet.BlockchainToken?
  ) {
    self.blockchainRegistry = blockchainRegistry
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.prefilledToken = prefilledToken
    
    self.rpcService.add(self)
    
    Task {
      await updateInfo()
    }
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
      let allNetworksForTokenCoin = await rpcService.allNetworks(prefilledToken.coin)
      guard let networkForToken = allNetworksForTokenCoin.first(where: { $0.chainId == prefilledToken.chainId }) else {
        // don't set prefilled token if it belongs to a network we don't know
        return
      }
      let success = await rpcService.setNetwork(networkForToken.chainId, coin: networkForToken.coin, origin: nil)
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
  ) async -> String? {
    guard let token = selectedBuyToken else { return nil }
    
    let symbol: String
    switch provider {
    case .ramp:
      symbol = token.rampNetworkSymbol
    case .sardine:
      symbol = token.symbol
    default:
      symbol = token.symbol
    }
    
    let (url, error) = await assetRatioService.buyUrlV1(
      provider,
      chainId: selectedNetwork.chainId,
      address: account.address,
      symbol: symbol,
      amount: buyAmount,
      currencyCode: selectedCurrency.currencyCode
    )

    guard error == nil else { return nil }
    
    return url
  }

  @MainActor
  private func fetchBuyTokens(network: BraveWallet.NetworkInfo) async {
    allTokens = []
    for provider in buyTokens.keys {
      let tokens = await blockchainRegistry.buyTokens(provider, chainId: network.chainId)
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
    orderedSupportedBuyOptions = [.ramp, .sardine, .transak]
    
    guard let selectedAccount = await keyringService.allAccounts().selectedAccount else {
      assertionFailure("selectedAccount should never be nil.")
      return
    }
    selectedNetwork = await rpcService.network(selectedAccount.coin, origin: nil)
    await validatePrefilledToken(on: selectedNetwork) // selectedNetwork may change
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
    if let firstCurrency = supportedCurrencies.first {
      selectedCurrency = firstCurrency
    }
  }
}

extension BuyTokenStore: BraveWalletJsonRpcServiceObserver {
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType, origin: URLOrigin?) {
    Task {
      await updateInfo()
    }
  }
  
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}

private extension BraveWallet.BlockchainToken {
  var isGasToken: Bool {
    guard let gasTokensByChain = BuyTokenStore.gasTokens[chainId] else { return false }
    return gasTokensByChain.contains { $0.caseInsensitiveCompare(symbol) == .orderedSame }
  }
  
  var isBatToken: Bool {
    // BAT/wormhole BAT/Avalanche C-Chain BAT
    return symbol.caseInsensitiveCompare("bat") == .orderedSame || symbol.caseInsensitiveCompare("wbat") == .orderedSame || symbol.caseInsensitiveCompare("bat.e") == .orderedSame
  }
  
  // a special symbol to fetch correct ramp.network buy url
  var rampNetworkSymbol: String {
    if symbol.caseInsensitiveCompare("bat") == .orderedSame && chainId.caseInsensitiveCompare(BraveWallet.MainnetChainId) == .orderedSame {
      // BAT is the only token on Ethereum Mainnet with a prefix on Ramp.Network
      return "ETH_BAT"
    } else if chainId.caseInsensitiveCompare(BraveWallet.AvalancheMainnetChainId) == .orderedSame && contractAddress.isEmpty {
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
      case BraveWallet.BinanceSmartChainMainnetChainId.lowercased():
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
