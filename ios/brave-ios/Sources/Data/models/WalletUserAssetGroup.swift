// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import os.log

public final class WalletUserAssetGroup: NSManagedObject, CRUD {
  @NSManaged public var groupId: String
  @NSManaged public var walletUserAssets: Set<WalletUserAsset>?
  
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
  
  public init(context: NSManagedObjectContext, groupId: String) {
    let entity = Self.entity(context)
    super.init(entity: entity, insertInto: context)
    self.groupId = groupId
  }
  
  public static func getGroup(groupId: String, context: NSManagedObjectContext? = nil) -> WalletUserAssetGroup? {
    WalletUserAssetGroup.first(where: NSPredicate(format: "groupId == %@", groupId), context: context ?? DataController.viewContext)
  }
  
  public static func getAllGroups(context: NSManagedObjectContext? = nil) -> [WalletUserAssetGroup]? {
    WalletUserAssetGroup.all(context: context ?? DataController.viewContext)
  }
  
  public static func removeGroup(_ groupId: String, completion: (() -> Void)? = nil) {
    WalletUserAssetGroup.deleteAll(
      predicate: NSPredicate(format: "groupId == %@", groupId),
      completion: completion
    )
  }
  
  public static func removeAllGroup(completion: (() -> Void)? = nil) {
    WalletUserAssetGroup.deleteAll(
      completion: completion
    )
  }
  
  public static func groupExists(groupId: String) -> Bool {
    if let count = WalletUserAssetGroup.count(predicate: NSPredicate(format: "groupId == %@", groupId)), count > 0 {
      return true
    }
    return false
  }
}

extension WalletUserAssetGroup {
  private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
    NSEntityDescription.entity(forEntityName: "WalletUserAssetGroup", in: context)!
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
