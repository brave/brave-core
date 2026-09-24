// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreData
import Foundation
import Shared
import os.log

public final class FilterListSetting: NSManagedObject, CRUD {
  @MainActor @NSManaged public var uuid: String
  @MainActor @NSManaged public var componentId: String?
  @MainActor @NSManaged public var isEnabled: Bool
  @MainActor @NSManaged public var isHidden: Bool
  @MainActor @NSManaged public var isAlwaysAggressive: Bool
  @MainActor @NSManaged public var isDefaultEnabled: Bool
  @MainActor @NSManaged public var order: NSNumber?
  @MainActor @NSManaged public var folderPath: String?

  /// Tells us which filter lists should be compiled during launch.
  ///
  /// The filter lists that will be eagerly loaded are ones that are:
  /// 1. Enabled
  /// 2. Hidden (i.e. there is no UI to disable/enable them).
  /// This includes the "default" and "first party" filter lists.
  /// These are not available when using the regional catalog (i.e. `regional_catalog.json`).
  @MainActor public var isEagerlyLoaded: Bool {
    return isEnabled && isHidden
  }

  /// Load all the flter list settings
  @MainActor public class func loadAllSettings(fromMemory: Bool) -> [FilterListSetting] {
    return all(
      context: fromMemory ? DataController.viewContextInMemory : DataController.viewContext
    ) ?? []
  }

  /// Create a filter list setting for the given UUID and enabled status
  @MainActor public class func create(
    uuid: String,
    componentId: String?,
    isEnabled: Bool,
    isHidden: Bool,
    order: Int,
    inMemory: Bool,
    isAlwaysAggressive: Bool,
    isDefaultEnabled: Bool
  ) -> FilterListSetting {
    var newSetting: FilterListSetting!

    // Settings are usually accesed on view context, but when the setting doesn't exist,
    // we have to switch to a background context to avoid writing on view context(bad practice).
    let writeContext =
      inMemory
      ? DataController.newBackgroundContextInMemory() : DataController.newBackgroundContext()

    save(on: writeContext) {
      newSetting = FilterListSetting(
        entity: FilterListSetting.entity(writeContext),
        insertInto: writeContext
      )
      newSetting.uuid = uuid
      newSetting.componentId = componentId
      newSetting.isEnabled = isEnabled
      newSetting.isHidden = isHidden
      newSetting.isAlwaysAggressive = isAlwaysAggressive
      newSetting.isDefaultEnabled = isDefaultEnabled
      newSetting.order = NSNumber(value: order)
    }

    let viewContext = inMemory ? DataController.viewContextInMemory : DataController.viewContext
    let settingOnCorrectContext =
      viewContext.object(with: newSetting.objectID) as? FilterListSetting
    return settingOnCorrectContext ?? newSetting
  }

  @MainActor public class func save(inMemory: Bool) {
    self.save(on: inMemory ? DataController.viewContextInMemory : DataController.viewContext)
  }

  /// Save this entry
  @MainActor private class func save(
    on writeContext: NSManagedObjectContext,
    changes: (() -> Void)? = nil
  ) {
    writeContext.performAndWait {
      changes?()

      if writeContext.hasChanges {
        do {
          try writeContext.save()
        } catch {
          Logger.module.error("FilterListSetting save error: \(error.localizedDescription)")
        }
      }
    }
  }

  @MainActor public func delete(inMemory: Bool) {
    let viewContext = inMemory ? DataController.viewContextInMemory : DataController.viewContext

    Self.save(on: viewContext) {
      self.delete(context: .existing(viewContext))
    }
  }

  // Currently required, because not `syncable`
  @MainActor private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
    return NSEntityDescription.entity(forEntityName: "FilterListSetting", in: context)!
  }
}
