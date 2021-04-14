// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared

private let log = Logger.browserLogger

@objc(PlaylistItem)
final public class PlaylistItem: NSManagedObject, CRUD {
    @NSManaged public var cachedData: Data?
    @NSManaged public var dateAdded: Date?
    @NSManaged public var duration: Float
    @NSManaged public var mediaSrc: String?
    @NSManaged public var mimeType: String?
    @NSManaged public var name: String?
    @NSManaged public var order: Int32
    @NSManaged public var pageSrc: String?
    @NSManaged public var pageTitle: String?
    
    public class func frc() -> NSFetchedResultsController<PlaylistItem> {
        let context = DataController.viewContext
        let fetchRequest = NSFetchRequest<PlaylistItem>()
        fetchRequest.entity = PlaylistItem.entity(context)
        fetchRequest.fetchBatchSize = 20
        
        let orderSort = NSSortDescriptor(key: "order", ascending: true)
        let createdSort = NSSortDescriptor(key: "dateAdded", ascending: false)
        fetchRequest.sortDescriptors = [orderSort, createdSort]
        
        return NSFetchedResultsController(fetchRequest: fetchRequest, managedObjectContext: context,
                                          sectionNameKeyPath: nil, cacheName: nil)
    }
    
    public static func addItem(_ item: PlaylistInfo, cachedData: Data?, completion: (() -> Void)? = nil) {
        DataController.perform(context: .new(inMemory: false), save: false) { context in
            let playlistItem = PlaylistItem(context: context)
            playlistItem.name = item.name
            playlistItem.pageTitle = item.pageTitle
            playlistItem.pageSrc = item.pageSrc
            playlistItem.dateAdded = Date()
            playlistItem.cachedData = cachedData ?? Data()
            playlistItem.duration = item.duration
            playlistItem.mimeType = item.mimeType
            playlistItem.mediaSrc = item.src
            playlistItem.order = -9999
            
            PlaylistItem.saveContext(context)
            PlaylistItem.reorderItems(context: context)
            PlaylistItem.saveContext(context)
            
            DispatchQueue.main.async {
                completion?()
            }
        }
    }
    
    public static func getItem(pageSrc: String) -> PlaylistItem? {
        return PlaylistItem.first(where: NSPredicate(format: "pageSrc == %@", pageSrc))
    }
    
    public static func itemExists(_ item: PlaylistInfo) -> Bool {
        if let count = PlaylistItem.count(predicate: NSPredicate(format: "pageSrc == %@", item.pageSrc)), count > 0 {
            return true
        }
        return false
    }
    
    public static func updateItem(_ item: PlaylistInfo, completion: (() -> Void)? = nil) {
        if itemExists(item) {
            DataController.perform(context: .new(inMemory: false), save: false) { context in
                if let existingItem = PlaylistItem.first(where: NSPredicate(format: "pageSrc == %@", item.pageSrc), context: context) {
                    existingItem.name = item.name
                    existingItem.pageTitle = item.pageTitle
                    existingItem.pageSrc = item.pageSrc
                    existingItem.duration = item.duration
                    existingItem.mimeType = item.mimeType
                    existingItem.mediaSrc = item.src
                }
                
                PlaylistItem.saveContext(context)
                
                DispatchQueue.main.async {
                    completion?()
                }
            }
        } else {
            addItem(item, cachedData: nil, completion: completion)
        }
    }
    
    public static func updateCache(pageSrc: String, cachedData: Data?) {
        DataController.perform(context: .new(inMemory: false), save: true) { context in
            let item = PlaylistItem.first(where: NSPredicate(format: "pageSrc == %@", pageSrc), context: context)
            item?.cachedData = cachedData
        }
    }
    
    public static func removeItem(_ item: PlaylistInfo) {
        PlaylistItem.deleteAll(predicate: NSPredicate(format: "mediaSrc == %@", item.src), context: .new(inMemory: false), includesPropertyValues: false)
    }
    
    // MARK: - Internal
    private static func reorderItems(context: NSManagedObjectContext) {
        DataController.perform(context: .existing(context), save: true) { context in
            let request = NSFetchRequest<PlaylistItem>()
            request.entity = PlaylistItem.entity(context)
            request.fetchBatchSize = 20
            
            let orderSort = NSSortDescriptor(key: "order", ascending: true)
            let items = PlaylistItem.all(sortDescriptors: [orderSort], context: context) ?? []
            
            for (order, item) in items.enumerated() {
                item.order = Int32(order)
            }
        }
    }
    
    @nonobjc
    private class func fetchRequest() -> NSFetchRequest<PlaylistItem> {
        NSFetchRequest<PlaylistItem>(entityName: "PlaylistItem")
    }
    
    private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
        NSEntityDescription.entity(forEntityName: "PlaylistItem", in: context)!
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
