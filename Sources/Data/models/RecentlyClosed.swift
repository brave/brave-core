// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import os.log

public struct SavedRecentlyClosed {
  public let url: String
  public let title: String
  public let dateAdded: Date
  public let interactionState: Data?
  public let index: Int32

  public init(url: URL, title: String, dateAdded: Date = .now, interactionState: Data?, order: Int32) {
    self.url = url.absoluteString
    self.title = title
    self.dateAdded = dateAdded
    self.interactionState = interactionState
    self.index = order
  }
}

public final class RecentlyClosed: NSManagedObject, CRUD {
  @NSManaged public var url: String
  @NSManaged public var title: String?
  @NSManaged public var dateAdded: Date
  @NSManaged public var interactionState: Data?
  @NSManaged public var historyIndex: Int16
  
  public class func get(with url: String) -> RecentlyClosed? {
    return getInternal(with: url)
  }

  public class func all() -> [RecentlyClosed] {
    let sortDescriptors = [NSSortDescriptor(key: #keyPath(RecentlyClosed.dateAdded), ascending: false)]
    return all(sortDescriptors: sortDescriptors) ?? []
  }

  public class func remove(with url: String) {
    DataController.perform { context in
      if let item = getInternal(with: url, context: context) {
        item.delete(context: .existing(context))
      }
    }
  }

  public static func removeAll() {
    RecentlyClosed.deleteAll()
  }
  
  public class func insert(_ saved: SavedRecentlyClosed) {
    Self.insertAll([saved])
  }
  
  public class func insertAll(_ savedList: [SavedRecentlyClosed]) {
    DataController.perform { context in
      savedList.enumerated().forEach { index, saved in
        if let entity = entity(in: context) {
          let source = RecentlyClosed(entity: entity, insertInto: context)
          source.url = saved.url
          source.title = saved.title
          source.dateAdded = saved.dateAdded
          source.interactionState = saved.interactionState
          source.historyIndex = Int16(index)
        }
      }
    }
  }
  
  public class func deleteAll(olderThan timeInterval: TimeInterval) {
    let addedKeyPath = #keyPath(RecentlyClosed.dateAdded)
    let date = Date().advanced(by: -timeInterval) as NSDate
    
    let predicate = NSPredicate(format: "\(addedKeyPath) != nil AND \(addedKeyPath) < %@", date)
    
    self.deleteAll(predicate: predicate)
  }

  // MARK: Private
  
  private class func entity(in context: NSManagedObjectContext) -> NSEntityDescription? {
    NSEntityDescription.entity(forEntityName: "RecentlyClosed", in: context)
  }

  private class func getInternal(
    with url: String,
    context: NSManagedObjectContext = DataController.viewContext) -> RecentlyClosed? {
      let predicate = NSPredicate(format: "\(#keyPath(RecentlyClosed.url)) == %@", argumentArray: [url])

    return first(where: predicate, context: context)
  }
}
