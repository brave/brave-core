// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Data
import BraveCore
import Preferences

public protocol WalletUserAssetManagerType: AnyObject {
  func getAllVisibleAssetsInNetworkAssets(networks: [BraveWallet.NetworkInfo]) -> [NetworkAssets]
  func getAllUserAssetsInNetworkAssets(networks: [BraveWallet.NetworkInfo]) -> [NetworkAssets]
  func getUserAsset(_ asset: BraveWallet.BlockchainToken) -> WalletUserAsset?
  func addUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?)
  func removeUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?)
  func removeGroup(for groupId: String, completion: (() -> Void)?)
  func updateUserAsset(for asset: BraveWallet.BlockchainToken, visible: Bool, completion: (() -> Void)?)
}

public class WalletUserAssetManager: WalletUserAssetManagerType {
  
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  
  public init(
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService
  ) {
    self.rpcService = rpcService
    self.walletService = walletService
  }
  
  public func getAllUserAssetsInNetworkAssets(networks: [BraveWallet.NetworkInfo]) -> [NetworkAssets] {
    var allVisibleUserAssets: [NetworkAssets] = []
    for (index, network) in networks.enumerated() {
      let groupId = network.walletUserAssetGroupId
      if let walletUserAssets = WalletUserAssetGroup.getGroup(groupId: groupId)?.walletUserAssets {
        let networkAsset = NetworkAssets(
          network: network,
          tokens: walletUserAssets.map({ $0.blockchainToken }),
          sortOrder: index
        )
        allVisibleUserAssets.append(networkAsset)
      }
    }
    return allVisibleUserAssets.sorted(by: { $0.sortOrder < $1.sortOrder })
  }
  
  public func getAllVisibleAssetsInNetworkAssets(networks: [BraveWallet.NetworkInfo]) -> [NetworkAssets] {
    var allVisibleUserAssets: [NetworkAssets] = []
    for (index, network) in networks.enumerated() {
      let groupId = network.walletUserAssetGroupId
      if let walletUserAssets = WalletUserAssetGroup.getGroup(groupId: groupId)?.walletUserAssets?.filter(\.visible) {
        let networkAsset = NetworkAssets(
          network: network,
          tokens: walletUserAssets.map({ $0.blockchainToken }),
          sortOrder: index
        )
        allVisibleUserAssets.append(networkAsset)
      }
    }
    return allVisibleUserAssets.sorted(by: { $0.sortOrder < $1.sortOrder })
  }
  
  public func getUserAsset(_ asset: BraveWallet.BlockchainToken) -> WalletUserAsset? {
    WalletUserAsset.getUserAsset(asset: asset)
  }
  
  public func addUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
    guard WalletUserAsset.getUserAsset(asset: asset) == nil else {
      completion?()
      return
    }
    WalletUserAsset.addUserAsset(asset: asset, completion: completion)
  }
  
  public func removeUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
    WalletUserAsset.removeUserAsset(asset: asset, completion: completion)
  }
  
  public func updateUserAsset(for asset: BraveWallet.BlockchainToken, visible: Bool, completion: (() -> Void)?) {
    WalletUserAsset.updateUserAsset(for: asset, visible: visible, completion: completion)
  }
  
  public func removeGroup(for groupId: String, completion: (() -> Void)?) {
    WalletUserAssetGroup.removeGroup(groupId, completion: completion)
  }
  
  public func migrateUserAssets(for coin: BraveWallet.CoinType? = nil, completion: (() -> Void)? = nil) {
    guard !Preferences.Wallet.migrateCoreToWalletUserAssetCompleted.value else {
      completion?()
      return
    }
    Task { @MainActor in
      var fetchedUserAssets: [String: [BraveWallet.BlockchainToken]] = [:]
      var networks: [BraveWallet.NetworkInfo] = []
      if let coin = coin {
        networks = await rpcService.allNetworks(coin)
      } else {
        networks = await rpcService.allNetworksForSupportedCoins(respectTestnetPreference: false)
      }
      let networkAssets = await walletService.allUserAssets(in: networks)
      for networkAsset in networkAssets {
        fetchedUserAssets["\(networkAsset.network.coin.rawValue).\(networkAsset.network.chainId)"] = networkAsset.tokens
      }
      WalletUserAsset.migrateVisibleAssets(fetchedUserAssets) {
        Preferences.Wallet.migrateCoreToWalletUserAssetCompleted.value = true
        completion?()
      }
    }
  }
}

#if DEBUG
public class TestableWalletUserAssetManager: WalletUserAssetManagerType {
  public var _getAllVisibleAssetsInNetworkAssets: ((_ networks: [BraveWallet.NetworkInfo]) -> [NetworkAssets])?
  public var _getAllUserAssetsInNetworkAssets: ((_ networks: [BraveWallet.NetworkInfo]) -> [NetworkAssets])?
  
  public init() {}
  
  public func getAllUserAssetsInNetworkAssets(networks: [BraveWallet.NetworkInfo]) -> [NetworkAssets] {
    let defaultAssets: [NetworkAssets] = [
      NetworkAssets(network: .mockMainnet, tokens: [.previewToken], sortOrder: 0),
      NetworkAssets(network: .mockGoerli, tokens: [.previewToken], sortOrder: 1)
    ]
    let chainIds = networks.map { $0.chainId }
    return _getAllUserAssetsInNetworkAssets?(networks) ?? defaultAssets.filter({
      chainIds.contains($0.network.chainId)
    })
  }
  
  public func getAllVisibleAssetsInNetworkAssets(networks: [BraveWallet.NetworkInfo]) -> [NetworkAssets] {
    _getAllVisibleAssetsInNetworkAssets?(networks) ?? []
  }
  
  public func getUserAsset(_ asset: BraveWallet.BlockchainToken) -> WalletUserAsset? {
    return nil
  }
  
  public func addUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
  }
  
  public func removeUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
  }

  public func removeGroup(for groupId: String, completion: (() -> Void)?) {
  }
  
  public func updateUserAsset(for asset: BraveWallet.BlockchainToken, visible: Bool, completion: (() -> Void)?) {
  }
}
#endif
