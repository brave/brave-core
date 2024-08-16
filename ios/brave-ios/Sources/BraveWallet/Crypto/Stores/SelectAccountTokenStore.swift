// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

class SelectAccountTokenStore: ObservableObject, WalletObserverStore {

  struct AccountSection: Equatable, Identifiable {
    struct TokenBalance: Equatable, Identifiable {
      var id: String { token.id }
      let token: BraveWallet.BlockchainToken
      let network: BraveWallet.NetworkInfo
      let balance: Double?
      let price: String?
      let nftMetadata: BraveWallet.NftMetadata?

      init(
        token: BraveWallet.BlockchainToken,
        network: BraveWallet.NetworkInfo,
        balance: Double? = nil,
        price: String? = nil,
        nftMetadata: BraveWallet.NftMetadata? = nil
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
    let bitcoinAccountInfo: BraveWallet.BitcoinAccountInfo?
    let tokenBalances: [TokenBalance]
  }

  /// The networks to filter the tokens/accounts by.
  @Published var networkFilters: [Selectable<BraveWallet.NetworkInfo>] = [] {
    didSet {
      guard !oldValue.isEmpty else { return }  // ignore initial assignment, all networks selected
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
  /// The BTC balances for each Bitcoin account.  Key is `account.id`.
  @Published private(set) var accountsBTCBalances: [String: [BTCBalanceType: Double]] = [:]
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
  private typealias TokenBalanceCache = [String: [String: Double]]
  /// Cache of balances of each asset for each account. [account.id: [token.id: balance]]
  private var balancesForAccountsCache: TokenBalanceCache = [:]
  /// Cache of prices of assets. The key(s) are the `BraveWallet.BlockchainToken.assetRatioId` lowercased.
  private var pricesForTokensCache: [String: String] = [:]
  /// Cache of metadata for NFTs. The key(s) is the token's `id`.
  private var metadataCache: [String: BraveWallet.NftMetadata] = [:]

  let didSelect: (BraveWallet.AccountInfo, BraveWallet.BlockchainToken) -> Void
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let bitcoinWalletService: BraveWalletBitcoinWalletService
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
    bitcoinWalletService: BraveWalletBitcoinWalletService,
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType,
    query: String? = nil
  ) {
    self.didSelect = didSelect
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.bitcoinWalletService = bitcoinWalletService
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
    self.assetManager.addUserAssetDataObserver(self)
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
            let existingSelectionValue = self?.networkFilters.first(where: {
              $0.model.chainId == network.chainId
            })?.isSelected
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
  /// All` BitcoinAccountInfo` models for every Bitcoin account. Key is `accountId.uniqueKey` of the Account.
  private var bitcoinAccounts: [String: BraveWallet.BitcoinAccountInfo] = [:]
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
      let btcAccountInfos = allAccounts.filter({ $0.coin == .btc })
      if !btcAccountInfos.isEmpty {
        self.bitcoinAccounts = await bitcoinWalletService.fetchBitcoinAccountInfo(
          accounts: btcAccountInfos
        )
      }
      let allNetworkAssets = await assetManager.getUserAssets(
        networks: allNetworks,
        visible: true
      )
      let allVisibleUserAssets = allNetworkAssets.flatMap(\.tokens)
      self.userVisibleAssets = allVisibleUserAssets.reduce(
        into: [String: BraveWallet.BlockchainToken](),
        {
          $0[$1.id] = $1
        }
      )
      // show accounts with all supported tokens until balances are fetched
      self.accountSections = await buildAccountSections(
        selectedNetworks: networkFilters.filter(\.isSelected).map(\.model),
        allAccounts: allAccounts,
        bitcoinAccounts: bitcoinAccounts,
        userVisibleAssets: Array(userVisibleAssets.values),
        balancesCache: balancesForAccountsCache,
        btcBalancesCache: accountsBTCBalances,
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
        bitcoinAccounts: bitcoinAccounts,
        userVisibleAssets: Array(userVisibleAssets.values),
        balancesCache: balancesForAccountsCache,
        btcBalancesCache: accountsBTCBalances,
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
        of: TokenBalanceCache.self,
        body: { group in
          for account in allAccounts where account.coin != .btc {
            if let allTokenBalance = assetManager.getAssetBalances(for: nil, account: account.id) {
              var result: [String: Double] = [:]
              for balancePerToken in allTokenBalance {
                let tokenId =
                  balancePerToken.contractAddress + balancePerToken.chainId
                  + balancePerToken.symbol + balancePerToken.tokenId
                result.merge(with: [
                  tokenId: Double(balancePerToken.balance) ?? 0
                ])
              }
              balancesForAccountsCache.merge(with: [account.id: result])
            } else {
              group.addTask {  // get balance for all tokens this account supports
                let balancesForTokens: [String: Double] = await self.rpcService
                  .fetchBalancesForTokens(
                    account: account,
                    networkAssets: networkAssets
                  )
                return [account.id: balancesForTokens]
              }
            }
          }
          return await group.reduce(
            into: TokenBalanceCache(),
            { partialResult, new in
              partialResult.merge(with: new)
            }
          )
        }
      )
      for account in allAccounts {
        if let updatedBalancesForAccount = balancesForAccounts[account.id] {
          // if balance fetch failed that we already have cached, don't overwrite existing
          if var existing = self.balancesForAccountsCache[account.id] {
            existing.merge(with: updatedBalancesForAccount)
            self.balancesForAccountsCache[account.id] = existing
          } else {
            self.balancesForAccountsCache[account.id] = updatedBalancesForAccount
          }
        }
      }
      if allAccounts.contains(where: { $0.coin == .btc }) {
        self.accountsBTCBalances = await withTaskGroup(of: [String: [BTCBalanceType: Double]].self)
        {
          [bitcoinWalletService] group in
          for account in allAccounts where account.coin == .btc {
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
      self.balancesFetched = true
      self.updateAccountSections()
      // fetch prices for tokens with balance
      var tokensIdsWithBalance: Set<String> = .init()
      for accountBalance in balancesForAccountsCache.values {
        let tokenIdsWithAccountBalance = accountBalance.filter { $1 > 0 }.map(\.key)
        tokenIdsWithAccountBalance.forEach { tokensIdsWithBalance.insert($0) }
      }
      if accountsBTCBalances.compactMap({ $0.value[.available] }).contains(where: { $0 > 0 }) {
        let btcAssets = networkAssets.filter({ $0.network.coin == .btc }).flatMap(\.tokens)
        btcAssets.forEach { tokensIdsWithBalance.insert($0.id) }
      }
      let assetRatioIdsForTokensWithBalance =
        tokensIdsWithBalance
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
      let allMetadata = await rpcService.fetchNFTMetadata(
        tokens: userVisibleNFTs,
        ipfsApi: ipfsApi
      )
      self.metadataCache.merge(with: allMetadata)
      self.updateAccountSections()
    }
  }

  /// Builds the array of `AccountSection`s for display, taking into account selected networks and filter query.
  private func buildAccountSections(
    selectedNetworks: [BraveWallet.NetworkInfo],
    allAccounts: [BraveWallet.AccountInfo],
    bitcoinAccounts: [String: BraveWallet.BitcoinAccountInfo],
    userVisibleAssets: [BraveWallet.BlockchainToken],
    balancesCache: TokenBalanceCache,
    btcBalancesCache: [String: [BTCBalanceType: Double]],
    balancesFetched: Bool,
    pricesCache: [String: String],
    metadataCache: [String: BraveWallet.NftMetadata],
    hideZeroBalances: Bool,
    query: String,
    currencyFormatter: NumberFormatter
  ) async -> [AccountSection] {
    let accountSections: [AccountSection] = allAccounts.compactMap { account in
      let tokensForAccountCoin: [BraveWallet.BlockchainToken] =
        userVisibleAssets
        .filter({ $0.coin == account.coin })
      let accountTokenBalances: [AccountSection.TokenBalance] =
        tokensForAccountCoin
        .compactMap { token in
          if !query.isEmpty,  // only if we have a filter query
            !(token.symbol.localizedCaseInsensitiveContains(query)
              || token.name.localizedCaseInsensitiveContains(query))
          {
            // token does not match query
            return nil
          }
          // network for must be selected
          if let tokenNetwork = selectedNetworks.first(where: { $0.chainId == token.chainId }),
            // network must support account keyring
            tokenNetwork.supportedKeyrings.contains(account.keyringId.rawValue as NSNumber)
          {
            let balance: Double
            if token.coin == .btc {
              // btc has 2 tokens, but tokens are account specific (mainnet/testnet)
              balance = accountsBTCBalances[account.id]?[.available] ?? 0
            } else {
              balance = balancesCache[account.id]?[token.id] ?? 0
            }
            if hideZeroBalances, balance <= 0 {
              // token has no balance, user is hiding zero balance tokens.
              return nil
            }
            var price: String?
            if let tokenPrice = pricesCache[token.assetRatioId.lowercased()],
              balance > 0
            {
              price = currencyFormatter.formatAsFiat(
                (Double(tokenPrice) ?? 0) * balance
              )
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
        bitcoinAccountInfo: bitcoinAccounts[account.id],
        tokenBalances:
          accountTokenBalances
          .sorted { lhs, rhs in
            if lhs.network.isKnownTestnet && rhs.network.isKnownTestnet {
              return (lhs.balance ?? 0) > (rhs.balance ?? 0)
            } else if lhs.network.isKnownTestnet {
              return false  // sort test networks to end of list
            } else if rhs.network.isKnownTestnet {
              return true  // sort test networks to end of list
            }
            return (lhs.balance ?? 0) > (rhs.balance ?? 0)
          }
      )
    }

    return accountSections
  }
}

extension SelectAccountTokenStore: WalletUserAssetDataObserver {
  func cachedBalanceRefreshed() {
    Task { @MainActor in
      let allNetworks = await rpcService.allNetworksForSupportedCoins()
      let allNetworkAssets = await assetManager.getUserAssets(
        networks: allNetworks,
        visible: true
      )
      fetchAccountBalances(networkAssets: allNetworkAssets)
    }
  }

  func userAssetUpdated() {
  }
}
