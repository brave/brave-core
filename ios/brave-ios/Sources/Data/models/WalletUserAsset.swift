// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import BraveCore
import os.log

public final class WalletUserAsset: NSManagedObject, CRUD {
  @NSManaged public var contractAddress: String
  @NSManaged public var name: String
  @NSManaged public var logo: String
  @NSManaged public var isERC20: Bool
  @NSManaged public var isERC721: Bool
  @NSManaged public var isERC1155: Bool
  @NSManaged public var isNFT: Bool
  @NSManaged public var isSpam: Bool
  @NSManaged public var symbol: String
  @NSManaged public var decimals: Int32
  @NSManaged public var visible: Bool
  @NSManaged public var tokenId: String
  @NSManaged public var coingeckoId: String
  @NSManaged public var chainId: String
  @NSManaged public var coin: Int16
  @NSManaged public var isDeletedByUser: Bool
  @NSManaged public var walletUserAssetGroup: WalletUserAssetGroup?
  
  public var blockchainToken: BraveWallet.BlockchainToken {
    .init(
      contractAddress: self.contractAddress,
      name: self.name,
      logo: self.logo,
      isErc20: self.isERC20,
      isErc721: self.isERC721,
      isErc1155: self.isERC1155,
      isNft: self.isNFT,
      isSpam: self.isSpam,
      symbol: self.symbol,
      decimals: self.decimals,
      visible: self.visible,
      tokenId: self.tokenId,
      coingeckoId: self.coingeckoId,
      chainId: self.chainId,
      coin: BraveWallet.CoinType(rawValue: Int(self.coin))!
    )
  }
  
  @available(*, unavailable)
  public init() {
    fatalError("No Such Initializer: init()")
  }
  
  @available(*, unavailable)
  public init(context: NSManagedObjectContext) {
    fatalError("No Such Initializer: init(context:)")
  }
  
  @objc
  private override init(entity: NSEntityDescription, insertInto context: NSManagedObjectContext?) {
    super.init(entity: entity, insertInto: context)
  }
  
  public init(context: NSManagedObjectContext, asset: BraveWallet.BlockchainToken) {
    let entity = Self.entity(context)
    super.init(entity: entity, insertInto: context)
    self.contractAddress = asset.contractAddress
    self.name = asset.name
    self.logo = asset.logo
    self.isERC20 = asset.isErc20
    self.isERC721 = asset.isErc721
    self.isERC1155 = asset.isErc1155
    self.isNFT = asset.isNft
    self.isSpam = asset.isSpam
    self.symbol = asset.symbol
    self.decimals = asset.decimals
    self.visible = asset.visible
    self.tokenId = asset.tokenId
    self.coingeckoId = asset.coingeckoId
    self.chainId = asset.chainId
    self.coin = Int16(asset.coin.rawValue)
    // `isDeletedByUser` has a default value `NO`
  }
  
  public static func getUserAsset(asset: BraveWallet.BlockchainToken, context: NSManagedObjectContext? = nil) -> WalletUserAsset? {
    WalletUserAsset.first(where: NSPredicate(format: "contractAddress == %@ AND chainId == %@ AND symbol == %@ AND tokenId == %@", asset.contractAddress, asset.chainId, asset.symbol, asset.tokenId), context: context ?? DataController.viewContext)
  }
  
  public static func getAllVisibleUserAssets(context: NSManagedObjectContext? = nil) -> [WalletUserAsset]? {
    WalletUserAsset.all(where: NSPredicate(format: "visible = true AND isDeletedByUser == false"), context: context ?? DataController.viewContext)
  }
  
  public static func getAllUserDeletedUserAssets(context: NSManagedObjectContext? = nil) -> [WalletUserAsset]? {
    WalletUserAsset.all(where: NSPredicate(format: "isDeletedByUser == true"), context: context ?? DataController.viewContext)
  }
  
  public static func migrateVisibleAssets(_ assets: [String: [BraveWallet.BlockchainToken]], completion: (() -> Void)? = nil) {
    DataController.perform(context: .new(inMemory: false), save: false) { context in
      for groupId in assets.keys {
        guard let assetsInOneGroup = assets[groupId] else { return }
        let group = WalletUserAssetGroup.getGroup(groupId: groupId, context: context) ?? WalletUserAssetGroup(context: context, groupId: groupId)
        for asset in assetsInOneGroup where WalletUserAsset.getUserAsset(asset: asset, context: context) == nil {
          let visibleAsset = WalletUserAsset(context: context, asset: asset)
          visibleAsset.walletUserAssetGroup = group
        }
        
        WalletUserAsset.saveContext(context)
      }
      DispatchQueue.main.async {
        completion?()
      }
    }
  }
  
  public static func updateUserAsset(
    for asset: BraveWallet.BlockchainToken,
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool,
    completion: (() -> Void)? = nil
  ) {
    DataController.perform(context: .new(inMemory: false), save: false) { context in
      if let asset = WalletUserAsset.first(where: NSPredicate(format: "contractAddress == %@ AND chainId == %@ AND symbol == %@ AND tokenId == %@", asset.contractAddress, asset.chainId, asset.symbol, asset.tokenId), context: context) {
        asset.visible = visible
        asset.isSpam = isSpam
        asset.isDeletedByUser = isDeletedByUser
      } else {
        let groupId = asset.walletUserAssetGroupId
        let group = WalletUserAssetGroup.getGroup(groupId: groupId, context: context) ?? WalletUserAssetGroup(context: context, groupId: groupId)
        let visibleAsset = WalletUserAsset(context: context, asset: asset)
        visibleAsset.visible = visible
        visibleAsset.isSpam = isSpam
        visibleAsset.isDeletedByUser = isDeletedByUser
        visibleAsset.walletUserAssetGroup = group
      }
      
      WalletUserAsset.saveContext(context)
      
      DispatchQueue.main.async {
        completion?()
      }
    }
  }
  
  public static func addUserAsset(asset: BraveWallet.BlockchainToken, completion: (() -> Void)? = nil) {
    DataController.perform(context: .new(inMemory: false), save: false) { context in
      let groupId = asset.walletUserAssetGroupId
      let group = WalletUserAssetGroup.getGroup(groupId: groupId, context: context) ?? WalletUserAssetGroup(context: context, groupId: groupId)
      let visibleAsset = WalletUserAsset(context: context, asset: asset)
      visibleAsset.visible = true // (`isSpam` and `isDeletedByUser` have a default value `NO`)
      visibleAsset.walletUserAssetGroup = group
      
      WalletUserAsset.saveContext(context)
      
      DispatchQueue.main.async {
        completion?()
      }
    }
  }
  
  public static func removeUserAsset(asset: BraveWallet.BlockchainToken, completion: (() -> Void)? = nil) {
    WalletUserAsset.deleteAll(
      predicate: NSPredicate(format: "contractAddress == %@ AND chainId == %@ AND symbol == %@ AND tokenId == %@", asset.contractAddress, asset.chainId, asset.symbol, asset.tokenId),
      completion: completion
    )
  }
}

extension WalletUserAsset {
  private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
    NSEntityDescription.entity(forEntityName: "WalletUserAsset", in: context)!
  }
  
  private static func saveContext(_ context: NSManagedObjectContext) {
    if context.concurrencyType == .mainQueueConcurrencyType {
      Logger.module.warning("Writing to view context, this should be avoided.")
    }
    
    if context.hasChanges {
      do {
        try context.save()
      } catch {
        assertionFailure("Error saving DB: \(error.localizedDescription)")
      }
    }
  }
}

private extension BraveWallet.BlockchainToken {
  /// The group id that this token should be stored with in CoreData
  /// - Warning: This format must to updated if
  /// `BraveWallet.NetworkInfo.walletUserAssetGroupId` format is
  ///  changed under `BraveWallet`
  var walletUserAssetGroupId: String {
    "\(coin.rawValue).\(chainId)"
  }
}
