// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

public class AssetStore: ObservableObject, Equatable {
  @Published var token: BraveWallet.BlockchainToken
  @Published var isVisible: Bool {
    didSet {
      if isCustomToken {
        walletService.setUserAssetVisible(
          token,
          chainId: chainId,
          visible: isVisible
        ) { _ in }
      } else {
        if isVisible {
          walletService.addUserAsset(token, chainId: chainId) { _ in }
        } else {
          walletService.removeUserAsset(token, chainId: chainId) { _ in }
        }
      }
    }
  }

  private let walletService: BraveWalletBraveWalletService
  private var chainId: String
  private(set) var isCustomToken: Bool
  
  init(
    walletService: BraveWalletBraveWalletService,
    chainId: String,
    token: BraveWallet.BlockchainToken,
    isCustomToken: Bool,
    isVisible: Bool
  ) {
    self.walletService = walletService
    self.chainId = chainId
    self.token = token
    self.isCustomToken = isCustomToken
    self.isVisible = isVisible
  }
  
  public static func == (lhs: AssetStore, rhs: AssetStore) -> Bool {
    lhs.token == rhs.token && lhs.isVisible == rhs.isVisible
  }
}

public class UserAssetsStore: ObservableObject {
  @Published private(set) var assetStores: [AssetStore] = []
  @Published var isSearchingToken: Bool = false
  
  private let walletService: BraveWalletBraveWalletService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let rpcService: BraveWalletJsonRpcService
  private let assetRatioService: BraveWalletAssetRatioService
  private var allTokens: [BraveWallet.BlockchainToken] = []
  private var timer: Timer?
  
  public init(
    walletService: BraveWalletBraveWalletService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    rpcService: BraveWalletJsonRpcService,
    assetRatioService: BraveWalletAssetRatioService
  ) {
    self.walletService = walletService
    self.blockchainRegistry = blockchainRegistry
    self.rpcService = rpcService
    self.assetRatioService = assetRatioService
    self.rpcService.add(self)
    
    fetchVisibleAssets()
  }
  
  private func updateSelectedAssets(_ network: BraveWallet.EthereumChain) {
    walletService.userAssets(network.chainId) { [self] userAssets in
      let visibleAssetIds = userAssets.filter(\.visible).map(\.id)
      let isTestnet = network.chainId != BraveWallet.MainnetChainId
      blockchainRegistry.allTokens(BraveWallet.MainnetChainId) { registryTokens in
        allTokens = (isTestnet ? [] : registryTokens) + [network.nativeToken]
        assetStores = allTokens.union(userAssets, f: { $0.id }).map { token in
          AssetStore(
            walletService: walletService,
            chainId: network.chainId,
            token: token,
            isCustomToken: !allTokens.contains(where: { $0.contractAddress == token.contractAddress }),
            isVisible: visibleAssetIds.contains(token.id)
          )
        }
      }
    }
  }
  
  func addUserAsset(token: BraveWallet.BlockchainToken, completion: @escaping (_ success: Bool) -> Void) {
    rpcService.network { [weak self] network in
      guard let self = self else { return }
      self.walletService.addUserAsset(token, chainId: network.chainId) { success in
        if success {
          self.updateSelectedAssets(network)
        }
        completion(success)
      }
    }
  }
  
  func removeUserAsset(token: BraveWallet.BlockchainToken, completion: @escaping (_ success: Bool) -> Void) {
    rpcService.network { [weak self] network in
      guard let self = self else { return }
      self.walletService.removeUserAsset(token, chainId: network.chainId) { success in
        if success {
          self.updateSelectedAssets(network)
        }
        completion(success)
      }
    }
  }
  
  func fetchVisibleAssets() {
    rpcService.network { [weak self] network in
      self?.updateSelectedAssets(network)
    }
  }
  
  func tokenInfo(by contractAddress: String, completion: @escaping (BraveWallet.BlockchainToken?) -> Void) {
    if let assetStore = assetStores.first(where: { $0.token.contractAddress.lowercased() == contractAddress.lowercased() }) { // First check user's visible assets
      completion(assetStore.token)
    } else if let token = allTokens.first(where: { $0.contractAddress.lowercased() == contractAddress.lowercased() }) { // Then check full tokens list
      completion(token)
    } else { // Last network call to get token info by its contractAddress only if the network is Mainnet
      timer?.invalidate()
      timer = Timer.scheduledTimer(withTimeInterval: 0.25, repeats: false, block: { [weak self] _ in
        guard let self = self else { return }
        self.rpcService.network { network in
          if network.id == BraveWallet.MainnetChainId {
            self.isSearchingToken = true
            self.assetRatioService.tokenInfo(contractAddress) { token in
              self.isSearchingToken = false
              completion(token)
            }
          } else {
            completion(nil)
          }
        }
      })
    }
  }
}

extension UserAssetsStore: BraveWalletJsonRpcServiceObserver {
  public func chainChangedEvent(_ chainId: String) {
    rpcService.network { [weak self] network in
      self?.updateSelectedAssets(network)
    }
  }
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
