// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CoreData
import Foundation
import Shared
import os.log

/// Model used for storing token visibility in CoreData to prevent brave-browser #28749.
/// If a user hides/deletes a token they have balance for, it may be auto-discovered again and set to visible. As a result we need to know if a user previously hid/deleted a token so it's not re-marked visible.
/// Notes:
///  - Previously this model was used as source of truth for tokens, but Wallet Service is now source of truth.
///  - 'Retired' properties:
///    - These models were used to reflect `BlockchainToken`. When a user added a custom asset, it was only added to Core Data not Wallet Service.
///      Because of this, these existing properties can't be removed so we can migrate existing custom assets from Core Data not Wallet Service.
///  - New properties on `BlockchainToken` should not be added here anymore unless required for identifying a token for visibility.
public final class WalletUserAsset: NSManagedObject, CRUD {
  // Following properties are used to identify tokens
  @NSManaged public var contractAddress: String
  @NSManaged public var chainId: String
  @NSManaged public var symbol: String
  @NSManaged public var tokenId: String
  // Following properties are the source of truth for
  // determining the status of visibility and deletion
  // respectfully
  @NSManaged public var visible: Bool
  @NSManaged public var isDeletedByUser: Bool
  // This property is the source of truth for user marked
  // spam assets
  @NSManaged public var isSpam: Bool
  // Following properties are `retired`. Values should be
  // read from `WalletService`
  @NSManaged public var name: String
  @NSManaged public var logo: String
  @NSManaged public var isCompressed: Bool
  @NSManaged public var isERC20: Bool
  @NSManaged public var isERC721: Bool
  @NSManaged public var isERC1155: Bool
  @NSManaged public var splTokenProgram: NSNumber?
  @NSManaged public var isNFT: Bool
  @NSManaged public var decimals: Int32
  @NSManaged public var coingeckoId: String
  @NSManaged public var coin: Int16
  // Relationship property
  @NSManaged public var walletUserAssetGroup: WalletUserAssetGroup?

  /// Helper to get the enum value based on the stored optional Int16. If nil, infer value based on `coin`.
  private var splTokenProgramEnumValue: BraveWallet.SPLTokenProgram {
    if let splTokenProgramValue = self.splTokenProgram?.intValue,
      let splTokenProgram = BraveWallet.SPLTokenProgram(rawValue: Int(splTokenProgramValue))
    {
      return splTokenProgram
    } else {
      return .unknown
    }
  }

  public var id: String {
    contractAddress + chainId + symbol + tokenId
  }

  public var blockchainToken: BraveWallet.BlockchainToken {
    .init(
      contractAddress: self.contractAddress,
      name: self.name,
      logo: self.logo,
      isCompressed: self.isCompressed,
      isErc20: self.isERC20,
      isErc721: self.isERC721,
      isErc1155: self.isERC1155,
      splTokenProgram: self.splTokenProgramEnumValue,
      isNft: self.isNFT,
      isSpam: self.isSpam,
      symbol: self.symbol,
      decimals: self.decimals,
      visible: self.visible,
      tokenId: self.tokenId,
      coingeckoId: self.coingeckoId,
      chainId: self.chainId,
      coin: BraveWallet.CoinType(rawValue: Int(self.coin))!,
      isShielded: false
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
    self.isCompressed = asset.isCompressed
    self.isERC20 = asset.isErc20
    self.isERC721 = asset.isErc721
    self.isERC1155 = asset.isErc1155
    self.splTokenProgram = NSNumber(value: asset.splTokenProgram.rawValue)
    self.isNFT = asset.isNft
    self.isSpam = asset.isSpam
    self.isCompressed = asset.isCompressed
    self.symbol = asset.symbol
    self.decimals = asset.decimals
    self.visible = asset.visible
    self.tokenId = asset.tokenId
    self.coingeckoId = asset.coingeckoId
    self.chainId = asset.chainId
    self.coin = Int16(asset.coin.rawValue)
    // `isDeletedByUser` has a default value `NO`
  }

  public static func getAllUserAssets(
    context: NSManagedObjectContext? = nil
  ) -> [WalletUserAsset] {
    WalletUserAsset.all(
      context: context ?? DataController.viewContext
    ) ?? []
  }

  public static func getUserAsset(
    token: BraveWallet.BlockchainToken,
    context: NSManagedObjectContext? = nil
  ) -> WalletUserAsset? {
    return WalletUserAsset.first(
      where: NSPredicate(
        format: "contractAddress ==[c] %@ AND chainId == %@ AND symbol == %@ AND tokenId == %@",
        token.contractAddress,
        token.chainId,
        token.symbol,
        token.tokenId
      ),
      context: context ?? DataController.viewContext
    )
  }

  public static func getAllUserAssets(
    visible: Bool,
    context: NSManagedObjectContext? = nil
  ) -> [WalletUserAsset]? {
    WalletUserAsset.all(
      where: NSPredicate(format: "visible == %@", NSNumber(booleanLiteral: visible)),
      context: context ?? DataController.viewContext
    )
  }

  public static func getAllUserDeletedUserAssets(
    context: NSManagedObjectContext? = nil
  ) -> [WalletUserAsset]? {
    WalletUserAsset.all(
      where: NSPredicate(format: "isDeletedByUser == true"),
      context: context ?? DataController.viewContext
    )
  }

  public static func migrateVisibleAssets(
    _ assets: [String: [BraveWallet.BlockchainToken]]
  ) async {
    await withCheckedContinuation { continuation in
      DataController.perform(context: .new(inMemory: false), save: false) { context in
        for groupId in assets.keys {
          guard let assetsInOneGroup = assets[groupId] else { return }
          let group =
            WalletUserAssetGroup.getGroup(groupId: groupId, context: context)
            ?? WalletUserAssetGroup(context: context, groupId: groupId)
          for asset in assetsInOneGroup
          where WalletUserAsset.getUserAsset(token: asset, context: context) == nil {
            let visibleAsset = WalletUserAsset(context: context, asset: asset)
            visibleAsset.walletUserAssetGroup = group
          }

          WalletUserAsset.saveContext(context)
        }
        DispatchQueue.main.async {
          continuation.resume()
        }
      }
    }
  }

  @MainActor public static func updateUserAsset(
    for token: BraveWallet.BlockchainToken,
    visible: Bool,
    isSpam: Bool,
    isDeletedByUser: Bool
  ) async {
    await withCheckedContinuation { @MainActor continuation in
      DataController.perform(context: .new(inMemory: false), save: false) { context in
        if let asset = WalletUserAsset.first(
          where: NSPredicate(
            format: "contractAddress ==[c] %@ AND chainId == %@ AND symbol == %@ AND tokenId == %@",
            token.contractAddress,
            token.chainId,
            token.symbol,
            token.tokenId
          ),
          context: context
        ) {
          asset.visible = visible
          asset.isSpam = isSpam
          asset.isDeletedByUser = isDeletedByUser
        } else {
          let groupId = token.walletUserAssetGroupId
          let group =
            WalletUserAssetGroup.getGroup(groupId: groupId, context: context)
            ?? WalletUserAssetGroup(context: context, groupId: groupId)
          let visibleAsset = WalletUserAsset(context: context, asset: token)
          visibleAsset.visible = visible
          visibleAsset.isSpam = isSpam
          visibleAsset.isDeletedByUser = isDeletedByUser
          visibleAsset.walletUserAssetGroup = group
        }

        WalletUserAsset.saveContext(context)

        DispatchQueue.main.async {
          continuation.resume()
        }
      }
    }
  }

  public static func addUserAsset(
    token: BraveWallet.BlockchainToken
  ) async {
    await withCheckedContinuation { continuation in
      DataController.perform(context: .new(inMemory: false), save: false) { context in
        let groupId = token.walletUserAssetGroupId
        let group =
          WalletUserAssetGroup.getGroup(groupId: groupId, context: context)
          ?? WalletUserAssetGroup(context: context, groupId: groupId)
        let visibleAsset = WalletUserAsset(context: context, asset: token)
        visibleAsset.visible = true  // (`isSpam` and `isDeletedByUser` have a default value `NO`)
        visibleAsset.walletUserAssetGroup = group

        WalletUserAsset.saveContext(context)

        DispatchQueue.main.async {
          continuation.resume()
        }
      }
    }
  }

  public static func removeUserAsset(
    token: BraveWallet.BlockchainToken
  ) async {
    await withCheckedContinuation { continuation in
      WalletUserAsset.deleteAll(
        predicate: NSPredicate(
          format: "contractAddress == %@ AND chainId == %@ AND symbol == %@ AND tokenId == %@",
          token.contractAddress,
          token.chainId,
          token.symbol,
          token.tokenId
        ),
        completion: {
          continuation.resume()
        }
      )
    }
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

extension BraveWallet.BlockchainToken {
  /// The group id that this token should be stored with in CoreData
  /// - Warning: This format must to updated if
  /// `BraveWallet.NetworkInfo.walletUserAssetGroupId` format is
  ///  changed under `BraveWallet`
  fileprivate var walletUserAssetGroupId: String {
    "\(coin.rawValue).\(chainId)"
  }
}
