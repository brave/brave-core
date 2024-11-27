// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Data
import Foundation
import Preferences
import SwiftUI

public enum AssetGroupType: Equatable, Identifiable {
  case none
  case network(BraveWallet.NetworkInfo)
  case account(BraveWallet.AccountInfo)

  public var id: String {
    switch self {
    case .none: return "Group.none"
    case .network(let network):
      return "Group.network.\(network.id)"
    case .account(let account):
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
    case .network(let network):
      return network.chainName
    case .account(let account):
      return account.name
    }
  }

  var description: String? {
    switch groupType {
    case .none, .network:
      return nil
    case .account(let account):
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
      case .account(let account):
        balance = asset.balanceForAccounts[account.id] ?? 0
      case .none, .network:
        balance = asset.totalBalance
      }
      let assetValue = (Double(asset.price) ?? 0) * balance
      return partialResult + assetValue
    }
  }

  /// Sort by the group's total fiat/value
  static func sorted(
    lhs: AssetGroupViewModel,
    rhs: AssetGroupViewModel
  ) -> Bool {
    if lhs.totalFiatValue == rhs.totalFiatValue {
      return sameBalanceSort(lhs: lhs, rhs: rhs)
    } else {
      return lhs.totalFiatValue > rhs.totalFiatValue
    }
  }

  /// Sorts primary networks to be first (Solana Mainnet first primary network),  then sorts alphabetically.
  /// Used when two tokens have the same balance or fiat value (typically 0 / $0).
  private static func sameBalanceSort(lhs: AssetGroupViewModel, rhs: AssetGroupViewModel) -> Bool {
    if case .account(let lhsAccount) = lhs.groupType, case .account(let rhsAccount) = rhs.groupType
    {
      return lhsAccount.sort(with: rhsAccount, parentOrder: lhs.id < rhs.id)
    }
    if case .network(let lhsNetwork) = lhs.groupType, case .network(let rhsNetwork) = rhs.groupType
    {
      return lhsNetwork.sort(with: rhsNetwork, parentOrder: lhs.id < rhs.id)
    }
    return lhs.id < rhs.id
  }
}

public struct AssetViewModel: Identifiable, Equatable {
  let groupType: AssetGroupType
  let token: BraveWallet.BlockchainToken
  let network: BraveWallet.NetworkInfo
  let price: String
  let history: [BraveWallet.AssetTimePrice]
  /// Balance for each account for this asset. The key is the account.id.
  let balanceForAccounts: [String: Double]
  /// All BTC balance types for each account. Key is `account.id`.
  let btcBalances: [String: [BTCBalanceType: Double]]
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
    case .account(let account):
      balance = balanceForAccounts[account.id] ?? 0
    case .none, .network:
      balance = totalBalance
    }
    return String(format: "%.04f", balance)
  }

  /// The formatted fiat amount to display for this asset within it's `AssetGroupType`.
  func fiatAmount(currencyFormatter: NumberFormatter) -> String {
    let balance: Double
    switch groupType {
    case .account(let account):
      balance = balanceForAccounts[account.id] ?? 0
    case .none, .network:
      balance = totalBalance
    }
    return currencyFormatter.formatAsFiat((Double(price) ?? 0) * balance) ?? ""
  }

  /// Sort by the fiat/value of the asset (price x balance), otherwise by balance when price is unavailable.
  static func sorted(
    by sortOrder: SortOrder = .valueDesc,
    lhs: AssetViewModel,
    rhs: AssetViewModel
  ) -> Bool {
    switch sortOrder {
    case .valueAsc, .valueDesc:
      if let lhsPrice = Double(lhs.price),
        let rhsPrice = Double(rhs.price)
      {
        let lhsValue = (lhsPrice * lhs.totalBalance)
        let rhsValue = (rhsPrice * rhs.totalBalance)
        if lhsValue == rhsValue, lhsValue <= 0 {
          return sameBalanceSort(lhs: lhs, rhs: rhs)
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
        return sameBalanceSort(lhs: lhs, rhs: rhs)
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
  private static func sameBalanceSort(lhs: AssetViewModel, rhs: AssetViewModel) -> Bool {
    var parentOrder: Bool {
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

    return lhs.network.sort(with: rhs.network, parentOrder: parentOrder)
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
  /// The BTC balances for each Bitcoin account.  Key is `account.id`.
  @Published private(set) var accountsBTCBalances: [String: [BTCBalanceType: Double]] = [:]
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
      return isLoadingBalances
        && tokenBalancesCache
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
          isSelected: !nonSelectedAccountAddresses.contains(
            where: { $0 == account.id }
          ),
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

  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter

  /// Cancellable for the last running `update()` Task.
  private var updateTask: Task<(), Never>?
  private typealias TokenBalanceCache = [String: [String: Double]]
  /// Cache of token balances for each account. [token.id: [account.id: balance]]
  private var tokenBalancesCache: TokenBalanceCache = [:]
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
  private let bitcoinWalletService: BraveWalletBitcoinWalletService
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
    bitcoinWalletService: BraveWalletBitcoinWalletService,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry
    self.ipfsApi = ipfsApi
    self.bitcoinWalletService = bitcoinWalletService
    self.assetManager = userAssetManager

    // cache balance update observer
    self.assetManager.addUserAssetDataObserver(self)

    self.setupObservers()

    keyringService.isLocked { [self] isLocked in
      if !isLocked {
        update()
      }
    }
    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
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
  }

  func update() {
    self.updateTask?.cancel()
    self.updateTask = Task { @MainActor in
      let isLocked = await keyringService.isLocked()
      guard !isLocked else { return }  // `update() will be called after unlock`

      self.isLoadingBalances = true
      self.allAccounts = await keyringService.allAccountInfos()
        .filter { account in
          WalletConstants.supportedCoinTypes().contains(account.coin)
        }
      self.allNetworks = await rpcService.allNetworksForSupportedCoins()
      let filters = self.filters
      let selectedAccounts = filters.accounts.filter(\.isSelected).map(\.model)
      let selectedNetworks = filters.networks.filter(\.isSelected).map(\.model)
      let allVisibleUserAssets: [NetworkAssets] =
        await assetManager.getUserAssets(
          networks: selectedNetworks,
          visible: true
        )
        .map { networkAssets in  // filter out NFTs from Portfolio
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
      typealias TokenNetworkAccounts = (
        token: BraveWallet.BlockchainToken, network: BraveWallet.NetworkInfo,
        accounts: [BraveWallet.AccountInfo]
      )
      let allTokenNetworkAccounts = allVisibleUserAssets.flatMap { networkAssets in
        networkAssets.tokens.map { token in
          TokenNetworkAccounts(
            token: token,
            network: networkAssets.network,
            accounts: selectedAccounts.filter {
              switch token.coin {
              case .fil, .btc, .zec:
                return $0.keyringId
                  == BraveWallet.KeyringId.keyringId(
                    for: token.coin,
                    on: token.chainId
                  )
              default:
                return $0.coin == token.coin
              }
            }
          )
        }
      }

      guard !Task.isCancelled else { return }
      if selectedAccounts.contains(where: { $0.coin == .btc }) {
        /// We  need to know if user has pending balance to show/hide banner. Re-fetch on view load.
        self.accountsBTCBalances = await withTaskGroup(of: [String: [BTCBalanceType: Double]].self)
        {
          [bitcoinWalletService] group in
          for account in selectedAccounts where account.coin == .btc {
            group.addTask {
              let btcBalances = await bitcoinWalletService.fetchBTCBalances(
                accountId: account.accountId
              )
              return [account.id: btcBalances]
            }
          }
          var accountsBTCBalances: [String: [BTCBalanceType: Double]] = [:]
          for await accountBTCBalances in group {
            accountsBTCBalances.merge(with: accountBTCBalances)
          }
          return accountsBTCBalances
        }
      }

      // Looping through `allTokenNetworkAccounts` to get token's cached balance
      for tokenNetworkAccounts in allTokenNetworkAccounts {
        if let tokenBalances = assetManager.getAssetBalances(
          for: tokenNetworkAccounts.token,
          account: nil
        ) {
          var result: [String: Double] = [:]
          for balancePerAccount in tokenBalances {
            result.merge(with: [
              balancePerAccount.accountAddress: Double(balancePerAccount.balance) ?? 0
            ])
          }
          tokenBalancesCache.merge(with: [tokenNetworkAccounts.token.id: result])
        } else {
          // 1. We have a user asset from CD but wallet has never
          // fetched it's balance. Should never happen. But we will fetch its
          // balance and cache it in CD.
          // 2. Test Cases will come here, we will fetch balance using
          // a mock `rpcService` and `bitcoinWalletService`
          let fetchedTokenBalances = await withTaskGroup(
            of: TokenBalanceCache.self,
            body: {
              @MainActor
              [tokenBalancesCache, rpcService, bitcoinWalletService, assetManager]
              group in
              group.addTask { @MainActor in
                let token = tokenNetworkAccounts.token
                var tokenBalances = tokenBalancesCache[token.id] ?? [:]
                // fetch balance for this token for each account
                for account in tokenNetworkAccounts.accounts {
                  var balanceForToken: Double?
                  let tokenNetwork = tokenNetworkAccounts.network
                  if account.coin == .btc,
                    tokenNetwork.supportedKeyrings.contains(
                      account.keyringId.rawValue as NSNumber
                    )
                  {
                    balanceForToken = await bitcoinWalletService.fetchBTCBalance(
                      accountId: account.accountId,
                      type: .total
                    )
                  } else {
                    balanceForToken = await rpcService.balance(
                      for: token,
                      in: account,
                      network: tokenNetworkAccounts.network
                    )
                  }
                  tokenBalances.merge(with: [account.id: balanceForToken ?? 0])
                  await assetManager.updateAssetBalance(
                    for: token,
                    account: account.id,
                    balance: "\(balanceForToken ?? 0)"
                  )
                }
                return [token.id: tokenBalances]
              }
              return await group.reduce(
                into: TokenBalanceCache(),
                { partialResult, new in
                  partialResult.merge(with: new)
                }
              )
            }
          )
          tokenBalancesCache.merge(with: fetchedTokenBalances)
        }
      }

      guard !Task.isCancelled else { return }
      // fetch price for every token
      let allTokens = allVisibleUserAssets.flatMap(\.tokens)
      let allAssetRatioIds = allTokens.map(\.assetRatioId)
      let prices: [String: String] = await assetRatioService.fetchPrices(
        for: allAssetRatioIds,
        toAssets: [currencyFormatter.currencyCode],
        timeframe: timeframe
      )
      for (key, value) in prices {  // update cached values
        self.pricesCache[key] = value
      }

      // fetch price history for every non-zero balance token
      let nonZeroBalanceAssetRatioIds: [String] =
        allTokens
        .filter { (tokenBalancesCache[$0.id] ?? [:]).values.reduce(0, +) > 0 }
        .map { $0.assetRatioId }
      let priceHistories: [String: [BraveWallet.AssetTimePrice]] = await fetchPriceHistory(
        for: nonZeroBalanceAssetRatioIds
      )
      for (key, value) in priceHistories {  // update cached values
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
      let currentBalance =
        allAssets
        .compactMap {
          if let price = Double($0.price) {
            return $0.totalBalance * price
          }
          return nil
        }
        .reduce(0.0, +)
      balance = currencyFormatter.formatAsFiat(currentBalance) ?? "–"
      // Compute historical balances based on historical prices and current balances
      let assetsWithHistory = allAssets.filter { $0.history.count > 1 }  // [[AssetTimePrice]]
      let minCount = assetsWithHistory.map(\.history.count).min() ?? 0  // Shortest array count
      historicalBalances = (0..<minCount).map { index in
        let value = assetsWithHistory.reduce(
          0.0,
          {
            $0 + ((Double($1.history[index].price) ?? 0.0) * $1.totalBalance)
          }
        )
        return .init(
          date: assetsWithHistory.map { $0.history[index].date }.max() ?? .init(),
          price: value,
          formattedPrice: currencyFormatter.formatAsFiat(value) ?? "0.00"
        )
      }

      if let oldestHistoricalValue = historicalBalances.first {
        let priceDifference = currentBalance - oldestHistoricalValue.price
        var percentageChange: Double = 0
        if oldestHistoricalValue.price > 0 {
          percentageChange = priceDifference / oldestHistoricalValue.price * 100
        }
        let isBalanceUp = priceDifference > 0
        balanceDifference = .init(
          priceDifference: String(
            format: "%@%@",
            isBalanceUp ? "+" : "",  // include plus if balance increased
            currencyFormatter.formatAsFiat(priceDifference) ?? "\(priceDifference)"
          ),
          percentageChange: String(
            format: "%@%.2f%%",
            isBalanceUp ? "+" : "",  // include plus if balance increased
            percentageChange.isNaN ? 0 : percentageChange
          ),
          isBalanceUp: isBalanceUp
        )
      } else {  // don't display difference / percentage change
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
            accounts: selectedAccounts,
            allVisibleUserAssets: allVisibleUserAssets
          )
        )
      ]
    case .accounts:
      groups = selectedAccounts.map { account in
        let groupType: AssetGroupType = .account(account)
        let assets = buildAssetViewModels(
          for: .account(account),
          accounts: selectedAccounts,
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
          accounts: selectedAccounts,
          allVisibleUserAssets: allVisibleUserAssets
        )
        return AssetGroupViewModel(
          groupType: groupType,
          assets: assets
        )
      }
    }

    return
      groups
      .sorted(by: {
        AssetGroupViewModel.sorted(lhs: $0, rhs: $1)
      })
      .optionallyFilter(  // when grouping assets & hiding small balances
        shouldFilter: filters.groupBy != .none || filters.isHidingSmallBalances,
        isIncluded: { group in
          if filters.isHidingSmallBalances {
            // hide groups without assets or zero fiat value.
            return (!group.assets.isEmpty && group.totalFiatValue > 0)
          }
          // assets are grouped, hide groups without assets
          return !group.assets.isEmpty
        }
      )
  }

  private func buildAssetViewModels(
    for groupType: AssetGroupType,
    accounts: [BraveWallet.AccountInfo],
    allVisibleUserAssets: [NetworkAssets]
  ) -> [AssetViewModel] {
    let selectedAccounts = self.filters.accounts.filter(\.isSelected).map(\.model)
    switch groupType {
    case .none:
      return allVisibleUserAssets.flatMap { networkAssets in
        networkAssets.tokens.map { token in
          var btcBalancesForToken: [String: [BTCBalanceType: Double]] = [:]
          if token.coin == .btc {
            let keyringIdForToken = BraveWallet.KeyringId.keyringId(for: .btc, on: token.chainId)
            let accountIdsForToken =
              accounts
              .filter({ $0.keyringId == keyringIdForToken })
              .map(\.accountId)
            btcBalancesForToken = accountsBTCBalances.filter({ item in
              accountIdsForToken.contains(where: { accountId in
                item.key == accountId.uniqueKey
              })
            })
          }
          return AssetViewModel(
            groupType: groupType,
            token: token,
            network: networkAssets.network,
            price: pricesCache[token.assetRatioId.lowercased(), default: ""],
            history: priceHistoriesCache[token.assetRatioId.lowercased(), default: []],
            balanceForAccounts: tokenBalancesCache[token.id, default: [:]]
              .filter { key, value in
                // if we previously fetched balance for an account it will remain in cache.
                // filter out to avoid including in total balance
                selectedAccounts.contains(where: { $0.id == key })
              },
            btcBalances: btcBalancesForToken
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
    case .network(let network):
      guard
        let networkAssets =
          allVisibleUserAssets
          .first(where: { $0.network.chainId == network.chainId && $0.network.coin == network.coin }
          )
      else {
        return []
      }
      var btcBalancesForNetwork: [String: [BTCBalanceType: Double]] = [:]
      if network.coin == .btc {
        let keyringIdForNetwork = BraveWallet.KeyringId.keyringId(for: .btc, on: network.chainId)
        let accountIdsForToken =
          accounts
          .filter({ $0.keyringId == keyringIdForNetwork })
          .map(\.accountId)
        btcBalancesForNetwork = accountsBTCBalances.filter({ item in
          accountIdsForToken.contains(where: { accountId in
            item.key == accountId.uniqueKey
          })
        })
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
                selectedAccounts.contains(where: { $0.id == key })
              },
            btcBalances: btcBalancesForNetwork
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
    case .account(let account):
      return
        allVisibleUserAssets
        .filter {
          $0.network.coin == account.coin
            && $0.network.supportedKeyrings.contains(
              account.accountId.keyringId.rawValue as NSNumber
            )
        }
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
                  key == account.id
                },
              btcBalances: accountsBTCBalances.filter({ $0.key == account.id })
            )
          }
        }
        .optionallyFilter(
          shouldFilter: filters.isHidingSmallBalances,
          isIncluded: { asset in
            let balanceForAccount = asset.balanceForAccounts[account.id] ?? 0
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
    let priceHistories = await withTaskGroup(of: [String: [BraveWallet.AssetTimePrice]].self) {
      @MainActor group -> [String: [BraveWallet.AssetTimePrice]] in
      for priceId in uniquePriceIds {
        let priceId = priceId.lowercased()
        group.addTask { @MainActor in
          let (success, history) = await self.assetRatioService.priceHistory(
            asset: priceId,
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
      return await group.reduce(
        into: [String: [BraveWallet.AssetTimePrice]](),
        { partialResult, new in
          for key in new.keys {
            partialResult[key] = new[key]
          }
        }
      )
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

extension PortfolioStore: WalletUserAssetDataObserver {
  public func cachedBalanceRefreshed() {
    update()
  }

  public func userAssetUpdated() {
    update()
  }
}

extension Array {
  /// `filter` helper that skips iterating through the entire array when not applying any filtering.
  @inlinable public func optionallyFilter(
    shouldFilter: Bool,
    isIncluded: (Element) throws -> Bool
  ) rethrows -> [Element] {
    guard shouldFilter else { return self }
    return try filter(isIncluded)
  }

  /// `sort` helper that skips iterating through the entire array when not applying any sorting.
  @inlinable public func optionallySort(
    shouldSort: Bool,
    by sort: (Element, Element) throws -> Bool
  ) rethrows -> [Element] {
    guard shouldSort else { return self }
    return try sorted(by: sort)
  }
}
