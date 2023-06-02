// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Combine

public class AssetStore: ObservableObject, Equatable {
  @Published var token: BraveWallet.BlockchainToken
  @Published var isVisible: Bool {
    didSet {
      if isCustomToken {
        walletService.setUserAssetVisible(
          token,
          visible: isVisible
        ) { _ in }
      } else {
        if isVisible {
          walletService.addUserAsset(token) { _ in }
        } else {
          walletService.removeUserAsset(token) { _ in }
        }
      }
    }
  }
  var network: BraveWallet.NetworkInfo

  private let walletService: BraveWalletBraveWalletService
  private let rpcService: BraveWalletJsonRpcService
  private let ipfsApi: IpfsAPI
  private(set) var isCustomToken: Bool

  init(
    walletService: BraveWalletBraveWalletService,
    rpcService: BraveWalletJsonRpcService,
    network: BraveWallet.NetworkInfo,
    token: BraveWallet.BlockchainToken,
    ipfsApi: IpfsAPI,
    isCustomToken: Bool,
    isVisible: Bool
  ) {
    self.walletService = walletService
    self.rpcService = rpcService
    self.network = network
    self.token = token
    self.ipfsApi = ipfsApi
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

  private let walletService: BraveWalletBraveWalletService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let rpcService: BraveWalletJsonRpcService
  private let keyringService: BraveWalletKeyringService
  private let assetRatioService: BraveWalletAssetRatioService
  private let ipfsApi: IpfsAPI
  private var allTokens: [BraveWallet.BlockchainToken] = []
  private var timer: Timer?

  public init(
    walletService: BraveWalletBraveWalletService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    rpcService: BraveWalletJsonRpcService,
    keyringService: BraveWalletKeyringService,
    assetRatioService: BraveWalletAssetRatioService,
    ipfsApi: IpfsAPI
  ) {
    self.walletService = walletService
    self.blockchainRegistry = blockchainRegistry
    self.rpcService = rpcService
    self.keyringService = keyringService
    self.assetRatioService = assetRatioService
    self.ipfsApi = ipfsApi
    self.keyringService.add(self)
  }
  
  @Published var networkFilter: NetworkFilter = .allNetworks {
    didSet {
      update()
    }
  }
  
  func update() {
    Task { @MainActor in
      let networks: [BraveWallet.NetworkInfo]
      switch networkFilter {
      case .allNetworks:
        networks = await self.rpcService.allNetworksForSupportedCoins()
      case let .network(network):
        networks = [network]
      }
      let allUserAssets = await self.walletService.allUserAssets(in: networks)
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
            walletService: walletService,
            rpcService: rpcService,
            network: assetsForNetwork.network,
            token: token,
            ipfsApi: self.ipfsApi,
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
    walletService.addUserAsset(asset) { [weak self] success in
      if success {
        self?.update()
      }
      completion(success)
    }
  }

  func removeUserAsset(token: BraveWallet.BlockchainToken, completion: @escaping (_ success: Bool) -> Void) {
    walletService.removeUserAsset(token) { [weak self] success in
      if success {
        self?.update()
      }
      completion(success)
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
    let allUserAssets = await walletService.allUserAssets(in: allNetworks)
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
    let allUserAssets = await walletService.allUserAssets(in: allNetworks)
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
  public func keyringCreated(_ keyringId: String) {
    update()
  }
  
  public func keyringRestored(_ keyringId: String) {
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
