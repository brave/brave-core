// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Preferences

struct NFTAssetViewModel: Identifiable, Equatable {
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
  /// The NFTs grouped by enum `NFTDisplayType` displayed in `NFTView`
  var displayNFTs: [NFTAssetViewModel] {
    switch displayType {
    case .visible:
      return userNFTs.filter(\.token.visible)
    case .hidden:
      return userNFTs.filter { !$0.token.visible && !$0.token.isSpam }
    case .spam:
      return userNFTs.filter(\.token.isSpam)
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
    case spam
    
    var id: Int {
      rawValue
    }
    
    var dropdownTitle: String {
      switch self {
      case .visible:
        return Strings.Wallet.nftsTitle
      case .hidden:
        return Strings.Wallet.nftHidden
      case .spam:
        return Strings.Wallet.nftSpam
      }
    }
    
    var emptyTitle: String {
      switch self {
      case .visible:
        return Strings.Wallet.nftPageEmptyTitle
      case .hidden:
        return Strings.Wallet.nftInvisiblePageEmptyTitle
      case .spam:
        return Strings.Wallet.nftSpamPageEmptyTitle
      }
    }
    
    var emptyDescription: String? {
      switch self {
      case .visible:
        return Strings.Wallet.nftPageEmptyDescription
      case .hidden, .spam:
        return nil
      }
    }
  }

  /// Current group to display
  @Published var displayType: NFTDisplayType = .visible
  /// View model for all NFT include visible, hidden and spams
  @Published private(set) var userNFTs: [NFTAssetViewModel] = []
  
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let ipfsApi: IpfsAPI
  private let assetManager: WalletUserAssetManagerType
  private var rpcServiceObserver: JsonRpcServiceObserver?
  private var keyringServiceObserver: KeyringServiceObserver?
  private var walletServiveObserber: WalletServiceObserver?
  
  /// Cancellable for the last running `update()` Task.
  private var updateTask: Task<(), Never>?
  /// Cache of metadata for NFTs. The key is the token's `id`.
  private var metadataCache: [String: NFTMetadata] = [:]
  /// Spam from SimpleHash in form of `NetworkAssets`
  private var simpleHashSpamNFTs: [NetworkAssets] = []
  
  var isObserving: Bool {
    rpcServiceObserver != nil && keyringServiceObserver != nil && walletServiveObserber != nil
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
    Preferences.Wallet.showTestNetworks.observe(from: self)
    Preferences.Wallet.isHidingUnownedNFTsFilter.observe(from: self)
    Preferences.Wallet.isShowingNFTNetworkLogoFilter.observe(from: self)
    Preferences.Wallet.nonSelectedNetworksFilter.observe(from: self)
  }
  
  func tearDown() {
    rpcServiceObserver = nil
    keyringServiceObserver = nil
    walletServiveObserber = nil
    
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
    self.walletServiveObserber = WalletServiceObserver(
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
    
    userAssetsStore.setupObservers()
  }
  
  /// Cache of NFT balances for each account tokenBalances: [token.contractAddress]
  private var nftBalancesCache: [String: [String: Int]] = [:]
  
  func update() {
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
      
      // user visible assets
      let userVisibleAssets = assetManager.getAllUserAssetsInNetworkAssetsByVisibility(networks: selectedNetworks, visible: true)
      // user hidden assets
      let userHiddenAssets = assetManager.getAllUserAssetsInNetworkAssetsByVisibility(networks: selectedNetworks, visible: false)
      // all spam NFTs marked by SimpleHash
      simpleHashSpamNFTs = await walletService.simpleHashSpamNFTs(for: selectedAccounts, on: selectedNetworks)
      let unionedSpamNFTs = computeSpamNFTs(
        selectedNetworks: selectedNetworks,
        selectedAccounts: selectedAccounts,
        simpleHashSpamNFTs: simpleHashSpamNFTs
      )
      
      let allNetworkAssets = userVisibleAssets + userHiddenAssets + unionedSpamNFTs
      userNFTs = buildAssetViewModels(allUserAssets: allNetworkAssets)
        .optionallyFilterUnownedNFTs(
          isHidingUnownedNFTs: filters.isHidingUnownedNFTs,
          selectedAccounts: selectedAccounts
        )
      
      var allTokens: [BraveWallet.BlockchainToken] = []
      for networkAssets in [userVisibleAssets, userHiddenAssets, unionedSpamNFTs] {
        allTokens.append(contentsOf: networkAssets.flatMap(\.tokens))
      }
      let allNFTs = allTokens.filter { $0.isNft || $0.isErc721 }
      // if we're not hiding unowned or grouping by account, balance isn't needed
      if filters.isHidingUnownedNFTs {
        // fetch balance for all NFTs
        let allAccounts = filters.accounts.map(\.model)
        nftBalancesCache = await withTaskGroup(
          of: [String: [String: Int]].self,
          body: { @MainActor [nftBalancesCache, rpcService] group in
            for nft in allNFTs { // for each NFT
              guard let networkForNFT = allNetworks.first(where: { $0.chainId == nft.chainId }) else {
                continue
              }
              group.addTask { @MainActor in
                let updatedBalances = await withTaskGroup(
                  of: [String: Int].self,
                  body: { @MainActor group in
                    for account in allAccounts where account.coin == nft.coin {
                      group.addTask { @MainActor in
                        let balanceForToken = await rpcService.balance(
                          for: nft,
                          in: account,
                          network: networkForNFT
                        )
                        return [account.address: Int(balanceForToken ?? 0)]
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
      userNFTs = buildAssetViewModels(allUserAssets: allNetworkAssets)
        .optionallyFilterUnownedNFTs(
          isHidingUnownedNFTs: filters.isHidingUnownedNFTs,
          selectedAccounts: selectedAccounts
        )
      
      // fetch nft metadata for all NFTs
      let allMetadata = await rpcService.fetchNFTMetadata(tokens: allNFTs, ipfsApi: ipfsApi)
      for (key, value) in allMetadata { // update cached values
        metadataCache[key] = value
      }
      guard !Task.isCancelled else { return }
      userNFTs = buildAssetViewModels(allUserAssets: allNetworkAssets)
        .optionallyFilterUnownedNFTs(
          isHidingUnownedNFTs: filters.isHidingUnownedNFTs,
          selectedAccounts: selectedAccounts
        )
    }
  }
  
  func updateNFTMetadataCache(for token: BraveWallet.BlockchainToken, metadata: NFTMetadata) {
    metadataCache[token.id] = metadata
    if let index = userNFTs.firstIndex(where: { $0.token.id == token.id }),
       var updatedViewModel = userNFTs[safe: index] {
      updatedViewModel.nftMetadata = metadata
      userNFTs[index] = updatedViewModel
    }
  }
  
  private func buildAssetViewModels(
    allUserAssets: [NetworkAssets]
  ) -> [NFTAssetViewModel] {
    allUserAssets.flatMap { networkAssets in
      networkAssets.tokens.compactMap { token in
        guard token.isErc721 || token.isNft else { return nil }
        return NFTAssetViewModel(
          token: token,
          network: networkAssets.network,
          balanceForAccounts: nftBalancesCache[token.id] ?? [:],
          nftMetadata: metadataCache[token.id]
        )
      }
    }
  }
  
  private func computeSpamNFTs(
    selectedNetworks: [BraveWallet.NetworkInfo],
    selectedAccounts: [BraveWallet.AccountInfo],
    simpleHashSpamNFTs: [NetworkAssets]
  ) -> [NetworkAssets] {
    // all spam NFTs marked by user
    let allUserMarkedSpamNFTs = assetManager.getAllUserNFTs(networks: selectedNetworks, isSpam: true)
    // filter out any spam NFTs from `simpleHashSpamNFTs` that are marked
    // not-spam by user
    var updatedSimpleHashSpamNFTs: [NetworkAssets] = []
    for simpleHashSpamNFTsOnNetwork in simpleHashSpamNFTs {
      let userMarkedNotSpamTokensOnNetwork = assetManager.getAllUserNFTs(networks: [simpleHashSpamNFTsOnNetwork.network], isSpam: false).flatMap(\.tokens)
      let filteredSimpleHashSpamTokens = simpleHashSpamNFTsOnNetwork.tokens.filter { simpleHashSpamToken in
        !userMarkedNotSpamTokensOnNetwork.contains { token in
          token.id == simpleHashSpamToken.id
        }
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
  
  @MainActor func isNFTDiscoveryEnabled() async -> Bool {
    await walletService.nftDiscoveryEnabled()
  }
  
  func enableNFTDiscovery() {
    walletService.setNftDiscoveryEnabled(true)
  }
  
  func updateNFTStatus(_ token: BraveWallet.BlockchainToken, visible: Bool, isSpam: Bool) {
    assetManager.updateUserAsset(for: token, visible: visible, isSpam: isSpam) { [weak self] in
      guard let self else { return }
      let selectedAccounts = self.filters.accounts.filter(\.isSelected).map(\.model)
      let selectedNetworks = self.filters.networks.filter(\.isSelected).map(\.model)
      let userVisibleAssets = self.assetManager.getAllUserAssetsInNetworkAssetsByVisibility(networks: selectedNetworks, visible: true)
      let userHiddenAssets = self.assetManager.getAllUserAssetsInNetworkAssetsByVisibility(networks: selectedNetworks, visible: false)
      let spamNFTs = computeSpamNFTs(
        selectedNetworks: selectedNetworks,
        selectedAccounts: selectedAccounts,
        simpleHashSpamNFTs: simpleHashSpamNFTs
      )
      let allNetworkAssets = userVisibleAssets + userHiddenAssets + spamNFTs
      userNFTs = buildAssetViewModels(allUserAssets: allNetworkAssets)
        .optionallyFilterUnownedNFTs(
          isHidingUnownedNFTs: filters.isHidingUnownedNFTs,
          selectedAccounts: selectedAccounts
        )
    }
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
