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
  private(set) var isCustomToken: Bool

  init(
    walletService: BraveWalletBraveWalletService,
    network: BraveWallet.NetworkInfo,
    token: BraveWallet.BlockchainToken,
    isCustomToken: Bool,
    isVisible: Bool
  ) {
    self.walletService = walletService
    self.network = network
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

  private func updateSelectedAssets(_ network: BraveWallet.NetworkInfo) {
    walletService.userAssets(network.chainId, coin: network.coin) { [self] userAssets in
      let visibleAssetIds = userAssets.filter(\.visible).map(\.id)
      blockchainRegistry.allTokens(network.chainId, coin: network.coin) { [self] registryTokens in
        allTokens = registryTokens + [network.nativeToken]
        assetStores = allTokens.union(userAssets, f: { $0.id }).map { token in
          AssetStore(
            walletService: walletService,
            network: network,
            token: token,
            isCustomToken: !allTokens.contains(where: {
              $0.contractAddress(in: network).caseInsensitiveCompare(token.contractAddress) == .orderedSame
            }),
            isVisible: visibleAssetIds.contains(token.id)
          )
        }
      }
    }
  }

  func addUserAsset(
    address: String,
    name: String,
    symbol: String,
    decimals: Int,
    completion: @escaping (_ success: Bool) -> Void
  ) {
    walletService.selectedCoin { [weak self] coinType in
      guard let self = self else { return }
      // TODO: Add network selection picker to `AddCustomAssetView`: Issue #5725
      self.rpcService.network(coinType) { network in
        let token = BraveWallet.BlockchainToken(
          contractAddress: address,
          name: name,
          logo: "",
          isErc20: coinType == .eth,
          isErc721: false,
          symbol: symbol,
          decimals: Int32(decimals),
          visible: true,
          tokenId: "",
          coingeckoId: "",
          chainId: network.chainId,
          coin: network.coin
        )
        self.walletService.addUserAsset(token) { success in
          if success {
            self.updateSelectedAssets(network)
          }
          completion(success)
        }
      }
    }
  }

  func removeUserAsset(token: BraveWallet.BlockchainToken, completion: @escaping (_ success: Bool) -> Void) {
    walletService.selectedCoin { [weak self] coin in
      guard let self = self else { return }
      self.rpcService.network(coin) { network in
        self.walletService.removeUserAsset(token) { success in
          if success {
            self.updateSelectedAssets(network)
          }
          completion(success)
        }
      }
    }
  }

  func fetchVisibleAssets() {
    walletService.selectedCoin { [weak self] coin in
      self?.rpcService.network(coin) { network in
        self?.updateSelectedAssets(network)
      }
    }
  }

  func tokenInfo(by contractAddress: String, completion: @escaping (BraveWallet.BlockchainToken?) -> Void) {
    if let assetStore = assetStores.first(where: { $0.token.contractAddress.lowercased() == contractAddress.lowercased() }) {  // First check user's visible assets
      completion(assetStore.token)
    } else if let token = allTokens.first(where: { $0.contractAddress.lowercased() == contractAddress.lowercased() }) {  // Then check full tokens list
      completion(token)
    } else {  // Last network call to get token info by its contractAddress only if the network is Mainnet
      timer?.invalidate()
      timer = Timer.scheduledTimer(
        withTimeInterval: 0.25, repeats: false,
        block: { [weak self] _ in
          guard let self = self else { return }
          self.rpcService.network(.eth) { network in
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
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType) {
    rpcService.network(coin) { [weak self] network in
      self?.updateSelectedAssets(network)
    }
  }
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
