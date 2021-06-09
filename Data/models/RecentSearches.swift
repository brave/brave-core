// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared

private let log = Logger.browserLogger

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
        
        return NSFetchedResultsController(fetchRequest: fetchRequest, managedObjectContext: context,
                                          sectionNameKeyPath: nil, cacheName: nil)
    }
    
    public static func addItem(type: RecentSearchType, text: String?, websiteUrl: String?) {
        DataController.perform(context: .new(inMemory: false), save: true) { context in
            let item = RecentSearch(context: context)
            item.searchType = type.rawValue
            item.text = text
            item.websiteUrl = websiteUrl
            item.dateAdded = Date()
        }
    }
    
    public static func getItem(text: String) -> RecentSearch? {
        return RecentSearch.first(where: NSPredicate(format: "text == %@", text))
    }
    
    public static func itemExists(text: String) -> Bool {
        return getItem(text: text) != nil
    }
    
    public static func removeItem(text: String) {
        RecentSearch.deleteAll(predicate: NSPredicate(format: "text == %@", text), context: .new(inMemory: false), includesPropertyValues: false)
    }
    
    public static func removeAll() {
        RecentSearch.deleteAll()
    }
    
    public static func totalCount() -> Int {
        let request = getFetchRequest()
        
        do {
            return try DataController.viewContext.count(for: request)
        } catch {
            log.error("Count error: \(error)")
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
            log.warning("Writing to view context, this should be avoided.")
        }
        
        if context.hasChanges {
            do {
                try context.save()
            } catch {
                assertionFailure("Error saving DB: \(error)")
            }
        }
    }
}
