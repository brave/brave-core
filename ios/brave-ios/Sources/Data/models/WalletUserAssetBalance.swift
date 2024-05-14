// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CoreData
import Foundation
import Shared
import os.log

public final class WalletUserAssetBalance: NSManagedObject, CRUD {
  @NSManaged public var contractAddress: String
  @NSManaged public var symbol: String
  @NSManaged public var chainId: String
  @NSManaged public var tokenId: String
  @NSManaged public var balance: String
  @NSManaged public var accountAddress: String

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

  public init(
    context: NSManagedObjectContext,
    asset: BraveWallet.BlockchainToken,
    balance: String,
    account: String
  ) {
    let entity = Self.entity(context)
    super.init(entity: entity, insertInto: context)
    self.contractAddress = asset.contractAddress
    self.chainId = asset.chainId
    self.symbol = asset.symbol
    self.tokenId = asset.tokenId
    self.balance = balance
    self.accountAddress = account
  }

  /// - Parameters:
  ///     - asset: An optional value of `BraveWallet.BlockchainToken`, nil value will remove the restriction of asset matching
  ///     - account: An optional value of `String`. It is the account's address value. nil value will remove the restriction of account address matching
  ///     - context: An optional value of `NSManagedObjectContext`
  ///
  /// - Returns: An optional of list of `WalletUserAssetBalance` that matches given parameters.
  public static func getBalances(
    for asset: BraveWallet.BlockchainToken? = nil,
    account: String? = nil,
    context: NSManagedObjectContext? = nil
  ) -> [WalletUserAssetBalance]? {
    if asset == nil, account == nil {
      // all `WalletAssetBalnce` with no restriction on assets or accounts
      return WalletUserAssetBalance.all()
    } else if let asset, account == nil {
      let predicate = NSPredicate(
        format: "contractAddress == %@ && chainId == %@ && symbol == %@ && tokenId == %@",
        asset.contractAddress,
        asset.chainId,
        asset.symbol,
        asset.tokenId
      )
      return WalletUserAssetBalance.all(
        where: predicate,
        context: context ?? DataController.viewContext
      )
    } else if asset == nil, let account {
      return WalletUserAssetBalance.all(
        where: NSPredicate(format: "accountAddress == %@", account),
        context: context ?? DataController.viewContext
      )
    } else if let asset, let account {
      let predicate = NSPredicate(
        format:
          "contractAddress == %@ && chainId == %@ && symbol == %@ && tokenId == %@ && accountAddress == %@",
        asset.contractAddress,
        asset.chainId,
        asset.symbol,
        asset.tokenId,
        account
      )
      return WalletUserAssetBalance.all(
        where: predicate,
        context: context ?? DataController.viewContext
      )
    }
    return nil
  }

  public static func updateBalance(
    for asset: BraveWallet.BlockchainToken,
    balance: String,
    account: String,
    completion: (() -> Void)? = nil
  ) {
    DataController.perform(context: .new(inMemory: false), save: false) { context in
      let predicate = NSPredicate(
        format:
          "contractAddress == %@ && chainId == %@ && symbol == %@ && tokenId == %@ && accountAddress == %@",
        asset.contractAddress,
        asset.chainId,
        asset.symbol,
        asset.tokenId,
        account
      )
      if let asset = WalletUserAssetBalance.first(where: predicate, context: context) {
        asset.balance = balance
      } else {
        _ = WalletUserAssetBalance(
          context: context,
          asset: asset,
          balance: balance,
          account: account
        )
      }

      WalletUserAssetBalance.saveContext(context)

      DispatchQueue.main.async {
        completion?()
      }
    }
  }

  /// - Parameters:
  ///     - asset: An optional value of `BraveWallet.BlockchainToken` to be removed from CD, nil value will remove the restriction of asset matching
  ///     - account: An optional value of `String`. It is the account's address value. nil value will remove the restriction of account address matching
  ///     - completion: An optional completion block
  public static func removeBalances(
    for asset: BraveWallet.BlockchainToken? = nil,
    account: String? = nil,
    completion: (() -> Void)? = nil
  ) {
    var predicate: NSPredicate?
    if let asset, let accountAddress = account {
      predicate = NSPredicate(
        format:
          "contractAddress == %@ && chainId == %@ && symbol == %@ && tokenId == %@ && accountAddress == %@",
        asset.contractAddress,
        asset.chainId,
        asset.symbol,
        asset.tokenId,
        accountAddress
      )
    } else if let asset {
      predicate = NSPredicate(
        format: "contractAddress == %@ && chainId == %@ && symbol == %@ && tokenId == %@",
        asset.contractAddress,
        asset.chainId,
        asset.symbol,
        asset.tokenId
      )
    }
    WalletUserAssetBalance.deleteAll(
      predicate: predicate,
      completion: completion
    )
  }

  /// - Parameters:
  ///     - network: `BraveWallet.NetworkInfo`, any user asset balance that matches this network's chainId
  ///     will be removed
  ///     - completion: An optional completion block
  public static func removeBalances(
    for network: BraveWallet.NetworkInfo,
    completion: (() -> Void)? = nil
  ) {
    let predict = NSPredicate(format: "chainId == %@", network.chainId)
    WalletUserAssetBalance.deleteAll(
      predicate: predict,
      completion: completion
    )
  }

  /// - Parameters:
  ///     - account: `BraveWallet.AccountInfo`, the account that we need to update the value of `accountAddress`
  public static func updateAccountAddress(
    for account: BraveWallet.AccountInfo
  ) async {
    await withCheckedContinuation { continuation in
      DataController.perform(context: .new(inMemory: false), save: false) { context in
        let predicate = NSPredicate(
          format: "accountAddress == %@",
          account.address
        )
        if let asset = WalletUserAssetBalance.first(where: predicate, context: context) {
          asset.accountAddress = account.accountId.uniqueKey
          WalletUserAssetBalance.saveContext(context)
        }
        continuation.resume()
      }
    }
  }
}

extension WalletUserAssetBalance {
  private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
    NSEntityDescription.entity(forEntityName: "WalletUserAssetBalance", in: context)!
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
