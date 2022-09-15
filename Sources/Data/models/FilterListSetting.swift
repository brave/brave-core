// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared

private let log = Logger.browserLogger

public final class FilterListSetting: NSManagedObject, CRUD {
  /// The directory to which we should store all the dowloaded files into
  private static var filterListBaseFolderURL: URL? {
    let location = FileManager.SearchPathDirectory.applicationSupportDirectory
    return FileManager.default.urls(for: location, in: .userDomainMask).first
  }
  
  @NSManaged public var uuid: String
  @NSManaged public var componentId: String?
  @NSManaged public var isEnabled: Bool
  @NSManaged private var folderPath: String?

  public var folderURL: URL? {
    get {
      return Self.makeFolderURL(forFilterListFolderPath: folderPath)
    }
    set {
      // We need to extract the path. We don't want to store the full URL
      self.folderPath = Self.extractFolderPath(fromFilterListFolderURL: newValue)
    }
  }
  
  /// Load all the flter list settings
  public class func loadAllSettings(fromMemory: Bool) -> [FilterListSetting] {
    return all(context: fromMemory ? DataController.viewContextInMemory : DataController.viewContext) ?? []
  }
  
  /// Create a filter list setting for the given UUID and enabled status
  public class func create(uuid: String, componentId: String?, isEnabled: Bool, inMemory: Bool) -> FilterListSetting {
    var newSetting: FilterListSetting!

    // Settings are usually accesed on view context, but when the setting doesn't exist,
    // we have to switch to a background context to avoid writing on view context(bad practice).
    let writeContext = inMemory ? DataController.newBackgroundContextInMemory() : DataController.newBackgroundContext()

    save(on: writeContext) {
      newSetting = FilterListSetting(entity: FilterListSetting.entity(writeContext), insertInto: writeContext)
      newSetting.uuid = uuid
      newSetting.componentId = componentId
      newSetting.isEnabled = isEnabled
    }

    let viewContext = inMemory ? DataController.viewContextInMemory : DataController.viewContext
    let settingOnCorrectContext = viewContext.object(with: newSetting.objectID) as? FilterListSetting
    return settingOnCorrectContext ?? newSetting
  }
  
  public class func save(inMemory: Bool) {
    self.save(on: inMemory ? DataController.viewContextInMemory : DataController.viewContext)
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
          log.error("FilterListSetting save error: \(error)")
        }
      }
    }
  }
  
  // Currently required, because not `syncable`
  private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
    return NSEntityDescription.entity(forEntityName: "FilterListSetting", in: context)!
  }

  public static func makeFolderURL(forFilterListFolderPath folderPath: String?) -> URL? {
    // Combine the path with the base URL
    guard let folderPath = folderPath else { return nil }
    return filterListBaseFolderURL?.appendingPathComponent(folderPath)
  }
  
  public static func extractFolderPath(fromFilterListFolderURL folderURL: URL?) -> String? {
    guard let baseURL = filterListBaseFolderURL, let folderURL = folderURL else {
      return nil
    }
    
    // We take the path after the base url
    if let range = folderURL.path.range(of: baseURL.path) {
      let folderPath = folderURL.path[range.upperBound...]
      return String(folderPath)
    } else {
      return nil
    }
  }
}
