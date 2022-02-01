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
  
  private let walletService: BraveWalletBraveWalletService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let rpcService: BraveWalletJsonRpcService
  
  public init(
    walletService: BraveWalletBraveWalletService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    rpcService: BraveWalletJsonRpcService
  ) {
    self.walletService = walletService
    self.blockchainRegistry = blockchainRegistry
    self.rpcService = rpcService
    self.rpcService.add(self)
    
    fetchVisibleAssets()
  }
  
  private func updateSelectedAssets(_ network: BraveWallet.EthereumChain) {
    walletService.userAssets(network.chainId) { [self] userAssets in
      let visibleAssetIds = userAssets.filter(\.visible).map(\.id)
      let isTestnet = network.chainId != BraveWallet.MainnetChainId
      blockchainRegistry.allTokens(BraveWallet.MainnetChainId) { registryTokens in
        let allTokens = (isTestnet ? [] : registryTokens) + [network.nativeToken]
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
