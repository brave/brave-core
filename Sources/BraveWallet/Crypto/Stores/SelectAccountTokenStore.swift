// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore

class SelectAccountTokenStore: ObservableObject, WalletObserverStore {
  
  struct AccountSection: Equatable, Identifiable {
    struct TokenBalance: Equatable, Identifiable {
      var id: String { token.id }
      let token: BraveWallet.BlockchainToken
      let network: BraveWallet.NetworkInfo
      let balance: Double?
      let price: String?
      let nftMetadata: NFTMetadata?
      
      init(
        token: BraveWallet.BlockchainToken,
        network: BraveWallet.NetworkInfo,
        balance: Double? = nil,
        price: String? = nil,
        nftMetadata: NFTMetadata? = nil
      ) {
        self.token = token
        self.network = network
        self.balance = balance
        self.price = price
        self.nftMetadata = nftMetadata
      }
    }
    var id: String { account.id }
    let account: BraveWallet.AccountInfo
    let tokenBalances: [TokenBalance]
  }
  
  @Published var networkFilters: [Selectable<BraveWallet.NetworkInfo>] = []
  private var allNetworks: [BraveWallet.NetworkInfo] = []
  
  @Published var isLoadingBalances = false
  @Published var isLoadingPrices = false
  @Published var isHidingZeroBalances = true
  @Published var accountSections: [AccountSection] = []
  @Published var query: String
  
  var filteredAccountSections: [AccountSection] {
    let networks = networkFilters.filter(\.isSelected).map(\.model)
    var filteredAccountSections: [AccountSection] = []
    for accountSection in accountSections {
      guard networks.contains(where: { $0.coin == accountSection.account.coin }) else {
        // don't show account section(s) for incompatible network filter selection
        continue
      }
      let updatedAccountSection = AccountSection(
        account: accountSection.account,
        tokenBalances: accountSection.tokenBalances
          .filter { tokenBalance in
            guard networks.contains(where: { $0.chainId == tokenBalance.network.chainId }) else {
              return false
            }
            if !query.isEmpty, // only if we have a search query
               case let normalizedQuery = query.lowercased(),
               !(tokenBalance.token.symbol.lowercased().contains(normalizedQuery) || tokenBalance.token.name.lowercased().contains(normalizedQuery)) {
              return false
            }
            if isHidingZeroBalances {
              return (tokenBalance.balance ?? 0) > 0
            }
            return true
          }
      )
      if shouldShowSection(updatedAccountSection) {
        filteredAccountSections.append(updatedAccountSection)
      }
    }
    return filteredAccountSections
  }
  
  private func shouldShowSection(_ accountSection: SelectAccountTokenStore.AccountSection) -> Bool {
    guard accountSection.tokenBalances.isEmpty else {
      return true // non-empty
    }
    // only if loading, or hiding zero balances.
    return isHidingZeroBalances && isLoadingBalances
  }
  
  /// The current default base currency code
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard oldValue != currencyCode else { return }
      Task {
        await update()
      }
    }
  }
  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
  
  /// Cache of balances of each asset for each account. The root key(s) are the account `address`/`id`, and the inner dictionary key(s) are the `BraveWallet.BlockchainToken.assetBalanceId`.
  private var balancesForAccountsCache: [String: [String: Double]] = [:]
  /// Cache of prices of assets. The key(s) are the `BraveWallet.BlockchainToken.assetRatioId` lowercased.
  private var pricesForTokensCache: [String: String] = [:]
  /// Cache of metadata for NFTs. The key(s) is the token's `id`.
  private var metadataCache: [String: NFTMetadata] = [:]
  
  let didSelect: (BraveWallet.AccountInfo, BraveWallet.BlockchainToken) -> Void
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let ipfsApi: IpfsAPI
  private let assetManager: WalletUserAssetManagerType
  private var walletServiceObserver: WalletServiceObserver?
  
  var isObserving: Bool {
    walletServiceObserver != nil
  }
  
  init(
    didSelect: @escaping (BraveWallet.AccountInfo, BraveWallet.BlockchainToken) -> Void,
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType,
    query: String? = nil
  ) {
    self.didSelect = didSelect
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.ipfsApi = ipfsApi
    self.assetManager = userAssetManager
    self.query = query ?? ""
    
    self.setupObservers()
  }
  
  func tearDown() {
    walletServiceObserver = nil
  }
  
  func setupObservers() {
    guard !isObserving else { return }
    self.walletServiceObserver = WalletServiceObserver(
      walletService: walletService,
      _onDefaultBaseCurrencyChanged: { [weak self] currency in
        self?.currencyCode = currency
      },
      _onNetworkListChanged: { [weak self] in
        Task { @MainActor [self] in
          // A network was added or removed, update our network filters for the change.
          guard let rpcService = self?.rpcService else { return }
          self?.networkFilters = await rpcService.allNetworksForSupportedCoins().map { network in
            let existingSelectionValue = self?.networkFilters.first(where: { $0.model.chainId == network.chainId})?.isSelected
            return .init(isSelected: existingSelectionValue ?? true, model: network)
          }
        }
      }
    )
  }
  
  func resetFilters() {
    isHidingZeroBalances = true
    networkFilters = allNetworks.map {
      .init(isSelected: true, model: $0)
    }
    query = ""
  }
  
  @MainActor func update() async {
    let allKeyrings = await keyringService.keyrings(for: WalletConstants.supportedCoinTypes())
    let allNetworks = await rpcService.allNetworksForSupportedCoins()
    self.allNetworks = allNetworks
    // setup network filters if not currently setup
    if self.networkFilters.isEmpty {
      self.networkFilters = allNetworks.map {
        .init(isSelected: true, model: $0)
      }
    }
    let allVisibleUserAssets = assetManager.getAllUserAssetsInNetworkAssetsByVisibility(networks: allNetworks, visible: true).flatMap { $0.tokens }
    guard !Task.isCancelled else { return }
    self.accountSections = allKeyrings.flatMap { keyring in
      let tokensForCoin = allVisibleUserAssets.filter { $0.coin == keyring.coin }
      return keyring.accountInfos.map { account in
        let tokenBalances = tokensForCoin.compactMap { token in
          let tokenNetwork = allNetworks.first(where: { $0.chainId == token.chainId }) ?? .init()
          if tokenNetwork.supportedKeyrings.contains(keyring.id.rawValue as NSNumber) {
            return AccountSection.TokenBalance(
              token: token,
              network: allNetworks.first(where: { $0.chainId == token.chainId }) ?? .init(),
              balance: cachedBalance(for: token, in: account),
              price: cachedPrice(for: token, in: account),
              nftMetadata: cachedMetadata(for: token)
            )
          }
          return nil
        }
        return AccountSection(
          account: account,
          tokenBalances: tokenBalances
        )
      }
    }
    
    updateAccountBalances()
    updateTokenPrices()
    updateNFTMetadata()
  }
  
  func updateTokenPrices() {
    guard !accountSections.isEmpty else { return }
    Task { @MainActor in
      self.isLoadingPrices = true
      defer { self.isLoadingPrices = false }
      let allAssetRatioIds = accountSections
        .flatMap(\.tokenBalances)
        .map(\.token)
        .map(\.assetRatioId)
      let prices: [String: String] = await assetRatioService.fetchPrices(
        for: allAssetRatioIds,
        toAssets: [currencyFormatter.currencyCode],
        timeframe: .oneDay
      )
      self.pricesForTokensCache.merge(with: prices)
      updateModels()
    }
  }
  
  private var updateAccountBalancesTask: Task<Void, Never>?
  func updateAccountBalances() {
    guard !accountSections.isEmpty else { return }
    updateAccountBalancesTask?.cancel()
    updateAccountBalancesTask = Task { @MainActor in
      self.isLoadingBalances = true
      defer { isLoadingBalances = false }
      for accountSection in accountSections {
        let balancesForTokens: [String: Double] = await withTaskGroup(
          of: [String: Double].self,
          body: { @MainActor group in
            for tokenBalance in accountSection.tokenBalances {
              group.addTask { @MainActor in
                let totalBalance = await self.rpcService.balance(
                  for: tokenBalance.token,
                  in: accountSection.account,
                  network: tokenBalance.network
                )
                return [tokenBalance.token.assetBalanceId: totalBalance ?? 0]
              }
            }
            return await group.reduce(into: [String: Double](), { partialResult, new in
              for key in new.keys {
                partialResult[key] = new[key]
              }
            })
          }
        )
        guard !Task.isCancelled else { return }
        var updatedBalancesForTokens = (self.balancesForAccountsCache[accountSection.account.id] ?? [:])
        updatedBalancesForTokens.merge(with: balancesForTokens)
        self.balancesForAccountsCache[accountSection.account.id] = updatedBalancesForTokens
      }
      updateModels()
    }
  }
  
  func updateNFTMetadata() {
    guard !accountSections.isEmpty else { return }
    Task { @MainActor in
      let allNFTs = accountSections.flatMap(\.tokenBalances).map(\.token).filter { $0.isNft || $0.isErc721 }
      guard !allNFTs.isEmpty else { return }
      let allMetadata = await rpcService.fetchNFTMetadata(tokens: allNFTs, ipfsApi: ipfsApi)
      self.metadataCache.merge(with: allMetadata)
      self.updateModels()
    }
  }
  
  /// Updates `accountSections` with the available data from `balancesForAccountsCache` &  `pricesForTokensCache`.
  @MainActor private func updateModels() {
    self.accountSections = accountSections.map { accountSection in
      let tokenBalances: [AccountSection.TokenBalance] = accountSection.tokenBalances
        .map { tokenBalance in
          return AccountSection.TokenBalance(
            token: tokenBalance.token,
            network: tokenBalance.network,
            balance: cachedBalance(for: tokenBalance.token, in: accountSection.account),
            price: cachedPrice(for: tokenBalance.token, in: accountSection.account),
            nftMetadata: cachedMetadata(for: tokenBalance.token)
          )
        }
        .sorted { lhs, rhs in
          if lhs.network.isKnownTestnet && rhs.network.isKnownTestnet {
            return (lhs.balance ?? 0) > (rhs.balance ?? 0)
          } else if lhs.network.isKnownTestnet {
            return false // sort test networks to end of list
          } else if rhs.network.isKnownTestnet {
            return true // sort test networks to end of list
          }
          return (lhs.balance ?? 0) > (rhs.balance ?? 0)
        }
      return AccountSection(
        account: accountSection.account,
        tokenBalances: tokenBalances
      )
    }
  }
  
  /// Helper function to get cached balance for a given account & token.
  private func cachedBalance(
    for token: BraveWallet.BlockchainToken,
    in account: BraveWallet.AccountInfo
  ) -> Double? {
    balancesForAccountsCache[account.id]?[token.assetBalanceId]
  }
  
  /// Helper function to get the formatted cached price for a given account & token.
  private func cachedPrice(
    for token: BraveWallet.BlockchainToken,
    in account: BraveWallet.AccountInfo
  ) -> String? {
    if let tokenPrice = pricesForTokensCache[token.assetRatioId.lowercased()],
       let tokenBalance = cachedBalance(for: token, in: account) {
      return currencyFormatter.string(from: NSNumber(value: (Double(tokenPrice) ?? 0) * tokenBalance))
    }
    return nil
  }
  
  /// Helper function to get cached metadata for a given token.
  private func cachedMetadata(
    for token: BraveWallet.BlockchainToken
  ) -> NFTMetadata? {
    metadataCache[token.id]
  }
}

#if DEBUG
extension SelectAccountTokenStore {
  func setupForTesting() {
    allNetworks = [.mockMainnet, .mockGoerli, .mockSolana, .mockSolanaTestnet, .mockFilecoinMainnet, .mockFilecoinTestnet]
  }
}
#endif

extension Array where Element == SelectAccountTokenStore.AccountSection.TokenBalance {
  func filterNonZeroBalances(shouldFilter: Bool = true) -> Self {
    guard shouldFilter else { return self }
    return filter { ($0.balance ?? 0) > 0 }
  }
}
