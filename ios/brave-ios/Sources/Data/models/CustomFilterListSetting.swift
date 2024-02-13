// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import os.log

@MainActor public final class CustomFilterListSetting: NSManagedObject, CRUD, Identifiable {
  @NSManaged public var uuid: String
  @NSManaged public var isEnabled: Bool
  @NSManaged public var externalURL: URL
  
  /// Load all the flter list settings
  public class func loadAllSettings(fromMemory: Bool) -> [CustomFilterListSetting] {
    return all(context: fromMemory ? DataController.viewContextInMemory : DataController.viewContext) ?? []
  }
  
  /// Create a filter list setting for the given UUID and enabled status
  public class func create(externalURL: URL, isEnabled: Bool, inMemory: Bool) -> CustomFilterListSetting {
    var newSetting: CustomFilterListSetting!
    
    // Settings are usually accesed on view context, but when the setting doesn't exist,
    // we have to switch to a background context to avoid writing on view context(bad practice).
    let writeContext = inMemory ? DataController.newBackgroundContextInMemory() : DataController.newBackgroundContext()
    
    save(on: writeContext) {
      newSetting = CustomFilterListSetting(entity: CustomFilterListSetting.entity(writeContext), insertInto: writeContext)
      newSetting.uuid = UUID().uuidString
      newSetting.isEnabled = isEnabled
      newSetting.externalURL = externalURL
    }
    
    let viewContext = inMemory ? DataController.viewContextInMemory : DataController.viewContext
    let settingOnCorrectContext = viewContext.object(with: newSetting.objectID) as? CustomFilterListSetting
    return settingOnCorrectContext ?? newSetting
  }
  
  public class func save(inMemory: Bool) {
    self.save(on: inMemory ? DataController.viewContextInMemory : DataController.viewContext)
  }
  
  public func delete(inMemory: Bool) {
    let viewContext = inMemory ? DataController.viewContextInMemory : DataController.viewContext
    
    Self.save(on: viewContext) {
      self.delete(context: .existing(viewContext))
    }
  }
  
  /// Save this entry
  private class func save(
    on writeContext: NSManagedObjectContext,
    changes: (() -> Void)? = nil
  ) {
    writeContext.performAndWait {
      changes?()
      
      if writeContext.hasChanges {
        do {
          try writeContext.save()
        } catch {
          Logger.module.error("CustomFilterListSetting save error: \(error.localizedDescription)")
        }
      }
    }
  }
  
  // Currently required, because not `syncable`
  private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
    return NSEntityDescription.entity(forEntityName: "CustomFilterListSetting", in: context)!
  }
}
