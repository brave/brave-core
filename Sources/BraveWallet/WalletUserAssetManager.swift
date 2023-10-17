// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Data
import BraveCore
import Preferences
import CoreData

public protocol WalletUserAssetManagerType: AnyObject {
  /// Return all visible or all invisible user assets in form of `NetworkAssets`
  func getAllUserAssetsInNetworkAssetsByVisibility(networks: [BraveWallet.NetworkInfo], visible: Bool) -> [NetworkAssets]
  /// Return all user assets in form of `NetworkAssets`
  func getAllUserAssetsInNetworkAssets(networks: [BraveWallet.NetworkInfo], includingUserDeleted: Bool) -> [NetworkAssets]
  /// Return all spam or non-spam user assets in form of `NetworkAssets`
  func getAllUserNFTs(networks: [BraveWallet.NetworkInfo], isSpam: Bool) -> [NetworkAssets]
  /// Return all user marked deleted user assets
  func getAllUserDeletedNFTs() -> [WalletUserAsset]
  /// Return a `WalletUserAsset` with a given `BraveWallet.BlockchainToken`
  func getUserAsset(_ asset: BraveWallet.BlockchainToken) -> WalletUserAsset?
  /// Add a `WalletUserAsset` representation of the given
  /// `BraveWallet.BlockchainToken` to CoreData
  func addUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?)
  /// Remove a `WalletUserAsset` representation of the given
  /// `BraveWallet.BlockchainToken` from CoreData
  func removeUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?)
  /// Remove an entire `WalletUserAssetGroup` with a given `groupId`
  func removeGroup(for groupId: String, completion: (() -> Void)?)
  /// Update a `WalletUserAsset`'s `visible`, `isSpam`, and `isDeletedByUser` status
  func updateUserAsset(for asset: BraveWallet.BlockchainToken, visible: Bool, isSpam: Bool, isDeletedByUser: Bool, completion: (() -> Void)?)
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
  
  /// Return all user's assets stored in CoreData
  public func getAllUserAssetsInNetworkAssets(networks: [BraveWallet.NetworkInfo], includingUserDeleted: Bool) -> [NetworkAssets] {
    var allUserAssets: [NetworkAssets] = []
    for (index, network) in networks.enumerated() {
      let groupId = network.walletUserAssetGroupId
      if let walletUserAssets = WalletUserAssetGroup.getGroup(groupId: groupId)?.walletUserAssets?.filter({
        if !includingUserDeleted {
          return $0.isDeletedByUser == false
        }
        return true
      }) {
        let networkAsset = NetworkAssets(
          network: network,
          tokens: walletUserAssets.map(\.blockchainToken),
          sortOrder: index
        )
        allUserAssets.append(networkAsset)
      }
    }
    return allUserAssets.sorted(by: { $0.sortOrder < $1.sortOrder })
  }
  
  /// Return either all visible or invisible user's assets (no spam) stored in CoreData based on
  /// the given `visible` value
  public func getAllUserAssetsInNetworkAssetsByVisibility(networks: [BraveWallet.NetworkInfo], visible: Bool) -> [NetworkAssets] {
    var allVisibleUserAssets: [NetworkAssets] = []
    for (index, network) in networks.enumerated() {
      let groupId = network.walletUserAssetGroupId
      if let walletUserAssets = WalletUserAssetGroup.getGroup(groupId: groupId)?.walletUserAssets?.filter({ $0.visible == visible && $0.isSpam == false && $0.isDeletedByUser == false }) {
        let networkAsset = NetworkAssets(
          network: network,
          tokens: walletUserAssets.map(\.blockchainToken),
          sortOrder: index
        )
        allVisibleUserAssets.append(networkAsset)
      }
    }
    return allVisibleUserAssets.sorted(by: { $0.sortOrder < $1.sortOrder })
  }
  
  /// Return all user NFTs stored in CoreData with the given `isSpam` status
  public func getAllUserNFTs(networks: [BraveWallet.NetworkInfo], isSpam: Bool) -> [NetworkAssets] {
    var allUserSpamAssets: [NetworkAssets] = []
    for (index, network) in networks.enumerated() {
      let groupId = network.walletUserAssetGroupId
      if let walletUserAssets = WalletUserAssetGroup.getGroup(groupId: groupId)?.walletUserAssets?.filter({ $0.isSpam == isSpam && ($0.isERC721 || $0.isNFT) && $0.isDeletedByUser == false }) { // Even though users can only spam/unspam NFTs, but we put the NFT filter here to make sure only NFTs are returned
        let networkAsset = NetworkAssets(
          network: network,
          tokens: walletUserAssets.map(\.blockchainToken),
          sortOrder: index
        )
        allUserSpamAssets.append(networkAsset)
      }
    }
    return allUserSpamAssets.sorted(by: { $0.sortOrder < $1.sortOrder })
  }
  
  public func getAllUserDeletedNFTs() -> [WalletUserAsset] {
    WalletUserAsset.getAllUserDeletedUserAssets() ?? []
  }
  
  public func getUserAsset(_ asset: BraveWallet.BlockchainToken) -> WalletUserAsset? {
    WalletUserAsset.getUserAsset(asset: asset)
  }
  
  public func addUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
    if let existedAsset = WalletUserAsset.getUserAsset(asset: asset) {
      if existedAsset.isDeletedByUser { // this asset was added before but user marked as deleted after
        WalletUserAsset.updateUserAsset(for: asset, visible: true, isSpam: false, isDeletedByUser: false, completion: completion)
      } else { // this asset already exists
        completion?()
        return
      }
    } else { // asset does not exist in database
      WalletUserAsset.addUserAsset(asset: asset, completion: completion)
    }
  }
  
  public func removeUserAsset(_ asset: BraveWallet.BlockchainToken, completion: (() -> Void)?) {
    WalletUserAsset.removeUserAsset(asset: asset, completion: completion)
  }
  
  public func updateUserAsset(
    for asset: BraveWallet.BlockchainToken,
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool,
    completion: (() -> Void)?
  ) {
    WalletUserAsset.updateUserAsset(
      for: asset,
      visible: visible,
      isSpam: isSpam,
      isDeletedByUser: isDeletedByUser,
      completion: completion
    )
  }
  
  public func removeGroup(for groupId: String, completion: (() -> Void)?) {
    WalletUserAssetGroup.removeGroup(groupId, completion: completion)
  }
  
  public func migrateUserAssets(completion: (() -> Void)? = nil) {
    Task { @MainActor in
      if !Preferences.Wallet.migrateCoreToWalletUserAssetCompleted.value {
        migrateUserAssets(for: Array(WalletConstants.supportedCoinTypes()), completion: completion)
      } else {
        let allNetworks = await rpcService.allNetworksForSupportedCoins(respectTestnetPreference: false)
        DataController.performOnMainContext { context in
          let newCoins = self.allNewCoinsIntroduced(networks: allNetworks, context: context)
          if !newCoins.isEmpty {
            self.migrateUserAssets(for: newCoins, completion: completion)
          } else {
            completion?()
          }
        }
      }
    }
  }
  
  private func allNewCoinsIntroduced(networks: [BraveWallet.NetworkInfo], context: NSManagedObjectContext) -> [BraveWallet.CoinType] {
    guard let assetGroupIds = WalletUserAssetGroup.getAllGroups(context: context)?.map({ group in
      group.groupId
    }) else { return WalletConstants.supportedCoinTypes().elements }
    var newCoins: Set<BraveWallet.CoinType> = []
    for network in networks where !assetGroupIds.contains("\(network.coin.rawValue).\(network.chainId)") {
      newCoins.insert(network.coin)
    }
    return Array(newCoins)
  }
  
  private func migrateUserAssets(for coins: [BraveWallet.CoinType], completion: (() -> Void)?) {
    Task { @MainActor in
      var fetchedUserAssets: [String: [BraveWallet.BlockchainToken]] = [:]
      let networks: [BraveWallet.NetworkInfo] = await rpcService.allNetworks(for: coins, respectTestnetPreference: false)
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
  public var _getAllUserAssetsInNetworkAssetsByVisibility: ((_ networks: [BraveWallet.NetworkInfo], _ visible: Bool) -> [NetworkAssets])?
  public var _getAllUserAssetsInNetworkAssets: ((_ networks: [BraveWallet.NetworkInfo], _ includingUserDeleted: Bool) -> [NetworkAssets])?
  public var _getAllUserNFTs: ((_ networks: [BraveWallet.NetworkInfo], _ spamStatus: Bool) -> [NetworkAssets])?
  public var _getAllUserDeletedNFTs: (() -> [WalletUserAsset])?
  
  public init() {}
  
  public func getAllUserAssetsInNetworkAssets(networks: [BraveWallet.NetworkInfo], includingUserDeleted: Bool) -> [NetworkAssets] {
    let defaultAssets: [NetworkAssets] = [
      NetworkAssets(network: .mockMainnet, tokens: [.previewToken], sortOrder: 0),
      NetworkAssets(network: .mockGoerli, tokens: [.previewToken], sortOrder: 1)
    ]
    let chainIds = networks.map { $0.chainId }
    return _getAllUserAssetsInNetworkAssets?(networks, includingUserDeleted) ?? defaultAssets.filter({
      chainIds.contains($0.network.chainId)
    })
  }
  
  public func getAllUserAssetsInNetworkAssetsByVisibility(networks: [BraveWallet.NetworkInfo], visible: Bool) -> [NetworkAssets] {
    _getAllUserAssetsInNetworkAssetsByVisibility?(networks, visible) ?? []
  }
  
  public func getAllUserNFTs(networks: [BraveWallet.NetworkInfo], isSpam: Bool) -> [NetworkAssets] {
    _getAllUserNFTs?(networks, isSpam) ?? []
  }
  
  public func getAllUserDeletedNFTs() -> [WalletUserAsset] {
    _getAllUserDeletedNFTs?() ?? []
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
  
  public func updateUserAsset(
    for asset: BraveWallet.BlockchainToken,
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool,
    completion: (() -> Void)?
  ) {
  }
}
#endif
