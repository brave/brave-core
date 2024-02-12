// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import SwiftUI
import Combine
import Data
import Preferences

public enum AssetGroupType: Equatable, Identifiable {
  case none
  case network(BraveWallet.NetworkInfo)
  case account(BraveWallet.AccountInfo)
  
  public var id: String {
    switch self {
    case .none: return "Group.none"
    case let .network(network):
      return "Group.network.\(network.id)"
    case let .account(account):
      return "Group.account.\(account.id)"
    }
  }
}

/// A protocol for both fungible and non-fungible asset group's view model
protocol WalletAssetGroupViewModel {
  associatedtype ViewModel
  var groupType: AssetGroupType { get }
  var assets: [ViewModel] { get }
}

extension WalletAssetGroupViewModel {
  var title: String {
    switch groupType {
    case .none:
      return ""
    case let .network(network):
      return network.chainName
    case let .account(account):
      return account.name
    }
  }
  
  var description: String? {
    switch groupType {
    case .none, .network:
      return nil
    case let .account(account):
      return account.address.truncatedAddress
    }
  }
}

public struct AssetGroupViewModel: WalletAssetGroupViewModel, Identifiable, Equatable {
  typealias ViewModel = AssetViewModel
  
  public var groupType: AssetGroupType
  public var assets: [AssetViewModel]
  public var id: String { "\(groupType.id) \(title)" }
  
  var totalFiatValue: Double {
    assets.reduce(0) { partialResult, asset in
      let balance: Double
      switch groupType {
      case let .account(account):
        balance = asset.balanceForAccounts[account.address] ?? 0
      case .none, .network:
        balance = asset.totalBalance
      }
      let assetValue = (Double(asset.price) ?? 0) * balance
      return partialResult + assetValue
    }
  }
}

public struct AssetViewModel: Identifiable, Equatable {
  let groupType: AssetGroupType
  let token: BraveWallet.BlockchainToken
  let network: BraveWallet.NetworkInfo
  let price: String
  let history: [BraveWallet.AssetTimePrice]
  /// Balance for each account for this asset. The key is the account address.
  let balanceForAccounts: [String: Double]
  /// The total balance for all accounts for this asset.
  var totalBalance: Double {
    balanceForAccounts.values.reduce(0, +)
  }

  public var id: String {
    "\(groupType.id)\(token.id)\(network.chainId)"
  }
  
  /// The quantity / balance to display for this asset within it's `AssetGroupType`.
  var quantity: String {
    let balance: Double
    switch groupType {
    case let .account(account):
      balance = balanceForAccounts[account.address] ?? 0
    case .none, .network:
      balance = totalBalance
    }
    return String(format: "%.04f", balance)
  }
  
  /// The formatted fiat amount to display for this asset within it's `AssetGroupType`.
  func fiatAmount(currencyFormatter: NumberFormatter) -> String {
    let balance: Double
    switch groupType {
    case let .account(account):
      balance = balanceForAccounts[account.address] ?? 0
    case .none, .network:
      balance = totalBalance
    }
    return currencyFormatter.string(from: NSNumber(value: (Double(price) ?? 0) * balance)) ?? ""
  }
  
  /// Sort by the fiat/value of the asset (price x balance), otherwise by balance when price is unavailable.
  static func sorted(by sortOrder: SortOrder = .valueDesc, lhs: AssetViewModel, rhs: AssetViewModel) -> Bool {
    switch sortOrder {
    case .valueAsc, .valueDesc:
      if let lhsPrice = Double(lhs.price),
         let rhsPrice = Double(rhs.price) {
        let lhsValue = (lhsPrice * lhs.totalBalance)
        let rhsValue = (rhsPrice * rhs.totalBalance)
        if lhsValue == rhsValue, lhsValue <= 0 {
          return emptyBalanceSort(lhs: lhs, rhs: rhs)
        }
        if sortOrder == .valueAsc {
          return lhsValue < rhsValue
        }
        return lhsValue > rhsValue
      } else if let lhsPrice = Double(lhs.price), (lhsPrice * lhs.totalBalance) > 0 {
        // lhs has a non-zero value
        return true
      } else if let rhsPrice = Double(rhs.price), (rhsPrice * rhs.totalBalance) > 0 {
        // rhs has a non-zero value
        return false
      }
      if lhs.totalBalance == rhs.totalBalance, lhs.totalBalance <= 0 {
        return emptyBalanceSort(lhs: lhs, rhs: rhs)
      }
      // price unavailable, sort by balance
      if sortOrder == .valueAsc {
        return lhs.totalBalance < rhs.totalBalance
      }
      return lhs.totalBalance > rhs.totalBalance
    case .alphaAsc:
      return lhs.token.name.localizedStandardCompare(rhs.token.name) == .orderedAscending
    case .alphaDesc:
      return lhs.token.name.localizedStandardCompare(rhs.token.name) == .orderedDescending
    }
  }
  
  /// Sorts primary networks to be first (Solana Mainnet first primary network), then sorts native assets to be first, then sorts alphabetically.
  /// Used when two tokens have the same balance or fiat value (typically 0 / $0).
  private static func emptyBalanceSort(lhs: AssetViewModel, rhs: AssetViewModel) -> Bool {
    // sort primary networks to be first
    let isLHSPrimaryNetwork = WalletConstants.primaryNetworkChainIds.contains(lhs.network.chainId)
    let isRHSPrimaryNetwork = WalletConstants.primaryNetworkChainIds.contains(rhs.network.chainId)
    if isLHSPrimaryNetwork && !isRHSPrimaryNetwork {
      return true
    } else if !isLHSPrimaryNetwork && isRHSPrimaryNetwork {
      return false
    } else if isLHSPrimaryNetwork, isRHSPrimaryNetwork,
              lhs.network.chainId != rhs.network.chainId,
              lhs.network.chainId == BraveWallet.SolanaMainnet {
      // Solana Mainnet to be first primary network
      return true
    } else if isLHSPrimaryNetwork, isRHSPrimaryNetwork,
              lhs.network.chainId != rhs.network.chainId,
              rhs.network.chainId == BraveWallet.SolanaMainnet {
      // Solana Mainnet to be first primary network
      return false
    }
    // sort native tokens to be first
    let isLHSNativeToken = lhs.network.isNativeAsset(lhs.token)
    let isRHSNativeToken = rhs.network.isNativeAsset(rhs.token)
    if isLHSNativeToken && !isRHSNativeToken {
      return true
    } else if !isLHSNativeToken && isRHSNativeToken {
      return false
    }
    // sort by name
    return lhs.token.name.localizedStandardCompare(rhs.token.name) == .orderedAscending
  }
}

struct BalanceTimePrice: DataPoint, Equatable {
  var date: Date
  var price: Double
  var formattedPrice: String

  var value: CGFloat {
    price
  }
}

struct BalanceDifference: Equatable {
  let priceDifference: String
  let percentageChange: String
  let isBalanceUp: Bool
}

/// A store containing data around the users assets
public class PortfolioStore: ObservableObject, WalletObserverStore {
  /// The dollar amount of your portfolio
  @Published private(set) var balance: String = "$0.00"
  /// Balance difference used to display difference between first point on the graph and current balance.
  @Published private(set) var balanceDifference: BalanceDifference?
  /// The users visible fungible token groups.
  @Published private(set) var assetGroups: [AssetGroupViewModel] = []
  /// The timeframe of the portfolio
  @Published var timeframe: BraveWallet.AssetPriceTimeframe = .oneDay {
    didSet {
      if timeframe != oldValue {
        update()
      }
    }
  }
  /// A set of balances of your portfolio's visible assets based on `timeframe`
  @Published private(set) var historicalBalances: [BalanceTimePrice] = []
  /// Whether or not balances are still currently loading
  @Published private(set) var isLoadingBalances: Bool = false
  /// The current default base currency code
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard oldValue != currencyCode else { return }
      update()
    }
  }
  
  @Published private(set) var isLoadingDiscoverAssets: Bool = false
  
  var isShowingAssetsLoadingState: Bool {
    if filters.isHidingSmallBalances || filters.groupBy != .none {
      // 1. If we are hiding small balances, assets will be hidden
      // unless they previously had fetched balance.
      // 2. If assets are grouped, groups with 0 total balance are hidden.
      // Show loading state when fetching balances for first time.
      return isLoadingBalances && tokenBalancesCache
        .filter { $0.value.values.reduce(0, +) > 0 }
        .isEmpty
    }
    // if we are not hiding small balances then assets are displayed
    // with empty balance, show them while balance is fetched
    return false
  }
  
  var isShowingAssetsEmptyState: Bool {
    guard !isShowingAssetsLoadingState else { return false }
    if filters.groupBy == .none, let noneGroup = assetGroups.first {
      return noneGroup.assets.isEmpty
    }
    return assetGroups.isEmpty
  }
  
  /// All User Accounts
  var allAccounts: [BraveWallet.AccountInfo] = []
  /// All available networks
  var allNetworks: [BraveWallet.NetworkInfo] = []
  var filters: Filters {
    let nonSelectedAccountAddresses = Preferences.Wallet.nonSelectedAccountsFilter.value
    let nonSelectedNetworkChainIds = Preferences.Wallet.nonSelectedNetworksFilter.value
    return Filters(
      accounts: allAccounts.map { account in
          .init(
            isSelected: !nonSelectedAccountAddresses.contains(where: { $0 == account.address }),
            model: account
          )
      },
      networks: allNetworks.map { network in
          .init(
            isSelected: !nonSelectedNetworkChainIds.contains(where: { $0 == network.chainId }),
            model: network
          )
      }
    )
  }
  /// Flag indicating when we are saving filters. Since we are observing multiple `Preference.Option`s,
  /// we should avoid calling `update()` in `preferencesDidChange()` unless another view changed.
  private var isSavingFilters: Bool = false

  public private(set) lazy var userAssetsStore: UserAssetsStore = .init(
    blockchainRegistry: self.blockchainRegistry,
    rpcService: self.rpcService,
    keyringService: self.keyringService,
    assetRatioService: self.assetRatioService,
    walletService: self.walletService,
    ipfsApi: self.ipfsApi,
    userAssetManager: self.assetManager
  )
  
  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
  
  /// Cancellable for the last running `update()` Task.
  private var updateTask: Task<(), Never>?
  /// Cache of token balances for each account. [token.id: [account.address: balance]]
  private var tokenBalancesCache: [String: [String: Double]] = [:]
  /// Cache of prices for each token. The key is the token's `assetRatioId`.
  private var pricesCache: [String: String] = [:]
  /// Cache of priceHistories. The key is the token's `assetRatioId`.
  private var priceHistoriesCache: [String: [BraveWallet.AssetTimePrice]] = [:]

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let ipfsApi: IpfsAPI
  private let assetManager: WalletUserAssetManagerType
  private var rpcServiceObserver: JsonRpcServiceObserver?
  private var keyringServiceObserver: KeyringServiceObserver?
  private var walletServiceObserver: WalletServiceObserver?
  
  var isObserving: Bool {
    rpcServiceObserver != nil && keyringServiceObserver != nil && walletServiceObserver != nil
  }

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry
    self.ipfsApi = ipfsApi
    self.assetManager = userAssetManager

    self.setupObservers()

    keyringService.isLocked { [self] isLocked in
      if !isLocked {
        update()
      }
    }
    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
    Preferences.Wallet.showTestNetworks.observe(from: self)
    Preferences.Wallet.sortOrderFilter.observe(from: self)
    Preferences.Wallet.isHidingSmallBalancesFilter.observe(from: self)
    Preferences.Wallet.nonSelectedAccountsFilter.observe(from: self)
    Preferences.Wallet.nonSelectedNetworksFilter.observe(from: self)
    Preferences.Wallet.groupByFilter.observe(from: self)
  }
  
  func tearDown() {
    rpcServiceObserver = nil
    keyringServiceObserver = nil
    walletServiceObserver = nil
    
    userAssetsStore.tearDown()
  }
  
  func setupObservers() {
    guard !isObserving else { return }
    self.rpcServiceObserver = JsonRpcServiceObserver(
      rpcService: rpcService,
      _chainChangedEvent: { [weak self] _, _, _ in
        self?.update()
      }
    )
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _unlocked: { [weak self] in
        DispatchQueue.main.async {
          self?.update()
        }
      },
      _accountsChanged: { [weak self] in
        Task { @MainActor [self] in
          // An account was added or removed, `update()` will update `allAccounts`.
          self?.update()
        }
      }
    )
    self.walletServiceObserver = WalletServiceObserver(
      walletService: walletService,
      _onDefaultBaseCurrencyChanged: { [weak self] currency in
        self?.currencyCode = currency
      },
      _onNetworkListChanged: { [weak self] in
        Task { @MainActor [self] in
          // A network was added or removed, `update()` will update `allNetworks`.
          self?.update()
        }
      },
      _onDiscoverAssetsStarted: { [weak self] in
        self?.isLoadingDiscoverAssets = true
      },
      _onDiscoverAssetsCompleted: { [weak self] discoveredAssets in
        self?.isLoadingDiscoverAssets = false
        // assets update will be called via `CryptoStore`
      }
    )
    
    self.userAssetsStore.setupObservers()
  }
  
  func update() {
    self.updateTask?.cancel()
    self.updateTask = Task { @MainActor in
      self.isLoadingBalances = true
      self.allAccounts = await keyringService.allAccounts().accounts
        .filter { account in
          WalletConstants.supportedCoinTypes().contains(account.coin)
        }
      self.allNetworks = await rpcService.allNetworksForSupportedCoins()
      let filters = self.filters
      let selectedAccounts = filters.accounts.filter(\.isSelected).map(\.model)
      let selectedNetworks = filters.networks.filter(\.isSelected).map(\.model)
      let allVisibleUserAssets: [NetworkAssets] = assetManager.getAllUserAssetsInNetworkAssetsByVisibility(
        networks: selectedNetworks,
        visible: true
      ).map { networkAssets in // filter out NFTs from Portfolio
        NetworkAssets(
          network: networkAssets.network,
          tokens: networkAssets.tokens.filter { !($0.isNft || $0.isErc721) },
          sortOrder: networkAssets.sortOrder
        )
      }
      // update assets on display immediately with empty values. Issue #5567
      self.assetGroups = buildAssetGroupViewModels(
        groupBy: filters.groupBy,
        selectedAccounts: selectedAccounts,
        selectedNetworks: selectedNetworks,
        allVisibleUserAssets: allVisibleUserAssets
      )
      guard !Task.isCancelled else { return }
      typealias TokenNetworkAccounts = (token: BraveWallet.BlockchainToken, network: BraveWallet.NetworkInfo, accounts: [BraveWallet.AccountInfo])
      let allTokenNetworkAccounts = allVisibleUserAssets.flatMap { networkAssets in
        networkAssets.tokens.map { token in
          TokenNetworkAccounts(
            token: token,
            network: networkAssets.network,
            accounts: selectedAccounts.filter {
              if token.coin == .fil {
                return $0.keyringId == BraveWallet.KeyringId.keyringId(for: token.coin, on: token.chainId)
              } else {
                return $0.coin == token.coin
              }
            }
          )
        }
      }
      
      /// Fetch balance for each token, for all accounts applicable to that token
      tokenBalancesCache = await withTaskGroup(
        of: [String: [String: Double]].self,
        body: { @MainActor [tokenBalancesCache, rpcService] group in
          for tokenNetworkAccounts in allTokenNetworkAccounts { // for each token
            group.addTask { @MainActor in
              let token = tokenNetworkAccounts.token
              var tokenBalances = tokenBalancesCache[token.id] ?? [:]
              for account in tokenNetworkAccounts.accounts { // fetch balance for this token for each account
                let balanceForToken = await rpcService.balance(
                  for: token,
                  in: account,
                  network: tokenNetworkAccounts.network
                )
                tokenBalances.merge(with: [account.address: balanceForToken ?? 0])
              }
              return [token.id: tokenBalances]
            }
          }
          return await group.reduce(into: [String: [String: Double]](), { partialResult, new in
            partialResult.merge(with: new)
          })
        })

      // fetch price for every token
      let allTokens = allVisibleUserAssets.flatMap(\.tokens)
      let allAssetRatioIds = allTokens.map(\.assetRatioId)
      let prices: [String: String] = await assetRatioService.fetchPrices(
        for: allAssetRatioIds,
        toAssets: [currencyFormatter.currencyCode],
        timeframe: timeframe
      )
      for (key, value) in prices { // update cached values
        self.pricesCache[key] = value
      }

      // fetch price history for every non-zero balance token
      let nonZeroBalanceAssetRatioIds: [String] = allTokens
        .filter { (tokenBalancesCache[$0.id] ?? [:]).values.reduce(0, +) > 0 }
        .map { $0.assetRatioId }
      let priceHistories: [String: [BraveWallet.AssetTimePrice]] = await fetchPriceHistory(for: nonZeroBalanceAssetRatioIds)
      for (key, value) in priceHistories { // update cached values
        self.priceHistoriesCache[key] = value
      }
      
      guard !Task.isCancelled else { return }
      self.assetGroups = buildAssetGroupViewModels(
        groupBy: filters.groupBy,
        selectedAccounts: selectedAccounts,
        selectedNetworks: selectedNetworks,
        allVisibleUserAssets: allVisibleUserAssets
      )
      
      let allAssets = assetGroups.flatMap(\.assets)
      // Compute balance based on current prices
      let currentBalance = allAssets
        .compactMap {
          if let price = Double($0.price) {
            return $0.totalBalance * price
          }
          return nil
        }
        .reduce(0.0, +)
      balance = currencyFormatter.string(from: NSNumber(value: currentBalance)) ?? "–"
      // Compute historical balances based on historical prices and current balances
      let assetsWithHistory = allAssets.filter { !$0.history.isEmpty }  // [[AssetTimePrice]]
      let minCount = assetsWithHistory.map(\.history.count).min() ?? 0  // Shortest array count
      historicalBalances = (0..<minCount).map { index in
        let value = assetsWithHistory.reduce(0.0, {
          $0 + ((Double($1.history[index].price) ?? 0.0) * $1.totalBalance)
        })
        return .init(
          date: assetsWithHistory.map { $0.history[index].date }.max() ?? .init(),
          price: value,
          formattedPrice: currencyFormatter.string(from: NSNumber(value: value)) ?? "0.00"
        )
      }
      
      if let oldestHistoricalValue = historicalBalances.first {
        let priceDifference = currentBalance - oldestHistoricalValue.price
        let percentageChange = priceDifference / oldestHistoricalValue.price * 100
        let isBalanceUp = priceDifference > 0
        balanceDifference = .init(
          priceDifference: String(
            format: "%@%@",
            isBalanceUp ? "+" : "", // include plus if balance increased
            currencyFormatter.string(from: NSNumber(value: priceDifference)) ?? "\(priceDifference)"),
          percentageChange: String(
            format: "%@%.2f%%",
            isBalanceUp ? "+" : "", // include plus if balance increased
            percentageChange),
          isBalanceUp: isBalanceUp
        )
      } else { // don't display difference / percentage change
        balanceDifference = nil
      }
      
      isLoadingBalances = false
    }
  }
  
  private func buildAssetGroupViewModels(
    groupBy: GroupBy,
    selectedAccounts: [BraveWallet.AccountInfo],
    selectedNetworks: [BraveWallet.NetworkInfo],
    allVisibleUserAssets: [NetworkAssets]
  ) -> [AssetGroupViewModel] {
    let groups: [AssetGroupViewModel]
    switch filters.groupBy {
    case .none:
      return [
        .init(
          groupType: .none,
          assets: buildAssetViewModels(
            for: .none,
            allVisibleUserAssets: allVisibleUserAssets
          )
        )
      ]
    case .accounts:
      groups = selectedAccounts.map { account in
        let groupType: AssetGroupType = .account(account)
        let assets = buildAssetViewModels(
          for: .account(account),
          allVisibleUserAssets: allVisibleUserAssets
        )
        return AssetGroupViewModel(
          groupType: groupType,
          assets: assets
        )
      }
    case .networks:
      groups = selectedNetworks.map { network in
        let groupType: AssetGroupType = .network(network)
        let assets = buildAssetViewModels(
          for: groupType,
          allVisibleUserAssets: allVisibleUserAssets
        )
        return AssetGroupViewModel(
          groupType: groupType,
          assets: assets
        )
      }
    }

    return groups
      .sorted(by: { $0.totalFiatValue > $1.totalFiatValue })
      .optionallyFilter( // when grouping assets & hiding small balances
        shouldFilter: filters.groupBy != .none && filters.isHidingSmallBalances,
        isIncluded: { group in
          // hide groups without assets or zero fiat value.
          return (!group.assets.isEmpty && group.totalFiatValue > 0)
        }
      )
  }
  
  private func buildAssetViewModels(
    for groupType: AssetGroupType,
    allVisibleUserAssets: [NetworkAssets]
  ) -> [AssetViewModel] {
    let selectedAccounts = self.filters.accounts.filter(\.isSelected).map(\.model)
    switch groupType {
    case .none:
      return allVisibleUserAssets.flatMap { networkAssets in
        networkAssets.tokens.map { token in
          AssetViewModel(
            groupType: groupType,
            token: token,
            network: networkAssets.network,
            price: pricesCache[token.assetRatioId.lowercased(), default: ""],
            history: priceHistoriesCache[token.assetRatioId.lowercased(), default: []],
            balanceForAccounts: tokenBalancesCache[token.id, default: [:]]
              .filter { key, value in
                // if we previously fetched balance for an account it will remain in cache.
                // filter out to avoid including in total balance
                selectedAccounts.contains(where: { $0.address == key })
              }
          )
        }
      }
      .optionallyFilter(
        shouldFilter: filters.isHidingSmallBalances,
        isIncluded: { asset in
          let value = (Double(asset.price) ?? 0) * asset.totalBalance
          return value >= 0.05
        }
      )
      .sorted(by: { lhs, rhs in
        AssetViewModel.sorted(by: filters.sortOrder, lhs: lhs, rhs: rhs)
      })
    case let .network(network):
      guard let networkAssets = allVisibleUserAssets
        .first(where: { $0.network.chainId == network.chainId && $0.network.coin == network.coin }) else {
        return []
      }
      return networkAssets.tokens
        .map { token in
          AssetViewModel(
            groupType: groupType,
            token: token,
            network: networkAssets.network,
            price: pricesCache[token.assetRatioId.lowercased(), default: ""],
            history: priceHistoriesCache[token.assetRatioId.lowercased(), default: []],
            balanceForAccounts: tokenBalancesCache[token.id, default: [:]]
              .filter { key, value in
                // if we previously fetched balance for an account it will remain in cache.
                // filter out to avoid including in total balance
                selectedAccounts.contains(where: { $0.address == key })
              }
          )
        }
        .optionallyFilter(
          shouldFilter: filters.isHidingSmallBalances,
          isIncluded: { asset in
            let value = (Double(asset.price) ?? 0) * asset.totalBalance
            return value >= 0.05
          }
        )
        .sorted(by: { lhs, rhs in
          AssetViewModel.sorted(by: filters.sortOrder, lhs: lhs, rhs: rhs)
        })
    case let .account(account):
      return allVisibleUserAssets
        .filter { $0.network.coin == account.coin && $0.network.supportedKeyrings.contains(account.accountId.keyringId.rawValue as NSNumber) }
        .flatMap { networkAssets in
          networkAssets.tokens.map { token in
            AssetViewModel(
              groupType: groupType,
              token: token,
              network: networkAssets.network,
              price: pricesCache[token.assetRatioId.lowercased(), default: ""],
              history: priceHistoriesCache[token.assetRatioId.lowercased(), default: []],
              balanceForAccounts: tokenBalancesCache[token.id, default: [:]]
                .filter { key, value in
                  // only provide grouped account balance
                  key == account.address
                }
            )
          }
        }
        .optionallyFilter(
          shouldFilter: filters.isHidingSmallBalances,
          isIncluded: { asset in
            let balanceForAccount = asset.balanceForAccounts[account.address] ?? 0
            let value = (Double(asset.price) ?? 0) * balanceForAccount
            return value >= 0.05
          }
        )
        .sorted(by: { lhs, rhs in
          AssetViewModel.sorted(by: filters.sortOrder, lhs: lhs, rhs: rhs)
        })
    }
  }
  
  /// Fetches the price history for the given `assetRatioId`, giving a dictionary with the price history for each symbol
  @MainActor func fetchPriceHistory(
    for priceIds: [String]
  ) async -> [String: [BraveWallet.AssetTimePrice]] {
    let uniquePriceIds = Set(priceIds)
    let priceHistories = await withTaskGroup(of: [String: [BraveWallet.AssetTimePrice]].self) { @MainActor group -> [String: [BraveWallet.AssetTimePrice]] in
      for priceId in uniquePriceIds {
        let priceId = priceId.lowercased()
        group.addTask { @MainActor in
          let (success, history) = await self.assetRatioService.priceHistory(
            priceId,
            vsAsset: self.currencyFormatter.currencyCode,
            timeframe: self.timeframe
          )
          if success {
            return [priceId: history.sorted(by: { $0.date < $1.date })]
          } else {
            return [:]
          }
        }
      }
      return await group.reduce(into: [String: [BraveWallet.AssetTimePrice]](), { partialResult, new in
        for key in new.keys {
          partialResult[key] = new[key]
        }
      })
    }
    return priceHistories
  }
}

extension PortfolioStore: PreferencesObserver {
  func saveFilters(_ filters: Filters) {
    isSavingFilters = true
    filters.save()
    isSavingFilters = false
    update()
  }
  public func preferencesDidChange(for key: String) {
    guard !isSavingFilters else { return }
    update()
  }
}

extension Array {
  /// `filter` helper that skips iterating through the entire array when not applying any filtering.
  @inlinable public func optionallyFilter(shouldFilter: Bool, isIncluded: (Element) throws -> Bool) rethrows -> [Element] {
    guard shouldFilter else { return self }
    return try filter(isIncluded)
  }
  
  /// `sort` helper that skips iterating through the entire array when not applying any sorting.
  @inlinable public func optionallySort(shouldSort: Bool, by sort: (Element, Element) throws -> Bool) rethrows -> [Element] {
    guard shouldSort else { return self }
    return try sorted(by: sort)
  }
}
