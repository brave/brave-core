// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import os.log

public enum RecentSearchType: Int32 {
  case qrCode = 0
  case text = 1
  case website = 2
}

@objc(RecentSearch)
final public class RecentSearch: NSManagedObject, CRUD {
  @NSManaged public var searchType: Int32
  @NSManaged public var text: String?
  @NSManaged public var websiteUrl: String?
  @NSManaged public var dateAdded: Date?

  public class func frc() -> NSFetchedResultsController<RecentSearch> {
    let context = DataController.viewContext
    let fetchRequest = NSFetchRequest<RecentSearch>()
    fetchRequest.entity = RecentSearch.entity(context)
    fetchRequest.fetchBatchSize = 5

    let createdSort = NSSortDescriptor(key: "dateAdded", ascending: false)
    fetchRequest.sortDescriptors = [createdSort]

    return NSFetchedResultsController(
      fetchRequest: fetchRequest, managedObjectContext: context,
      sectionNameKeyPath: nil, cacheName: nil)
  }

  public func update(dateAdded: Date) {
    if let context = self.managedObjectContext {
      let objectId = self.objectID
      DataController.perform(context: .existing(context), save: true) { context in
        let recentSearch = context.object(with: objectId) as? RecentSearch
        recentSearch?.dateAdded = dateAdded
      }
    } else {
      let objectId = self.objectID
      DataController.perform { context in
        let recentSearch = context.object(with: objectId) as? RecentSearch
        recentSearch?.dateAdded = dateAdded
      }
    }
  }

  public static func addItem(type: RecentSearchType, text: String?, websiteUrl: String?) {
    let isNullOrEmpty = { (string: String?) in
      return (string ?? "").trimmingCharacters(in: CharacterSet.whitespacesAndNewlines).isEmpty
    }

    // If both are empty, do NOT add it to Recent Searches
    // It's better to check here since a RecentSearch can be added from multiple places
    // and can contain either field.
    if isNullOrEmpty(text) && isNullOrEmpty(websiteUrl) {
      return
    }

    if let recentSearch = getItem(text: text, websiteUrl: websiteUrl) {
      recentSearch.update(dateAdded: Date())
    } else {
      DataController.perform(context: .new(inMemory: false), save: true) { context in
        let item = RecentSearch(context: context)
        item.searchType = type.rawValue
        item.text = text
        item.websiteUrl = websiteUrl
        item.dateAdded = Date()
      }
    }
  }

  public static func getItem(text: String?, websiteUrl: String?) -> RecentSearch? {
    // A search engine search has both set
    // Because it is a query text against a search engine url
    if let text = text, let websiteUrl = websiteUrl {
      return RecentSearch.first(where: NSPredicate(format: "text == %@ AND websiteUrl == %@", text, websiteUrl))
    } else if let text = text {
      // Just text
      return RecentSearch.first(where: NSPredicate(format: "text == %@", text))
    } else if let websiteUrl = websiteUrl {
      // Just a website
      return RecentSearch.first(where: NSPredicate(format: "websiteUrl == %@", websiteUrl))
    }
    return nil
  }

  public class func get(with objectID: NSManagedObjectID) -> RecentSearch? {
    DataController.viewContext.object(with: objectID) as? RecentSearch
  }

  public static func removeItem(query: String) {
    RecentSearch.deleteAll(predicate: NSPredicate(format: "text == %@ OR websiteUrl == %@", query, query), context: .new(inMemory: false), includesPropertyValues: false)
  }

  public static func removeAll() {
    RecentSearch.deleteAll()
  }

  public static func totalCount() -> Int {
    let request = getFetchRequest()

    do {
      return try DataController.viewContext.count(for: request)
    } catch {
      Logger.module.error("Count error: \(error.localizedDescription)")
    }
    return 0
  }

  @nonobjc
  private class func fetchRequest() -> NSFetchRequest<RecentSearch> {
    NSFetchRequest<RecentSearch>(entityName: "RecentSearch")
  }

  private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
    NSEntityDescription.entity(forEntityName: "RecentSearch", in: context)!
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
