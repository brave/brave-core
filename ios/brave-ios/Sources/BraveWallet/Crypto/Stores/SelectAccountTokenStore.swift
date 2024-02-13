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
  
  /// The networks to filter the tokens/accounts by.
  @Published var networkFilters: [Selectable<BraveWallet.NetworkInfo>] = [] {
    didSet {
      guard !oldValue.isEmpty else { return } // ignore initial assignment, all networks selected
      updateAccountSections()
    }
  }
  /// Indicates accounts, networks and assets are fetched
  @Published var isSetup = false
  @Published var isLoadingBalances = false
  @Published var isLoadingPrices = false
  /// Indicates tokens without a balance are hidden
  @Published var isHidingZeroBalances = true {
    didSet {
      guard oldValue != isHidingZeroBalances else { return }
      updateAccountSections()
    }
  }
  /// Each account and it's tokens
  @Published var accountSections: [AccountSection] = []
  /// Filter displayed tokens by this query
  @Published var query: String {
    didSet {
      updateAccountSections()
    }
  }
  /// The current default base currency code
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard oldValue != currencyCode else { return }
      updateAccountSections()
    }
  }
  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
  
  private var allNetworks: [BraveWallet.NetworkInfo] = []
  private var balancesFetched: Bool = false
  /// Cache of balances of each asset for each account. [account.address: [token.id: balance]]
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
    updateAccountSections()
  }
  
  // All user accounts.
  private var allAccounts: [BraveWallet.AccountInfo] = []
  // All user visible assets, key is `Identifiable.id` of `BlockchainToken`.
  private var userVisibleAssets: [String: BraveWallet.BlockchainToken] = [:]
  // All user accounts.
  private var userVisibleNetworkAssets: [NetworkAssets] = []
  
  func setup() {
    Task { @MainActor in
      let allAccounts = await keyringService.allAccounts().accounts
      self.allAccounts = allAccounts
      let allNetworks = await rpcService.allNetworksForSupportedCoins()
      self.allNetworks = allNetworks
      self.networkFilters = allNetworks.map {
        .init(isSelected: true, model: $0)
      }
      let allNetworkAssets = assetManager.getAllUserAssetsInNetworkAssetsByVisibility(
        networks: allNetworks,
        visible: true
      )
      let allVisibleUserAssets = allNetworkAssets.flatMap(\.tokens)
      self.userVisibleAssets = allVisibleUserAssets.reduce(
        into: [String: BraveWallet.BlockchainToken](), {
          $0[$1.id] = $1
        })
      // show accounts with all supported tokens until balances are fetched
      self.accountSections = await buildAccountSections(
        selectedNetworks: networkFilters.filter(\.isSelected).map(\.model),
        allAccounts: allAccounts,
        userVisibleAssets: Array(userVisibleAssets.values),
        balancesCache: balancesForAccountsCache,
        balancesFetched: balancesFetched,
        pricesCache: pricesForTokensCache,
        metadataCache: metadataCache,
        hideZeroBalances: isHidingZeroBalances,
        query: query,
        currencyFormatter: currencyFormatter
      )
      self.isSetup = true
      
      // fetch balances for visible assets, fetch prices for tokens with balance
      self.fetchAccountBalances(networkAssets: allNetworkAssets)
      
      // fetch metadata for visible NFT assets (user may select to show 0 balance)
      let allVisibleNFTs = allVisibleUserAssets.filter { $0.isNft || $0.isErc721 }
      self.fetchNFTMetadata(for: allVisibleNFTs)
    }
  }
  
  func updateAccountSections() {
    Task { @MainActor in
      self.accountSections = await buildAccountSections(
        selectedNetworks: networkFilters.filter(\.isSelected).map(\.model),
        allAccounts: allAccounts,
        userVisibleAssets: Array(userVisibleAssets.values),
        balancesCache: balancesForAccountsCache,
        balancesFetched: balancesFetched,
        pricesCache: pricesForTokensCache,
        metadataCache: metadataCache,
        hideZeroBalances: isHidingZeroBalances,
        query: query,
        currencyFormatter: currencyFormatter
      )
    }
  }
  
  /// Fetch the balances for each account for the given `allNetworkAssets`, store in cache and update `accountSections`, and then fetch prices for tokens with non-zero balance.
  func fetchAccountBalances(networkAssets: [NetworkAssets]) {
    guard !self.allAccounts.isEmpty else { return }
    Task { @MainActor in
      self.isLoadingBalances = true
      defer { isLoadingBalances = false }
      let balancesForAccounts = await withTaskGroup(
        of: [String: [String: Double]].self,
        body: { group in
          for account in allAccounts {
            group.addTask { // get balance for all tokens this account supports
              let balancesForTokens: [String: Double] = await self.rpcService.fetchBalancesForTokens(
                account: account, 
                networkAssets: networkAssets
              )
              return [account.address: balancesForTokens]
            }
          }
          return await group.reduce(into: [String: [String: Double]](), { partialResult, new in
            partialResult.merge(with: new)
          })
        }
      )
      for account in allAccounts {
        if let updatedBalancesForAccount = balancesForAccounts[account.address] {
          // if balance fetch failed that we already have cached, don't overwrite existing
          if var existing = self.balancesForAccountsCache[account.address] {
            existing.merge(with: updatedBalancesForAccount)
            self.balancesForAccountsCache[account.address] = existing
          } else {
            self.balancesForAccountsCache[account.address] = updatedBalancesForAccount
          }
        }
      }
      self.balancesFetched = true
      self.updateAccountSections()
      // fetch prices for tokens with balance
      var tokensIdsWithBalance: Set<String> = .init()
      for accountBalance in balancesForAccountsCache.values {
        let tokenIdsWithAccountBalance = accountBalance.filter { $1 > 0 }.map(\.key)
        tokenIdsWithAccountBalance.forEach { tokensIdsWithBalance.insert($0) }
      }
      let assetRatioIdsForTokensWithBalance = tokensIdsWithBalance
        .compactMap { tokenId in
          userVisibleAssets[tokenId]?.assetRatioId
        }
      self.fetchTokenPrices(for: assetRatioIdsForTokensWithBalance)
    }
  }
  
  /// Fetch the prices for the given `assetRatioIds`, store in cache and update `accountSections`.
  func fetchTokenPrices(for assetRatioIds: [String]) {
    guard !assetRatioIds.isEmpty else { return }
    Task { @MainActor in
      self.isLoadingPrices = true
      defer { self.isLoadingPrices = false }
      let prices: [String: String] = await assetRatioService.fetchPrices(
        for: assetRatioIds,
        toAssets: [currencyFormatter.currencyCode],
        timeframe: .oneDay
      )
      self.pricesForTokensCache.merge(with: prices)
      self.updateAccountSections()
    }
  }
  
  func fetchNFTMetadata(for userVisibleNFTs: [BraveWallet.BlockchainToken]) {
    guard !userVisibleNFTs.isEmpty else { return }
    Task { @MainActor in
      let allMetadata = await rpcService.fetchNFTMetadata(tokens: userVisibleNFTs, ipfsApi: ipfsApi)
      self.metadataCache.merge(with: allMetadata)
      self.updateAccountSections()
    }
  }
  
  /// Builds the array of `AccountSection`s for display, taking into account selected networks and filter query.
  private func buildAccountSections(
    selectedNetworks: [BraveWallet.NetworkInfo],
    allAccounts: [BraveWallet.AccountInfo],
    userVisibleAssets: [BraveWallet.BlockchainToken],
    balancesCache: [String: [String: Double]],
    balancesFetched: Bool,
    pricesCache: [String: String],
    metadataCache: [String: NFTMetadata],
    hideZeroBalances: Bool,
    query: String,
    currencyFormatter: NumberFormatter
  ) async -> [AccountSection] {
    let accountSections: [AccountSection] = allAccounts.compactMap { account in
      let tokensForAccountCoin: [BraveWallet.BlockchainToken] = userVisibleAssets
        .filter({ $0.coin == account.coin })
      let accountTokenBalances: [AccountSection.TokenBalance] = tokensForAccountCoin
        .compactMap { token in
          if !query.isEmpty, // only if we have a filter query
             !(token.symbol.localizedCaseInsensitiveContains(query) ||
               token.name.localizedCaseInsensitiveContains(query)) {
            // token does not match query
            return nil
          }
          // network for must be selected
          if let tokenNetwork = selectedNetworks.first(where: { $0.chainId == token.chainId }),
             // network must support account keyring
             tokenNetwork.supportedKeyrings.contains(account.keyringId.rawValue as NSNumber) {
            let balance = balancesCache[account.address]?[token.id] ?? 0
            if hideZeroBalances, balance <= 0 {
              // token has no balance, user is hiding zero balance tokens.
              return nil
            }
            var price: String?
            if let tokenPrice = pricesCache[token.assetRatioId.lowercased()],
               balance > 0 {
              price = currencyFormatter.string(from: NSNumber(value: (Double(tokenPrice) ?? 0) * balance))
            }
            return AccountSection.TokenBalance(
              token: token,
              network: tokenNetwork,
              balance: balance,
              price: price,
              nftMetadata: metadataCache[token.id]
            )
          }
          return nil
        }
      
      if accountTokenBalances.isEmpty && balancesFetched {
        // don't show this account section without token balances
        return nil
      }
      
      return AccountSection(
        account: account,
        tokenBalances: accountTokenBalances
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
      )
    }
    
    return accountSections
  }
}
