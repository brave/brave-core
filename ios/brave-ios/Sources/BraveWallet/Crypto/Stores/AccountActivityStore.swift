// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

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

  /// Sort by the fiat/value of the asset (price x balance) descending, otherwise by balance when price is unavailable.
  static func sorted(lhs: AssetViewModel, rhs: AssetViewModel) -> Bool {
    if let lhsPrice = Double(lhs.price),
      let rhsPrice = Double(rhs.price)
    {
      let lhsValue = (lhsPrice * lhs.totalBalance)
      let rhsValue = (rhsPrice * rhs.totalBalance)
      if lhsValue == rhsValue, lhsValue <= 0 {
        return sameBalanceSort(lhs: lhs, rhs: rhs)
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
    return lhs.totalBalance > rhs.totalBalance
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

struct NFTAssetViewModel: Identifiable, Equatable {
  let groupType: AssetGroupType
  var token: BraveWallet.BlockchainToken
  var network: BraveWallet.NetworkInfo
  /// Balance for the NFT for each account address. The key is the account address.
  var balanceForAccounts: [String: Int]
  var nftMetadata: BraveWallet.NftMetadata?

  public var id: String {
    token.id + network.chainId
  }

  static func == (lhs: NFTAssetViewModel, rhs: NFTAssetViewModel) -> Bool {
    lhs.id == rhs.id
  }
}

class AccountActivityStore: ObservableObject, WalletObserverStore {
  /// If we want to observe selected account changes (ex. in `WalletPanelView`).
  /// In some cases, we do not want to update the account displayed when the
  /// selected account changes (ex. when removing an account).
  let isWalletPanel: Bool
  @Published private(set) var account: BraveWallet.AccountInfo {
    didSet {
      guard oldValue != account else { return }
      tokenBalanceCache.removeAll()
    }
  }
  @Published private(set) var isLoadingAccountFiat: Bool = false
  @Published private(set) var accountTotalFiat: String = "$0.00"
  @Published private(set) var userAssets: [AssetViewModel] = []
  @Published private(set) var userNFTs: [NFTAssetViewModel] = []
  /// Sections of transactions for display. Each section represents one date.
  @Published var transactionSections: [TransactionSection] = []
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard oldValue != currencyCode else { return }
      update()
    }
  }

  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter

  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let rpcService: BraveWalletJsonRpcService
  private let assetRatioService: BraveWalletAssetRatioService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private let ipfsApi: IpfsAPI
  private let bitcoinWalletService: BraveWalletBitcoinWalletService
  private let zcashWalletService: BraveWalletZCashWalletService
  private let assetManager: WalletUserAssetManagerType
  /// Cache for storing `BlockchainToken`s that are not in user assets or our token registry.
  /// This could occur with a dapp creating a transaction.
  private var tokenInfoCache: [BraveWallet.BlockchainToken] = []
  private var tokenBalanceCache: [String: Double] = [:]
  private var tokenPricesCache: [BraveWallet.AssetPrice] = []
  private var nftMetadataCache: [String: BraveWallet.NftMetadata] = [:]
  private var solEstimatedTxFeesCache: [String: UInt64] = [:]
  private var btcBalancesCache: [BTCBalanceType: Double] = [:]

  private var keyringServiceObserver: KeyringServiceObserver?
  private var rpcServiceObserver: JsonRpcServiceObserver?
  private var txServiceObserver: TxServiceObserver?
  private var walletServiceObserver: WalletServiceObserver?

  var isObserving: Bool {
    keyringServiceObserver != nil && rpcServiceObserver != nil
      && txServiceObserver != nil && walletServiceObserver != nil
  }

  init(
    account: BraveWallet.AccountInfo,
    isWalletPanel: Bool,
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService,
    rpcService: BraveWalletJsonRpcService,
    assetRatioService: BraveWalletAssetRatioService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    ipfsApi: IpfsAPI,
    bitcoinWalletService: BraveWalletBitcoinWalletService,
    zcashWalletService: BraveWalletZCashWalletService,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.account = account
    self.isWalletPanel = isWalletPanel
    self.keyringService = keyringService
    self.walletService = walletService
    self.rpcService = rpcService
    self.assetRatioService = assetRatioService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.solTxManagerProxy = solTxManagerProxy
    self.ipfsApi = ipfsApi
    self.bitcoinWalletService = bitcoinWalletService
    self.zcashWalletService = zcashWalletService
    self.assetManager = userAssetManager

    self.setupObservers()

    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
  }

  func tearDown() {
    keyringServiceObserver = nil
    rpcServiceObserver = nil
    txServiceObserver = nil
    walletServiceObserver = nil
    transactionDetailsStore?.tearDown()
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.assetManager.addUserAssetDataObserver(self)
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _accountsChanged: {
        Task { @MainActor in
          let allAccounts = await self.keyringService.allAccounts()
          if let account = allAccounts.accounts.first(where: {
            $0.accountId == self.account.accountId
          }) {
            // user may have updated the account name
            self.account = account
          }
        }
      },
      _selectedWalletAccountChanged: { [weak self] account in
        guard let self, self.isWalletPanel else { return }
        self.account = account
        self.update()
      },
      _selectedDappAccountChanged: { [weak self] _, account in
        guard let self, self.isWalletPanel, let account else { return }
        self.account = account
        self.update()
      }
    )
    self.rpcServiceObserver = JsonRpcServiceObserver(
      rpcService: rpcService,
      _chainChangedEvent: { [weak self] _, _, _ in
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
          // Handle small gap between chain changing and txController having the correct chain Id
          self?.update()
        }
      }
    )
    self.txServiceObserver = TxServiceObserver(
      txService: txService,
      _onNewUnapprovedTx: { [weak self] _ in
        self?.update()
      },
      _onTransactionStatusChanged: { [weak self] _ in
        self?.update()
      }
    )
    self.walletServiceObserver = WalletServiceObserver(
      walletService: walletService,
      _onDefaultBaseCurrencyChanged: { [weak self] currency in
        self?.currencyCode = currency
      }
    )
  }

  func update() {
    Task { @MainActor in
      let allNetworks = await rpcService.allNetworksForSupportedCoins()
      let networksForAccountCoin = await rpcService.allNetworks(for: [account.coin])
      let networksForAccount = networksForAccountCoin.filter {
        // .fil coin type has two different keyring ids
        $0.supportedKeyrings.contains(account.keyringId.rawValue as NSNumber)
      }

      // Include user deleted for case user sent an NFT
      // then deleted it, we need it for display in transaction list
      let allUserNetworkAssets =
        await assetManager.getAllUserAssetsInNetworkAssets(
          networks: networksForAccount,
          includingUserDeleted: true
        )
      let allHiddenTokens =
        await assetManager.getUserAssets(
          networks: allNetworks,
          visible: false
        ).flatMap(\.tokens)
      let allVisibleNetworkAssets =
        allUserNetworkAssets.map {
          NetworkAssets(
            network: $0.network,
            tokens: $0.tokens.filter({ token in
              !allHiddenTokens.contains { hiddenToken in
                hiddenToken.id.caseInsensitiveCompare(token.id) == .orderedSame
              }
            }),
            sortOrder: $0.sortOrder
          )
        }
      let allUserAssets = allUserNetworkAssets.flatMap(\.tokens)
      let allTokens = await blockchainRegistry.allTokens(in: networksForAccountCoin).flatMap(
        \.tokens
      )
      let allAccountsForCoin = await keyringService.allAccounts().accounts.filter {
        $0.coin == account.coin
      }
      let transactions = await txService.allTransactions(
        networks: networksForAccountCoin,
        for: account
      )
      self.transactionSections = buildTransactionSections(
        transactions: transactions,
        allNetworks: allNetworks,
        accountInfos: allAccountsForCoin,
        userAssets: allUserAssets,
        allTokens: allTokens,
        tokenPrices: tokenPricesCache,
        nftMetadata: nftMetadataCache,
        solEstimatedTxFees: solEstimatedTxFeesCache
      )

      self.isLoadingAccountFiat = true
      var tokenBalances: [String: Double] = [:]
      if account.coin == .btc {
        let networkAsset = allUserNetworkAssets.first {
          $0.network.supportedKeyrings.contains(account.keyringId.rawValue as NSNumber)
        }
        btcBalancesCache = await self.bitcoinWalletService.fetchBTCBalances(
          accountId: account.accountId
        )
        if let btcToken = networkAsset?.tokens.first,
          let btcTotalBalance = btcBalancesCache[.total]
        {
          tokenBalances = [btcToken.id: btcTotalBalance]
        }
      } else {
        if let accountBalances = self.assetManager.getAssetBalances(for: nil, account: account.id) {
          tokenBalances = accountBalances.reduce(into: [String: Double]()) {
            let tokenId =
              $1.contractAddress + $1.chainId
              + $1.symbol + $1.tokenId
            $0[tokenId] = Double($1.balance) ?? 0
          }
        } else {
          if account.coin == .zec {
            let zecNetworkAsset = allUserNetworkAssets.first {
              $0.network.supportedKeyrings.contains(account.keyringId.rawValue as NSNumber)
            }
            if let zecToken = zecNetworkAsset?.tokens.first {
              let zecBalance =
                await self.zcashWalletService.fetchZECTransparentBalances(
                  accountId: account.accountId
                ) ?? 0
              tokenBalances = [zecToken.id: zecBalance]
            }
          } else {
            tokenBalances = await self.rpcService.fetchBalancesForTokens(
              account: account,
              networkAssets: allUserNetworkAssets
            )
          }
        }
      }
      tokenBalanceCache.merge(with: tokenBalances)
      // update assets, NFTs, after balance fetch
      guard !Task.isCancelled else { return }
      (self.userAssets, self.userNFTs) = buildAssetsAndNFTs(
        userNetworkAssets: allVisibleNetworkAssets,
        tokenBalances: tokenBalanceCache,
        tokenPrices: tokenPricesCache,
        nftMetadata: nftMetadataCache,
        btcBalances: btcBalancesCache
      )

      // fetch price for every user asset
      let prices: [BraveWallet.AssetPrice] = await assetRatioService.fetchPrices(
        for: allUserAssets,
        vsCurrency: currencyFormatter.currencyCode
      )
      tokenPricesCache.update(with: prices)

      var totalFiat: Double = 0
      for (key, balance) in tokenBalances where balance > 0 {
        if let token = allUserAssets.first(where: { $0.id == key }) {
          let assetPrice = prices.getTokenPrice(for: token)
          let price = Double(assetPrice?.price ?? "0") ?? 0
          let tokenFiat = balance * price
          totalFiat += tokenFiat
        }
      }
      self.accountTotalFiat = currencyFormatter.formatAsFiat(totalFiat) ?? "$0.00"
      self.isLoadingAccountFiat = false

      // update assets, NFTs, transactions after balance & price fetch
      guard !Task.isCancelled else { return }
      (self.userAssets, self.userNFTs) = buildAssetsAndNFTs(
        userNetworkAssets: allVisibleNetworkAssets,
        tokenBalances: tokenBalanceCache,
        tokenPrices: tokenPricesCache,
        nftMetadata: nftMetadataCache,
        btcBalances: btcBalancesCache
      )
      self.transactionSections = buildTransactionSections(
        transactions: transactions,
        allNetworks: allNetworks,
        accountInfos: allAccountsForCoin,
        userAssets: allUserAssets,
        allTokens: allTokens,
        tokenPrices: tokenPricesCache,
        nftMetadata: nftMetadataCache,
        solEstimatedTxFees: solEstimatedTxFeesCache
      )

      // fetch NFTs metadata
      let allNFTMetadata = await rpcService.fetchNFTMetadata(
        tokens: userNFTs.map(\.token),
        ipfsApi: ipfsApi
      )
      nftMetadataCache.merge(with: allNFTMetadata)
      // update assets, NFTs, transactions after balance & price & metadata fetch
      guard !Task.isCancelled else { return }
      (self.userAssets, self.userNFTs) = buildAssetsAndNFTs(
        userNetworkAssets: allVisibleNetworkAssets,
        tokenBalances: tokenBalanceCache,
        tokenPrices: tokenPricesCache,
        nftMetadata: nftMetadataCache,
        btcBalances: btcBalancesCache
      )
      self.transactionSections = buildTransactionSections(
        transactions: transactions,
        allNetworks: allNetworks,
        accountInfos: allAccountsForCoin,
        userAssets: allUserAssets,
        allTokens: allTokens,
        tokenPrices: tokenPricesCache,
        nftMetadata: nftMetadataCache,
        solEstimatedTxFees: solEstimatedTxFeesCache
      )

      if !transactions.isEmpty {
        var solEstimatedTxFees: [String: UInt64] = [:]
        switch account.coin {
        case .eth:
          // Gather known information about the transaction(s) tokens
          let unknownTokenInfo = transactions.unknownTokenContractAddressChainIdPairs(
            knownTokens: allUserAssets + allTokens + tokenInfoCache
          )
          if !unknownTokenInfo.isEmpty {
            let unknownTokens: [BraveWallet.BlockchainToken] = await rpcService.fetchEthTokens(
              for: unknownTokenInfo
            )
            tokenInfoCache.append(contentsOf: unknownTokens)
          }
        case .sol:
          solEstimatedTxFees = await solTxManagerProxy.solanaTxFeeEstimations(for: transactions)
          self.solEstimatedTxFeesCache.merge(with: solEstimatedTxFees)
        default:
          break
        }
        self.transactionSections = buildTransactionSections(
          transactions: transactions,
          allNetworks: allNetworks,
          accountInfos: allAccountsForCoin,
          userAssets: allUserAssets,
          allTokens: allTokens,
          tokenPrices: tokenPricesCache,
          nftMetadata: allNFTMetadata,
          solEstimatedTxFees: solEstimatedTxFeesCache
        )
      }
    }
  }

  @MainActor func handleTransactionFollowUpAction(
    _ action: TransactionFollowUpAction,
    transaction: BraveWallet.TransactionInfo
  ) async -> String? {
    guard
      let errorMessage = await txService.handleTransactionFollowUpAction(
        action,
        transaction: transaction
      )
    else {
      return nil
    }
    return errorMessage
  }

  private func buildAssetsAndNFTs(
    userNetworkAssets: [NetworkAssets],
    tokenBalances: [String: Double],
    tokenPrices: [BraveWallet.AssetPrice],
    nftMetadata: [String: BraveWallet.NftMetadata],
    btcBalances: [BTCBalanceType: Double]
  ) -> ([AssetViewModel], [NFTAssetViewModel]) {
    var updatedUserAssets: [AssetViewModel] = []
    var updatedUserNFTs: [NFTAssetViewModel] = []
    for networkAssets in userNetworkAssets {
      for token in networkAssets.tokens where token.visible {
        if token.isErc721 || token.isNft {
          guard Int(tokenBalances[token.id] ?? 0) > 0 else {
            // only show NFTs belonging to this account
            continue
          }
          updatedUserNFTs.append(
            NFTAssetViewModel(
              groupType: .none,
              token: token,
              network: networkAssets.network,
              balanceForAccounts: [account.id: Int(tokenBalances[token.id] ?? 0)],
              nftMetadata: nftMetadata[token.id]
            )
          )
        } else {
          guard (tokenBalances[token.id] ?? 0) > 0 else {
            // only show assets belonging to this account
            continue
          }
          updatedUserAssets.append(
            AssetViewModel(
              groupType: .none,
              token: token,
              network: networkAssets.network,
              price: tokenPrices.getTokenPrice(for: token)?.price ?? "",
              history: [],
              balanceForAccounts: [account.id: tokenBalances[token.id] ?? 0],
              btcBalances: token.coin == .btc ? [account.id: btcBalancesCache] : [:]
            )
          )
        }
      }
    }
    updatedUserAssets = updatedUserAssets.sorted(by: AssetViewModel.sorted)

    return (updatedUserAssets, updatedUserNFTs)
  }

  private func buildTransactionSections(
    transactions: [BraveWallet.TransactionInfo],
    allNetworks: [BraveWallet.NetworkInfo],
    accountInfos: [BraveWallet.AccountInfo],
    userAssets: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    tokenPrices: [BraveWallet.AssetPrice],
    nftMetadata: [String: BraveWallet.NftMetadata],
    solEstimatedTxFees: [String: UInt64]
  ) -> [TransactionSection] {
    // Group transactions by day (only compare day/month/year)
    let transactionsGroupedByDate = Dictionary(grouping: transactions) { transaction in
      let dateComponents = Calendar.current.dateComponents(
        [.year, .month, .day],
        from: transaction.createdTime
      )
      return Calendar.current.date(from: dateComponents) ?? transaction.createdTime
    }

    // Map to 1 `TransactionSection` per date
    return transactionsGroupedByDate.keys.sorted(by: { $0 > $1 }).compactMap { date in
      let transactions = transactionsGroupedByDate[date] ?? []
      guard !transactions.isEmpty else { return nil }
      let parsedTransactions: [ParsedTransaction] =
        transactions
        .sorted(by: { $0.createdTime > $1.createdTime })
        .compactMap { transaction in
          return TransactionParser.parseTransaction(
            transaction: transaction,
            allNetworks: allNetworks,
            accountInfos: accountInfos,
            userAssets: userAssets,
            allTokens: allTokens + tokenInfoCache,
            assetRatios: tokenPrices,
            nftMetadata: nftMetadata,
            solEstimatedTxFee: solEstimatedTxFees[transaction.id],
            currencyFormatter: currencyFormatter,
            decimalFormatStyle: .decimals(precision: 4)
          )
        }
      return TransactionSection(
        date: date,
        transactions: parsedTransactions
      )
    }
  }

  private var transactionDetailsStore: TransactionDetailsStore?
  func transactionDetailsStore(
    for transaction: BraveWallet.TransactionInfo
  ) -> TransactionDetailsStore {
    let parsedTransaction =
      transactionSections
      .flatMap(\.transactions)
      .first(where: { $0.transaction.id == transaction.id })
    let transactionDetailsStore = TransactionDetailsStore(
      transaction: transaction,
      parsedTransaction: parsedTransaction,
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      txService: txService,
      solanaTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      userAssetManager: assetManager
    )
    self.transactionDetailsStore = transactionDetailsStore
    return transactionDetailsStore
  }

  func closeTransactionDetailsStore() {
    self.transactionDetailsStore?.tearDown()
    self.transactionDetailsStore = nil
  }
}

extension AccountActivityStore: WalletUserAssetDataObserver {
  public func cachedBalanceRefreshed() {
    update()
  }

  public func userAssetUpdated() {
    // auto-discovery found new asset, user changed
    // visibility status, or added new custom asset
    update()
  }
}
