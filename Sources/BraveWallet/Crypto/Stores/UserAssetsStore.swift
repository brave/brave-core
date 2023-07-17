// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Combine
import Data

public class AssetStore: ObservableObject, Equatable {
  @Published var token: BraveWallet.BlockchainToken
  @Published var isVisible: Bool {
    didSet {
      assetManager.updateUserAsset(for: token, visible: isVisible, completion: nil)
    }
  }
  var network: BraveWallet.NetworkInfo

  private let rpcService: BraveWalletJsonRpcService
  private let ipfsApi: IpfsAPI
  private let assetManager: WalletUserAssetManagerType
  private(set) var isCustomToken: Bool

  init(
    rpcService: BraveWalletJsonRpcService,
    network: BraveWallet.NetworkInfo,
    token: BraveWallet.BlockchainToken,
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType,
    isCustomToken: Bool,
    isVisible: Bool
  ) {
    self.rpcService = rpcService
    self.network = network
    self.token = token
    self.ipfsApi = ipfsApi
    self.assetManager = userAssetManager
    self.isCustomToken = isCustomToken
    self.isVisible = isVisible
  }

  public static func == (lhs: AssetStore, rhs: AssetStore) -> Bool {
    lhs.token == rhs.token && lhs.isVisible == rhs.isVisible
  }
  
  @MainActor func fetchERC721Metadata() async -> NFTMetadata? {
    return await rpcService.fetchNFTMetadata(for: token, ipfsApi: self.ipfsApi)
  }
}

public class UserAssetsStore: ObservableObject {
  @Published private(set) var assetStores: [AssetStore] = []
  @Published var isSearchingToken: Bool = false
  @Published var networkFilters: [Selectable<BraveWallet.NetworkInfo>] = [] {
    didSet {
      guard !oldValue.isEmpty else { return } // initial assignment to `networkFilters`
      update()
    }
  }

  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let rpcService: BraveWalletJsonRpcService
  private let keyringService: BraveWalletKeyringService
  private let assetRatioService: BraveWalletAssetRatioService
  private let ipfsApi: IpfsAPI
  private let assetManager: WalletUserAssetManagerType
  private var allTokens: [BraveWallet.BlockchainToken] = []
  private var timer: Timer?

  public init(
    blockchainRegistry: BraveWalletBlockchainRegistry,
    rpcService: BraveWalletJsonRpcService,
    keyringService: BraveWalletKeyringService,
    assetRatioService: BraveWalletAssetRatioService,
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.blockchainRegistry = blockchainRegistry
    self.rpcService = rpcService
    self.keyringService = keyringService
    self.assetRatioService = assetRatioService
    self.ipfsApi = ipfsApi
    self.assetManager = userAssetManager
    self.keyringService.add(self)
  }
  
  func update() {
    Task { @MainActor in
      // setup network filters if not currently setup
      if self.networkFilters.isEmpty {
        self.networkFilters = await self.rpcService.allNetworksForSupportedCoins().map {
          .init(isSelected: true, model: $0)
        }
      }
      let networks: [BraveWallet.NetworkInfo] = self.networkFilters.filter(\.isSelected).map(\.model)
      let allUserAssets = assetManager.getAllUserAssetsInNetworkAssets(networks: networks)
      var allTokens = await self.blockchainRegistry.allTokens(in: networks)
      // Filter `allTokens` to remove any tokens existing in `allUserAssets`. This is possible for ERC721 tokens in the registry without a `tokenId`, which requires the user to add as a custom token
      let allUserTokens = allUserAssets.flatMap(\.tokens)
      allTokens = allTokens.map { assetsForNetwork in
        NetworkAssets(
          network: assetsForNetwork.network,
          tokens: assetsForNetwork.tokens.filter { token in
            !allUserTokens.contains(where: { $0.id == token.id })
          },
          sortOrder: assetsForNetwork.sortOrder)
      }
      
      let visibleIds = allUserAssets.flatMap(\.tokens).filter(\.visible).map { $0.id + $0.chainId }
      assetStores = (allUserAssets + allTokens).flatMap { assetsForNetwork in
        assetsForNetwork.tokens.map { token in
          var isCustomToken: Bool {
            if token.contractAddress.isEmpty {
              return false
            }
            // Any token with a tokenId should be considered a custom token.
            if !token.tokenId.isEmpty {
              return true
            }
            return !allTokens.flatMap(\.tokens).contains(where: {
              $0.contractAddress(in: assetsForNetwork.network).caseInsensitiveCompare(token.contractAddress) == .orderedSame
            })
          }
          return AssetStore(
            rpcService: rpcService,
            network: assetsForNetwork.network,
            token: token,
            ipfsApi: self.ipfsApi,
            userAssetManager: assetManager,
            isCustomToken: isCustomToken,
            isVisible: visibleIds.contains(where: { $0 == (token.id + token.chainId) })
          )
        }
      }
    }
  }

  func addUserAsset(
    _ asset: BraveWallet.BlockchainToken,
    completion: @escaping (_ success: Bool) -> Void
  ) {
    if assetManager.getUserAsset(asset) != nil {
      completion(false)
    } else {
      assetManager.addUserAsset(asset) { [weak self] in
        self?.update()
        completion(true)
      }
    }
  }

  func removeUserAsset(token: BraveWallet.BlockchainToken, completion: @escaping (_ success: Bool) -> Void) {
    assetManager.removeUserAsset(token) { [weak self] in
      self?.update()
      completion(true)
    }
  }

  func tokenInfo(
    address: String,
    completion: @escaping (BraveWallet.BlockchainToken?) -> Void
  ) {
    // First check user's visible assets
    if let assetStore = assetStores.first(where: { $0.token.contractAddress.caseInsensitiveCompare(address) == .orderedSame }) {
      completion(assetStore.token)
    } // else check full tokens list
    else if let token = allTokens.first(where: { $0.contractAddress.caseInsensitiveCompare(address) == .orderedSame }) {
      completion(token)
    } // else use network request to get token info
    else if address.isETHAddress { // only Eth Mainnet supported, require ethereum address
      timer?.invalidate()
      timer = Timer.scheduledTimer(
        withTimeInterval: 0.25, repeats: false,
        block: { [weak self] _ in
          guard let self = self else { return }
          self.isSearchingToken = true
          self.assetRatioService.tokenInfo(address) { token in
            self.isSearchingToken = false
            completion(token)
          }
        })
    }
  }
  
  @MainActor func networkInfo(by chainId: String, coin: BraveWallet.CoinType) async -> BraveWallet.NetworkInfo? {
    let allNetworks = await rpcService.allNetworks(coin)
    return allNetworks.first { $0.chainId.caseInsensitiveCompare(chainId) == .orderedSame }
  }
  
  @MainActor func allAssets() async -> [AssetViewModel] {
    let allNetworks = await rpcService.allNetworksForSupportedCoins()
    let allUserAssets = assetManager.getAllUserAssetsInNetworkAssets(networks: allNetworks)
    // Filter `allTokens` to remove any tokens existing in `allUserAssets`. This is possible for ERC721 tokens in the registry without a `tokenId`, which requires the user to add as a custom token
    let allUserTokens = allUserAssets.flatMap(\.tokens)
    let allBlockchainTokens = await blockchainRegistry.allTokens(in: allNetworks)
      .map { assetsForNetwork in
        NetworkAssets(
          network: assetsForNetwork.network,
          tokens: assetsForNetwork.tokens.filter { token in
            !allUserTokens.contains(where: { $0.id == token.id })
          },
          sortOrder: assetsForNetwork.sortOrder)
      }
    
    return (allUserAssets + allBlockchainTokens).flatMap { networkAssets in
      networkAssets.tokens.map { token in
        AssetViewModel(
          token: token,
          network: networkAssets.network,
          decimalBalance: 0,
          price: "",
          history: []
        )
      }
    }
  }
  
  @MainActor func allNFTMetadata() async -> [String: NFTMetadata] {
    let allNetworks = await rpcService.allNetworksForSupportedCoins()
    let allUserAssets = assetManager.getAllUserAssetsInNetworkAssets(networks: allNetworks)
    // Filter `allTokens` to remove any tokens existing in `allUserAssets`. This is possible for ERC721 tokens in the registry without a `tokenId`, which requires the user to add as a custom token
    let allUserTokens = allUserAssets.flatMap(\.tokens)
    
    // NFT metadata only exists for custom NFT added by users. ERC721 tokens from token registry do not have metadata
    return await rpcService.fetchNFTMetadata(
      tokens: allUserTokens.filter { $0.isErc721 || $0.isNft },
      ipfsApi: ipfsApi
    )
  }
}

extension UserAssetsStore: BraveWalletKeyringServiceObserver {
  public func keyringCreated(_ keyringId: BraveWallet.KeyringId) {
    update()
  }
  
  public func keyringRestored(_ keyringId: BraveWallet.KeyringId) {
  }
  
  public func keyringReset() {
  }
  
  public func locked() {
  }
  
  public func unlocked() {
  }
  
  public func backedUp() {
  }
  
  public func accountsChanged() {
  }
  
  public func autoLockMinutesChanged() {
  }
  
  public func selectedAccountChanged(_ coin: BraveWallet.CoinType) {
  }
  
  public func accountsAdded(_ addedAccounts: [BraveWallet.AccountInfo]) {
  }
}

extension UserAssetsStore: BraveWalletBraveWalletServiceObserver {
  public func onActiveOriginChanged(_ originInfo: BraveWallet.OriginInfo) { }
  
  public func onDefaultEthereumWalletChanged(_ wallet: BraveWallet.DefaultWallet) { }
  
  public func onDefaultSolanaWalletChanged(_ wallet: BraveWallet.DefaultWallet) { }
  
  public func onDefaultBaseCurrencyChanged(_ currency: String) { }
  
  public func onDefaultBaseCryptocurrencyChanged(_ cryptocurrency: String) { }
  
  public func onNetworkListChanged() {
    Task { @MainActor in
      // A network was added or removed, update our network filters for the change.
      self.networkFilters = await self.rpcService.allNetworksForSupportedCoins().map { network in
        let existingSelectionValue = self.networkFilters.first(where: { $0.model.chainId == network.chainId})?.isSelected
        return .init(isSelected: existingSelectionValue ?? true, model: network)
      }
    }
  }
  
  public func onDiscoverAssetsStarted() { }
  
  public func onDiscoverAssetsCompleted(_ discoveredAssets: [BraveWallet.BlockchainToken]) { }
  
  public func onResetWallet() { }
}
