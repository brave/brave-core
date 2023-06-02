// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

struct NFTAssetViewModel: Identifiable, Equatable {
  var token: BraveWallet.BlockchainToken
  var network: BraveWallet.NetworkInfo
  var balance: Int
  var nftMetadata: NFTMetadata?
  
  public var id: String {
    token.id + network.chainId
  }
  
  static func == (lhs: NFTAssetViewModel, rhs: NFTAssetViewModel) -> Bool {
    lhs.id == rhs.id
  }
}

public class NFTStore: ObservableObject {
  /// The users visible NFTs
  @Published private(set) var userVisibleNFTs: [NFTAssetViewModel] = []
  /// The network filter in NFT tab
  @Published var networkFilter: NetworkFilter = .allNetworks {
    didSet {
      update()
    }
  }
  @Published var isLoadingDiscoverAssets: Bool = false
  
  public private(set) lazy var userAssetsStore: UserAssetsStore = .init(
    walletService: self.walletService,
    blockchainRegistry: self.blockchainRegistry,
    rpcService: self.rpcService,
    keyringService: self.keyringService,
    assetRatioService: self.assetRatioService,
    ipfsApi: self.ipfsApi
  )
  
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let ipfsApi: IpfsAPI
  
  /// Cancellable for the last running `update()` Task.
  private var updateTask: Task<(), Never>?
  /// Cache of metadata for NFTs. The key is the token's `id`.
  private var metadataCache: [String: NFTMetadata] = [:]
  
  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    ipfsApi: IpfsAPI
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry
    self.ipfsApi = ipfsApi
    
    self.rpcService.add(self)
    self.keyringService.add(self)
    self.walletService.add(self)
    
    keyringService.isLocked { [self] isLocked in
      if !isLocked {
        update()
      }
    }
  }
  
  func update() {
    self.updateTask?.cancel()
    self.updateTask = Task { @MainActor in
      let networks: [BraveWallet.NetworkInfo]
      switch networkFilter {
      case .allNetworks:
        networks = await self.rpcService.allNetworksForSupportedCoins()
          .filter { !WalletConstants.supportedTestNetworkChainIds.contains($0.chainId) }
      case let .network(network):
        networks = [network]
      }
      let allVisibleUserAssets = await self.walletService.allVisibleUserAssets(in: networks)
      var updatedUserVisibleNFTs: [NFTAssetViewModel] = []
      for networkAssets in allVisibleUserAssets {
        for token in networkAssets.tokens {
          if token.isErc721 || token.isNft {
            updatedUserVisibleNFTs.append(
              NFTAssetViewModel(
                token: token,
                network: networkAssets.network,
                balance: 0, // no balance shown in NFT tab
                nftMetadata: metadataCache[token.id]
              )
            )
          }
        }
      }
      self.userVisibleNFTs = updatedUserVisibleNFTs
      
      // fetch nft metadata for all NFTs
      // fetch price for every token
      let allTokens = allVisibleUserAssets.flatMap(\.tokens)
      let allNFTs = allTokens.filter { $0.isNft || $0.isErc721 }
      let allMetadata = await rpcService.fetchNFTMetadata(tokens: allNFTs, ipfsApi: ipfsApi)
      for (key, value) in allMetadata { // update cached values
        metadataCache[key] = value
      }
      
      guard !Task.isCancelled else { return }
      updatedUserVisibleNFTs.removeAll()
      for networkAssets in allVisibleUserAssets {
        for token in networkAssets.tokens {
          if token.isErc721 || token.isNft {
            updatedUserVisibleNFTs.append(
              NFTAssetViewModel(
                token: token,
                network: networkAssets.network,
                balance: 0, // no balance shown in NFT tab
                nftMetadata: metadataCache[token.id]
              )
            )
          }
        }
      }
      self.userVisibleNFTs = updatedUserVisibleNFTs
    }
  }
  
  func updateNFTMetadataCache(for token: BraveWallet.BlockchainToken, metadata: NFTMetadata) {
    metadataCache[token.id] = metadata
    if let index = userVisibleNFTs.firstIndex(where: { $0.token.id == token.id }), let viewModel = userVisibleNFTs[safe: index] {
      userVisibleNFTs[index] = NFTAssetViewModel(token: viewModel.token, network: viewModel.network, balance: viewModel.balance, nftMetadata: metadata)
    }
  }
  
  @MainActor func isNFTDiscoveryEnabled() async -> Bool {
    await walletService.nftDiscoveryEnabled()
  }
  
  func enableNFTDiscovery() {
    walletService.setNftDiscoveryEnabled(true)
  }
}

extension NFTStore: BraveWalletJsonRpcServiceObserver {
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
  
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType, origin: URLOrigin?) {
    update()
  }
}

extension NFTStore: BraveWalletKeyringServiceObserver {
  public func keyringReset() {
  }
  
  public func accountsChanged() {
    update()
  }
  public func backedUp() {
  }
  public func keyringCreated(_ keyringId: String) {
  }
  public func keyringRestored(_ keyringId: String) {
  }
  public func locked() {
  }
  public func unlocked() {
    DispatchQueue.main.async { [self] in
      update()
    }
  }
  public func autoLockMinutesChanged() {
  }
  public func selectedAccountChanged(_ coinType: BraveWallet.CoinType) {
    DispatchQueue.main.async { [self] in
      update()
    }
  }
  
  public func accountsAdded(_ addedAccounts: [BraveWallet.AccountInfo]) {
  }
}

extension NFTStore: BraveWalletBraveWalletServiceObserver {
  public func onActiveOriginChanged(_ originInfo: BraveWallet.OriginInfo) {
  }
  
  public func onDefaultEthereumWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }
  
  public func onDefaultSolanaWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }
  
  public func onDefaultBaseCurrencyChanged(_ currency: String) {
  }
  
  public func onDefaultBaseCryptocurrencyChanged(_ cryptocurrency: String) {
  }
  
  public func onNetworkListChanged() {
  }
  
  public func onDiscoverAssetsStarted() {
    isLoadingDiscoverAssets = true
  }
  
  public func onDiscoverAssetsCompleted(_ discoveredAssets: [BraveWallet.BlockchainToken]) {
    isLoadingDiscoverAssets = false
    if !discoveredAssets.isEmpty {
      update()
    }
  }
  
  public func onResetWallet() {
  }
}
