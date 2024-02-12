// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Preferences

struct NFTGroupViewModel: WalletAssetGroupViewModel, Equatable, Identifiable {
  typealias ViewModel = NFTAssetViewModel
  
  var groupType: AssetGroupType
  var assets: [NFTAssetViewModel]
  var id: String {
    "\(groupType.id) \(title)"
  }
}

struct NFTAssetViewModel: Identifiable, Equatable {
  let groupType: AssetGroupType
  var token: BraveWallet.BlockchainToken
  var network: BraveWallet.NetworkInfo
  /// Balance for the NFT for each account address. The key is the account address.
  var balanceForAccounts: [String: Int]
  var nftMetadata: NFTMetadata?
  
  public var id: String {
    token.id + network.chainId
  }
  
  static func == (lhs: NFTAssetViewModel, rhs: NFTAssetViewModel) -> Bool {
    lhs.id == rhs.id
  }
}

public class NFTStore: ObservableObject, WalletObserverStore {
  /// The current displayed NFT groups
  var displayNFTGroups: [NFTGroupViewModel] {
    switch displayType {
    case .visible:
      return userNFTGroups.map {
        NFTGroupViewModel(
          groupType: $0.groupType,
          assets: $0.assets.filter(\.token.visible)
        )
      }
    case .hidden:
      return userNFTGroups.map {
        NFTGroupViewModel(
          groupType: $0.groupType,
          assets: $0.assets.filter { nft in
            !nft.token.visible
          }
        )
      }
    }
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
  @Published var isLoadingDiscoverAssets: Bool = false
  
  public private(set) lazy var userAssetsStore: UserAssetsStore = .init(
    blockchainRegistry: self.blockchainRegistry,
    rpcService: self.rpcService,
    keyringService: self.keyringService,
    assetRatioService: self.assetRatioService,
    walletService: self.walletService,
    ipfsApi: self.ipfsApi,
    userAssetManager: self.assetManager
  )
  
  enum NFTDisplayType: Int, CaseIterable, Identifiable {
    case visible
    case hidden
    
    var id: Int {
      rawValue
    }
    
    var dropdownTitle: String {
      switch self {
      case .visible:
        return Strings.Wallet.nftCollected
      case .hidden:
        return Strings.Wallet.nftHidden
      }
    }
    
    var emptyTitle: String {
      switch self {
      case .visible:
        return Strings.Wallet.nftPageEmptyTitle
      case .hidden:
        return Strings.Wallet.nftInvisiblePageEmptyTitle
      }
    }
    
    var emptyDescription: String? {
      switch self {
      case .visible:
        return Strings.Wallet.nftPageEmptyDescription
      case .hidden:
        return nil
      }
    }
  }

  /// Current group to display
  @Published var displayType: NFTDisplayType = .visible
  /// View model for all NFT include visible, hidden and spams
  @Published private(set) var userNFTGroups: [NFTGroupViewModel] = []
  /// showing shimmering loading state when the view is fetching non-fungible tokens without fetching its metadata or balance
  @Published var isLoadingJunkNFTs: Bool = false
  
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let ipfsApi: IpfsAPI
  private let assetManager: WalletUserAssetManagerType
  private let txService: BraveWalletTxService
  private var rpcServiceObserver: JsonRpcServiceObserver?
  private var keyringServiceObserver: KeyringServiceObserver?
  private var walletServiveObserver: WalletServiceObserver?
  private var txServiceObserver: TxServiceObserver?
  
  /// Cancellable for the last running `update()` Task.
  private var updateTask: Task<(), Never>?
  /// Cache of metadata for NFTs. The key is the token's `id`.
  private var metadataCache: [String: NFTMetadata] = [:]
  /// Spam from SimpleHash in form of `NetworkAssets`
  private var simpleHashSpamNFTs: [NetworkAssets] = [] {
    didSet {
      update()
    }
  }
  
  var isObserving: Bool {
    rpcServiceObserver != nil && keyringServiceObserver != nil && walletServiveObserver != nil && txServiceObserver != nil
  }
  
  var isShowingNFTEmptyState: Bool {
    if filters.groupBy == .none, let noneGroup = displayNFTGroups.first {
      return noneGroup.assets.isEmpty
    }
    return displayNFTGroups.isEmpty
  }
  
  var totalDisplayedNFTCount: Int {
    displayNFTGroups.reduce(0) { $0 + $1.assets.count }
  }
  
  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType,
    txService: BraveWalletTxService
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry
    self.ipfsApi = ipfsApi
    self.assetManager = userAssetManager
    self.txService = txService
    
    self.setupObservers()
    
    keyringService.isLocked { [self] isLocked in
      if !isLocked {
        update()
      }
    }
    Preferences.Wallet.showTestNetworks.observe(from: self)
    Preferences.Wallet.isHidingUnownedNFTsFilter.observe(from: self)
    Preferences.Wallet.isShowingNFTNetworkLogoFilter.observe(from: self)
    Preferences.Wallet.nonSelectedNetworksFilter.observe(from: self)
    Preferences.Wallet.groupByFilter.observe(from: self)
  }
  
  func tearDown() {
    rpcServiceObserver = nil
    keyringServiceObserver = nil
    walletServiveObserver = nil
    txServiceObserver = nil
    
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
        self?.update()
      }
    )
    self.walletServiveObserver = WalletServiceObserver(
      walletService: walletService,
      _onNetworkListChanged: { [weak self] in
        // A network was added or removed, `update()` will update `allNetworks`.
        self?.update()
      },
      _onDiscoverAssetsStarted: { [weak self] in
        self?.isLoadingDiscoverAssets = true
      },
      _onDiscoverAssetsCompleted: { [weak self] _ in
        self?.isLoadingDiscoverAssets = false
        // assets update will be called via `CryptoStore`
      }
    )
    self.txServiceObserver = TxServiceObserver(
      txService: txService, _onTransactionStatusChanged: { [weak self] txInfo in
        if txInfo.txStatus == .confirmed, txInfo.isSend, (txInfo.coin == .eth || txInfo.coin == .sol) {
          self?.update(forceUpdateNFTBalances: true)
        }
      }
    )
    
    userAssetsStore.setupObservers()
  }
  
  /// Cache of NFT balances for each account tokenBalances: [token.contractAddress]
  private var nftBalancesCache: [String: [String: Int]] = [:]
  
  func update(forceUpdateNFTBalances: Bool = false) {
    self.updateTask?.cancel()
    self.updateTask = Task { @MainActor in
      self.allAccounts = await keyringService.allAccounts().accounts
        .filter { account in
          WalletConstants.supportedCoinTypes().contains(account.coin)
        }
      self.allNetworks = await rpcService.allNetworksForSupportedCoins()
      let filters = self.filters
      let selectedAccounts = filters.accounts.filter(\.isSelected).map(\.model)
      let selectedNetworks = filters.networks.filter(\.isSelected).map(\.model)
      
      // First display grids with placeholder since we haven't fetched balance
      // or metadata
      let unionedSpamNFTs = computeSpamNFTs(
        selectedNetworks: selectedNetworks,
        selectedAccounts: selectedAccounts,
        simpleHashSpamNFTs: simpleHashSpamNFTs
      )
      let (userNFTGroups, allUserNFTs) = buildNFTGroupModels(
        groupBy: filters.groupBy,
        spams: unionedSpamNFTs,
        selectedAccounts: selectedAccounts,
        selectedNetworks: selectedNetworks
      )
      self.userNFTGroups = userNFTGroups
      
      // Then, we fetch balance and update the UI
      // if we're not hiding unowned or grouping by account, balance isn't needed
      if filters.isHidingUnownedNFTs || filters.groupBy == .accounts {
        let allAccounts = filters.accounts.map(\.model)
        nftBalancesCache = await withTaskGroup(
          of: [String: [String: Int]].self,
          body: { @MainActor [nftBalancesCache, rpcService] group in
            for nft in allUserNFTs { // for all NFTs that have not yet been fetched its balance
              guard let networkForNFT = allNetworks.first(where: { $0.chainId == nft.chainId }) else {
                continue
              }
              group.addTask { @MainActor in
                let updatedBalances = await withTaskGroup(
                  of: [String: Int].self,
                  body: { @MainActor group in
                    for account in allAccounts where account.coin == nft.coin {
                      if !forceUpdateNFTBalances, let cachedBalance = nftBalancesCache[nft.id]?[account.address] { // cached balance
                        return [account.address: cachedBalance]
                      } else { // no balance for this account
                        group.addTask { @MainActor in
                          let balanceForToken = await rpcService.balance(
                            for: nft,
                            in: account,
                            network: networkForNFT
                          )
                          return [account.address: Int(balanceForToken ?? 0)]
                        }
                      }
                    }
                    return await group.reduce(into: [String: Int](), { partialResult, new in
                      partialResult.merge(with: new)
                    })
                  })
                var tokenBalances = nftBalancesCache[nft.id] ?? [:]
                tokenBalances.merge(with: updatedBalances)
                return [nft.id: tokenBalances]
              }
            }
            return await group.reduce(into: [String: [String: Int]](), { partialResult, new in
              partialResult.merge(with: new)
            })
          })
      }
      guard !Task.isCancelled else { return }
      let (userNFTGroupsWithBalance, _) = buildNFTGroupModels(
        groupBy: filters.groupBy,
        spams: unionedSpamNFTs,
        selectedAccounts: selectedAccounts,
        selectedNetworks: selectedNetworks
      )
      self.userNFTGroups = userNFTGroupsWithBalance
      
      // Last, we fet fetch nft metadata for NFTs that do not have metadata loaded
      // and update the UI
      let nftsMissingMetadata = allUserNFTs.filter { metadataCache[$0.id] == nil }
      let fetchedMetadata = await rpcService.fetchNFTMetadata(tokens: nftsMissingMetadata, ipfsApi: ipfsApi)
      metadataCache.merge(with: fetchedMetadata)
      
      guard !Task.isCancelled else { return }
      let (userNFTGroupsWithMetadata, _) = buildNFTGroupModels(
        groupBy: filters.groupBy,
        spams: unionedSpamNFTs,
        selectedAccounts: selectedAccounts,
        selectedNetworks: selectedNetworks
      )
      self.userNFTGroups = userNFTGroupsWithMetadata
    }
  }
  
  func updateNFTMetadataCache(for token: BraveWallet.BlockchainToken, metadata: NFTMetadata) {
    metadataCache[token.id] = metadata
    var updatedGroups: [NFTGroupViewModel] = []
    for group in userNFTGroups {
      if let index = group.assets.firstIndex(where: { $0.token.id == token.id }) {
        var newAssets = group.assets
        newAssets[index].nftMetadata = metadata
        updatedGroups.append(.init(groupType: group.groupType, assets: newAssets))
      } else {
        updatedGroups.append(group)
      }
    }
    userNFTGroups = updatedGroups
  }
  
  private func buildNFTAssetViewModels(
    for groupType: AssetGroupType,
    allUserNFTs: [NetworkAssets]
  ) -> [NFTAssetViewModel] {
    let selectedAccounts = self.filters.accounts.filter(\.isSelected).map(\.model)
    switch groupType {
    case .none:
      return allUserNFTs.flatMap { networkAssets in
        networkAssets.tokens.map { token in
          NFTAssetViewModel(
            groupType: groupType,
            token: token,
            network: networkAssets.network,
            balanceForAccounts: nftBalancesCache[token.id] ?? [:],
            nftMetadata: metadataCache[token.id]
          )
        }
      }
      .optionallyFilterUnownedNFTs(
        isHidingUnownedNFTs: filters.isHidingUnownedNFTs,
        selectedAccounts: selectedAccounts
      )
      .optionallySort(shouldSort: true) { first, second in
        first.token.symbol < second.token.symbol
      }
    case let .network(network):
      guard let networkNFTs = allUserNFTs
        .first(where: { $0.network.chainId == network.chainId && $0.network.coin == network.coin }) else {
        return []
      }
      return networkNFTs.tokens
        .map { token in
          NFTAssetViewModel(
            groupType: groupType,
            token: token,
            network: networkNFTs.network,
            balanceForAccounts: nftBalancesCache[token.id] ?? [:],
            nftMetadata: metadataCache[token.id]
          )
        }
        .optionallyFilterUnownedNFTs(
          isHidingUnownedNFTs: filters.isHidingUnownedNFTs,
          selectedAccounts: selectedAccounts
        )
        .optionallySort(shouldSort: true) { first, second in
          first.token.symbol < second.token.symbol
        }
    case let .account(account):
      return allUserNFTs
        .filter { $0.network.coin == account.coin && $0.network.supportedKeyrings.contains(account.accountId.keyringId.rawValue as NSNumber)
        }
        .flatMap { networkNFTs in
          networkNFTs.tokens.compactMap { token in
            // we need to exclude any NFT that THIS account does not own (balance is not 1)
            guard let balance = nftBalancesCache[token.id]?[account.address], balance > 0 else { return nil }
            return NFTAssetViewModel(
              groupType: groupType,
              token: token,
              network: networkNFTs.network,
              balanceForAccounts: nftBalancesCache[token.id] ?? [:],
              nftMetadata: metadataCache[token.id]
            )
          }
        }
        .optionallyFilterUnownedNFTs(
          isHidingUnownedNFTs: true,
          selectedAccounts: selectedAccounts
        )
        .optionallySort(shouldSort: true) { first, second in
          first.token.symbol < second.token.symbol
        }
    }
  }
  
  private func generateAllNFTsInNetworks(
    userVisibleNFTs: [NetworkAssets],
    userHiddenNFTs: [NetworkAssets],
    computedSpamNFTs: [NetworkAssets]
  ) -> [NetworkAssets] {
    var allNetworkNFTs: [NetworkAssets] = []
    for networkNFTs in userVisibleNFTs {
      let hiddenNFTs = userHiddenNFTs.first(where: {
        $0.network.chainId == networkNFTs.network.chainId && $0.network.coin == networkNFTs.network.coin
      })?.tokens ?? []
      let spamNFTs = computedSpamNFTs.first(where: {
        $0.network.chainId == networkNFTs.network.chainId && $0.network.coin == networkNFTs.network.coin
      })?.tokens ?? []
      allNetworkNFTs.append(.init(network: networkNFTs.network, tokens: networkNFTs.tokens + hiddenNFTs + spamNFTs, sortOrder: networkNFTs.sortOrder))
    }
    return allNetworkNFTs
  }
  
  private func computeSpamNFTs(
    selectedNetworks: [BraveWallet.NetworkInfo],
    selectedAccounts: [BraveWallet.AccountInfo],
    simpleHashSpamNFTs: [NetworkAssets]
  ) -> [NetworkAssets] {
    // all user marked deleted NFTs
    let allUserMarkedDeletedNFTs = assetManager.getAllUserDeletedNFTs()
    // all spam NFTs marked by user
    let allUserMarkedSpamNFTs = assetManager.getAllUserNFTs(networks: selectedNetworks, isSpam: true)
    // filter out any spam NFTs from `simpleHashSpamNFTs` that are marked
    // not-spam or deleted by user
    var updatedSimpleHashSpamNFTs: [NetworkAssets] = []
    for simpleHashSpamNFTsOnNetwork in simpleHashSpamNFTs {
      let userMarkedNotSpamTokensOnNetwork = assetManager.getAllUserNFTs(networks: [simpleHashSpamNFTsOnNetwork.network], isSpam: false).flatMap(\.tokens)
      let filteredSimpleHashSpamTokens = simpleHashSpamNFTsOnNetwork.tokens.filter { simpleHashSpamToken in
        return !userMarkedNotSpamTokensOnNetwork.contains { token in
          token.id == simpleHashSpamToken.id
        } && !allUserMarkedDeletedNFTs.contains(where: { deletedNFT in
          deletedNFT.contractAddress == simpleHashSpamToken.contractAddress && deletedNFT.chainId == simpleHashSpamToken.chainId && deletedNFT.tokenId == simpleHashSpamToken.tokenId
        })
      }
      updatedSimpleHashSpamNFTs.append(NetworkAssets(network: simpleHashSpamNFTsOnNetwork.network, tokens: filteredSimpleHashSpamTokens, sortOrder: simpleHashSpamNFTsOnNetwork.sortOrder))
    }
    // union user marked spam NFTs with spam NFTs from SimpleHash
    var computedSpamNFTs: [NetworkAssets] = []
    for (index, network) in selectedNetworks.enumerated() {
      let userMarkedSpamNFTsOnNetwork: [BraveWallet.BlockchainToken] = allUserMarkedSpamNFTs.first { $0.network.chainId == network.chainId }?.tokens ?? []
      let simpleHashSpamNFTsOnNetwork = updatedSimpleHashSpamNFTs.first { $0.network.chainId == network.chainId }?.tokens ?? []
      var spamNFTUnion: [BraveWallet.BlockchainToken] = userMarkedSpamNFTsOnNetwork // we take all spam marked by user
      // then iterate through spams from SimpleHash
      for simpleHashSpam in simpleHashSpamNFTsOnNetwork where !userMarkedSpamNFTsOnNetwork.contains(where: { userSpam in
        userSpam.id == simpleHashSpam.id
      }) {
        // make sure flags are correct (`isSpam` and `visible` might not be the true value for spams from
        // from SimpleHash)
        simpleHashSpam.isSpam = true
        simpleHashSpam.visible = false
        spamNFTUnion.append(simpleHashSpam)
      }
      computedSpamNFTs.append(NetworkAssets(network: network, tokens: spamNFTUnion, sortOrder: index))
    }
    return computedSpamNFTs
  }
  
  private func buildNFTGroupModels(
    groupBy: GroupBy,
    spams: [NetworkAssets],
    selectedAccounts: [BraveWallet.AccountInfo],
    selectedNetworks: [BraveWallet.NetworkInfo]
  ) -> ([NFTGroupViewModel], [BraveWallet.BlockchainToken]) {
    
    // user visible NFTs
    let userVisibleNFTs = assetManager.getAllUserAssetsInNetworkAssetsByVisibility(networks: selectedNetworks, visible: true)
      .map { networkAssets in
        NetworkAssets(
          network: networkAssets.network,
          tokens: networkAssets.tokens.filter { $0.isNft || $0.isErc721 },
          sortOrder: networkAssets.sortOrder
        )
      }
    // user hidden NFTs
    let userHiddenNFTs = assetManager.getAllUserAssetsInNetworkAssetsByVisibility(networks: selectedNetworks, visible: false)
      .map { networkAssets in
        NetworkAssets(
          network: networkAssets.network,
          tokens: networkAssets.tokens.filter { $0.isNft || $0.isErc721 },
          sortOrder: networkAssets.sortOrder
        )
      }
    
    let allUserNFTsInNetworks = generateAllNFTsInNetworks(
      userVisibleNFTs: userVisibleNFTs,
      userHiddenNFTs: userHiddenNFTs,
      computedSpamNFTs: spams
    )
    
    let allUserNFTs = (userVisibleNFTs + userHiddenNFTs + spams).flatMap(\.tokens)

    let groups: [NFTGroupViewModel]
    switch filters.groupBy {
    case .none:
      let assets = buildNFTAssetViewModels(
        for: .none,
        allUserNFTs: allUserNFTsInNetworks
      )
      return ([
        .init(
          groupType: .none,
          assets: assets
        )
      ], allUserNFTs)
    case .accounts:
      groups = selectedAccounts.map { account in
        let groupType: AssetGroupType = .account(account)
        let assets = buildNFTAssetViewModels(
          for: .account(account),
          allUserNFTs: allUserNFTsInNetworks
        )
        return NFTGroupViewModel(
          groupType: groupType,
          assets: assets
        )
      }
    case .networks:
      groups = selectedNetworks.map { network in
        let groupType: AssetGroupType = .network(network)
        let assets = buildNFTAssetViewModels(
          for: .network(network),
          allUserNFTs: allUserNFTsInNetworks
        )
        return NFTGroupViewModel(
          groupType: groupType,
          assets: assets
        )
      }
    }
    return (groups, allUserNFTs)
  }
  
  func fetchJunkNFTs() {
    Task { @MainActor in
      // all spam NFTs marked by SimpleHash (for all accounts on all networks)
      self.isLoadingJunkNFTs = true
      let allAccounts = await keyringService.allAccounts().accounts
        .filter { account in
          WalletConstants.supportedCoinTypes().contains(account.coin)
        }
      let allNetworks = await rpcService.allNetworksForSupportedCoins()
      self.simpleHashSpamNFTs = await walletService.simpleHashSpamNFTs(for: allAccounts, on: allNetworks)
      self.isLoadingJunkNFTs = false
    }
  }
  
  @MainActor func isNFTDiscoveryEnabled() async -> Bool {
    await walletService.nftDiscoveryEnabled()
  }
  
  func enableNFTDiscovery() {
    walletService.setNftDiscoveryEnabled(true)
  }
  
  func updateNFTStatus(
    _ token: BraveWallet.BlockchainToken,
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool
  ) {
    assetManager.updateUserAsset(
      for: token,
      visible: visible,
      isSpam: isSpam,
      isDeletedByUser: isDeletedByUser
    ) { [weak self] in
      guard let self else { return }
      let selectedAccounts = self.filters.accounts.filter(\.isSelected).map(\.model)
      let selectedNetworks = self.filters.networks.filter(\.isSelected).map(\.model)

      let unionedSpamNFTs = computeSpamNFTs(
        selectedNetworks: selectedNetworks,
        selectedAccounts: selectedAccounts,
        simpleHashSpamNFTs: simpleHashSpamNFTs
      )
      
      (userNFTGroups, _) = buildNFTGroupModels(
        groupBy: filters.groupBy,
        spams: unionedSpamNFTs,
        selectedAccounts: selectedAccounts,
        selectedNetworks: selectedNetworks
      )
    }
  }
  
  func owner(for nft: BraveWallet.BlockchainToken) -> BraveWallet.AccountInfo? {
    guard let allBalances = nftBalancesCache[nft.id],
          let address = allBalances.first(where: { address, balance in
            balance > 0
          })?.key
    else { return nil }
    return allAccounts.first { $0.address.caseInsensitiveCompare(address) == .orderedSame }
  }
}

extension NFTStore: PreferencesObserver {
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

private extension Array where Element == NFTAssetViewModel {
  /// Optionally filters out NFTs not belonging to the given `selectedAccounts`.
  func optionallyFilterUnownedNFTs(
    isHidingUnownedNFTs: Bool,
    selectedAccounts: [BraveWallet.AccountInfo]
  ) -> [Element] {
    optionallyFilter(
      shouldFilter: isHidingUnownedNFTs,
      isIncluded: { nftAsset in
        let balancesForSelectedAccounts = nftAsset.balanceForAccounts.filter { balance in
          selectedAccounts.contains(where: { account in
            account.address == balance.key
          })
        }
        return balancesForSelectedAccounts.contains(where: { $0.value > 0 })
      })
  }
}
