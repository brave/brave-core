/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Foundation
import Shared
import Storage

private let log = Logger.browserLogger

public final class Bookmark: NSManagedObject, WebsitePresentable, Syncable, CRUD {
    // Favorite bookmarks are shown only on homepanel as a tile, they are not visible on bookmarks panel.
    @NSManaged public var isFavorite: Bool
    @NSManaged public var isFolder: Bool
    @NSManaged public var title: String?
    @NSManaged public var customTitle: String?
    @NSManaged public var url: String?
    @NSManaged public var visits: Int32
    @NSManaged public var lastVisited: Date?
    @NSManaged public var created: Date?
    @NSManaged public var order: Int16
    @NSManaged public var tags: [String]?
    @NSManaged public var syncOrder: String?
    
    /// Should not be set directly, due to specific formatting required, use `syncUUID` instead
    /// CD does not allow (easily) searching on transformable properties, could use binary, but would still require tranformtion
    //  syncUUID should never change
    @NSManaged public var syncDisplayUUID: String?
    @NSManaged public var syncParentDisplayUUID: String?
    @NSManaged public var parentFolder: Bookmark?
    @NSManaged public var children: Set<Bookmark>?
    
    @NSManaged public var domain: Domain?
    
    public var recordType: SyncRecordType = .bookmark
    
    var syncParentUUID: [Int]? {
        get { return SyncHelpers.syncUUID(fromString: syncParentDisplayUUID) }
        set(value) {
            // Save actual instance variable
            syncParentDisplayUUID = SyncHelpers.syncDisplay(fromUUID: value)
            
            // Attach parent, only works if parent exists.
            let parent = Bookmark.get(parentSyncUUID: value, context: self.managedObjectContext)
            parentFolder = parent
        }
    }
    
    public var displayTitle: String? {
        if let custom = customTitle, !custom.isEmpty {
            return customTitle
        }
        
        if let t = title, !t.isEmpty {
            return title
        }
        
        // Want to return nil so less checking on frontend
        return nil
    }
    
    override public func awakeFromInsert() {
        super.awakeFromInsert()
        lastVisited = created
    }
    
    public class func frc(forFavorites: Bool = false, parentFolder: Bookmark?) -> NSFetchedResultsController<Bookmark> {
        let context = DataController.viewContext
        let fetchRequest = NSFetchRequest<Bookmark>()
        
        fetchRequest.entity = Bookmark.entity(context: context)
        fetchRequest.fetchBatchSize = 20
        
        let orderSort = NSSortDescriptor(key: "order", ascending: true)
        let createdSort = NSSortDescriptor(key: "created", ascending: false)
        fetchRequest.sortDescriptors = [orderSort, createdSort]
        
        fetchRequest.predicate = forFavorites ?
            NSPredicate(format: "isFavorite == YES") : allBookmarksOfAGivenLevelPredicate(parent: parentFolder)
        
        return NSFetchedResultsController(fetchRequest: fetchRequest, managedObjectContext: context,
                                          sectionNameKeyPath: nil, cacheName: nil)
    }
    
    public func update(customTitle: String?, url: String?, newSyncOrder: String? = nil, save: Bool = true,
                       sendToSync: Bool = true) {
        var contextToUpdate: NSManagedObjectContext?
        
        // Syncable.update() doesn't save to CD at the moment, we need to use managedObjectContext here.
        if save == false {
            contextToUpdate = managedObjectContext
            // Updated object usually uses view context, all database writes should happen on a background thread
            // so we need to fetch the object using background context.
        } else if managedObjectContext?.concurrencyType != .privateQueueConcurrencyType {
            contextToUpdate = DataController.newBackgroundContext()
        }
        
        guard let bookmarkToUpdate = (try? contextToUpdate?.existingObject(with: objectID)) as? Bookmark,
            let context = contextToUpdate else {
                return
        }
        
        // See if there has been any change
        if self.customTitle == customTitle && self.url == url && syncOrder == newSyncOrder {
            return
        }
        
        if let ct = customTitle, !ct.isEmpty {
            bookmarkToUpdate.customTitle = customTitle
        }
        
        if let u = url, !u.isEmpty {
            bookmarkToUpdate.url = url
            if let theURL = URL(string: u) {
                bookmarkToUpdate.domain = Domain.getOrCreateForUrl(theURL, context: context)
            } else {
                bookmarkToUpdate.domain = nil
            }
        }
        
        // Checking if syncOrder has changed is imporant here for performance reasons.
        // Currently to do bookmark sorting right, we have to grab all bookmarks in a given directory
        // and update their order which is a costly operation.
        if newSyncOrder != nil && syncOrder != newSyncOrder {
            syncOrder = newSyncOrder
            Bookmark.setOrderForAllBookmarksOnGivenLevel(parent: parentFolder, forFavorites: isFavorite, context: context)
        }
        
        if save {
            DataController.save(context: context)
        }
        
        if !isFavorite && sendToSync {
            Sync.shared.sendSyncRecords(action: .update, records: [bookmarkToUpdate])
        }
    }
    
    // Should not be used for updating, modify to increase protection
    private class func add(rootObject root: SyncBookmark?,
                           save: Bool = true,
                           sendToSync: Bool = true,
                           parentFolder: Bookmark? = nil,
                           context: NSManagedObjectContext = DataController.newBackgroundContext()) -> Bookmark? {
        
        let bookmark = root
        let site = bookmark?.site
        
        var bk: Bookmark!
        if let id = root?.objectId, let foundbks = Bookmark.get(syncUUIDs: [id], context: context) as? [Bookmark], let foundBK = foundbks.first {
            // Found a pre-existing bookmark, cannot add duplicate
            // Turn into 'update' record instead
            bk = foundBK
        } else {
            bk = Bookmark(entity: Bookmark.entity(context: context), insertInto: context)
        }
        
        // BRAVE TODO:
        // if site?.location?.startsWith(WebServer.sharedInstance.base) ?? false {
        //    return nil
        // }
        
        // Use new values, fallback to previous values
        bk.url = site?.location ?? bk.url
        bk.title = site?.title ?? bk.title
        bk.customTitle = site?.customTitle ?? bk.customTitle // TODO: Check against empty titles
        bk.isFavorite = bookmark?.isFavorite ?? bk.isFavorite
        bk.isFolder = bookmark?.isFolder ?? bk.isFolder
        bk.syncUUID = root?.objectId ?? bk.syncUUID ?? SyncCrypto.uniqueSerialBytes(count: 16)
        bk.syncOrder = root?.syncOrder
        bk.created = root?.syncNativeTimestamp ?? Date()
        
        if let location = site?.location, let url = URL(string: location) {
            bk.domain = Domain.getOrCreateForUrl(url, context: context, save: false)
        }
        
        // Update parent folder if one exists
        if let newParent = bookmark?.parentFolderObjectId {
            bk.syncParentUUID = newParent
        }
        
        if bk.syncOrder == nil {
            bk.newSyncOrder(forFavorites: bk.isFavorite, context: context)
        }
        
        // This also sets up a parent folder
        bk.syncParentUUID = bookmark?.parentFolderObjectId ?? bk.syncParentUUID
        
        // For folders that are saved _with_ a syncUUID, there may be child bookmarks
        //  (e.g. sync sent down bookmark before parent folder)
        if bk.isFolder {
            // Find all children and attach them
            if let children = Bookmark.getChildren(forFolderUUID: bk.syncUUID, context: context) {
                // Re-link all orphaned children
                children.forEach {
                    $0.syncParentUUID = bk.syncUUID
                    // The setter for syncParentUUID creates the parent/child relationship in CD, however in this specific instance
                    // the objects have not been written to disk, so cannot be fetched on a different context and the relationship
                    // will not be properly established. Manual attachment is necessary here during these batch additions.
                    $0.parentFolder = bk
                }
            }
        }
        
        setOrderForAllBookmarksOnGivenLevel(parent: bk.parentFolder, forFavorites: bk.isFavorite, context: context)
        
        if save {
            DataController.save(context: context)
        }
        
        if sendToSync && !bk.isFavorite {
            // Submit to server, must be on main thread
            Sync.shared.sendSyncRecords(action: .create, records: [bk])
        }
        
        return bk
    }
    
    class func allBookmarksOfAGivenLevelPredicate(parent: Bookmark?) -> NSPredicate {
        let isFavoriteKP = #keyPath(Bookmark.isFavorite)
        let parentFolderKP = #keyPath(Bookmark.parentFolder)
        
        // A bit hacky but you can't just pass 'nil' string to %@.
        let nilArgumentForPredicate = 0
        return NSPredicate(
            format: "%K == %@ AND %K == NO", parentFolderKP, parent ?? nilArgumentForPredicate, isFavoriteKP)
    }
    
    public class func add(from list: [(url: URL, title: String)]) {
        let context = DataController.newBackgroundContext()
        context.performAndWait {
            list.forEach { fav in
                Bookmark.add(url: fav.url, title: fav.title, isFavorite: true, save: false, context: context)
            }
            DataController.save(context: context)
        }
    }
    
    public class func add(url: URL?,
                          title: String?,
                          customTitle: String? = nil, // Folders only use customTitle
                          parentFolder: Bookmark? = nil,
                          isFolder: Bool = false,
                          isFavorite: Bool = false,
                          syncOrder: String? = nil,
                          save: Bool = true,
                          context: NSManagedObjectContext? = nil) {
        
        let site = SyncSite()
        site.title = title
        site.customTitle = customTitle
        site.location = url?.absoluteString
        
        let bookmark = SyncBookmark()
        bookmark.isFavorite = isFavorite
        bookmark.isFolder = isFolder
        bookmark.parentFolderObjectId = parentFolder?.syncUUID
        bookmark.site = site
        
        _ = add(rootObject: bookmark, save: save, parentFolder: parentFolder,
                context: context ?? DataController.newBackgroundContext())
    }
    
    public class func contains(url: URL, getFavorites: Bool = false) -> Bool {
        guard let count = count(forUrl: url, getFavorites: getFavorites) else { return false }
        return count > 0
    }
    
    /// Reordering bookmarks has two steps:
    /// 1. Sets new `syncOrder` for the source(moving) Bookmark
    /// 2. Recalculates `syncOrder` for all Bookmarks on a given level. This is required because
    /// we use a special String-based order and algorithg. Simple String comparision doesn't work here.
    public class func reorderBookmarks(frc: NSFetchedResultsController<Bookmark>?, sourceIndexPath: IndexPath,
                                       destinationIndexPath: IndexPath) {
        guard let frc = frc else { return }
        
        let dest = frc.object(at: destinationIndexPath)
        let src = frc.object(at: sourceIndexPath)
        
        if dest === src { return }
        
        let context = DataController.newBackgroundContext()
        // Note: sync order is also used for ordering favorites and non synchronized bookmarks.
        let srcUpdated = updateSyncOrderOfMovedBookmark(frc: frc, sourceBookmark: src,
                                                        destinationBookmark: dest,
                                                        sourceIndexPath: sourceIndexPath,
                                                        destinationIndexPath: destinationIndexPath,
                                                        context: context)
        
        setOrderForAllBookmarksOnGivenLevel(parent: src.parentFolder, forFavorites: src.isFavorite, context: context)
        
        DataController.save(context: context)
        
        if let bookmarkWithUpdatedSyncOrder = srcUpdated, !bookmarkWithUpdatedSyncOrder.isFavorite {
            Sync.shared.sendSyncRecords(action: .update, records: [bookmarkWithUpdatedSyncOrder])
        }
    }
    
    private class func updateSyncOrderOfMovedBookmark(frc: NSFetchedResultsController<Bookmark>,
                                                      sourceBookmark src: Bookmark,
                                                      destinationBookmark dest: Bookmark,
                                                      sourceIndexPath: IndexPath,
                                                      destinationIndexPath: IndexPath,
                                                      context: NSManagedObjectContext) -> Bookmark? {
        
        guard let srcBgContext = context.object(with: src.objectID) as? Bookmark,
            let destBgContext = context.object(with: dest.objectID) as? Bookmark else {
            return nil
        }
        
        // Depending on drag direction, all other bookmarks are pushed up or down.
        let isMovingUp = sourceIndexPath.row > destinationIndexPath.row
        
        var previousOrder: String?
        var nextOrder: String?
        
        if isMovingUp {
            let bookmarkMovedToTop = destinationIndexPath.row == 0
            // A Bookmark that is moved to top has no previous bookmark.
            if !bookmarkMovedToTop {
                let index = IndexPath(row: destinationIndexPath.row - 1, section: destinationIndexPath.section)
                let objectAtIndexOnBgContext = context.object(with: frc.object(at: index).objectID) as? Bookmark
                previousOrder = objectAtIndexOnBgContext?.syncOrder
            }
            
            nextOrder = destBgContext.syncOrder
        } else {
            previousOrder = destBgContext.syncOrder
            
            // A Bookmark that is moved to bottom has no next bookmark.
            if let objects = frc.fetchedObjects, destinationIndexPath.row + 1 < objects.count {
                let index = IndexPath(row: destinationIndexPath.row + 1, section: destinationIndexPath.section)
                let objectAtIndexOnBgContext = context.object(with: frc.object(at: index).objectID) as? Bookmark
                nextOrder = objectAtIndexOnBgContext?.syncOrder
            }
        }
        
        srcBgContext.syncOrder = Sync.shared.getBookmarkOrder(previousOrder: previousOrder, nextOrder: nextOrder)
        return srcBgContext
    }
    
    /// Takes all Bookmarks and Favorites from 1.6 and sets correct order for them.
    /// 1.6 had few bugs with reordering which we want to avoid, in particular non-reordered bookmarks on 1.6
    /// all have order set to 0 which makes sorting confusing.
    /// In migration we take all bookmarks using the same sorting method as on 1.6 and add a proper `order`
    /// attribute to them. The goal is to have all bookmarks with a proper unique order number set.
    public class func migrateOrder(parentFolder: Bookmark? = nil,
                                   forFavorites: Bool,
                                   context: NSManagedObjectContext = DataController.newBackgroundContext()) {
        
        let predicate = forFavorites ?
            NSPredicate(format: "isFavorite == true") : allBookmarksOfAGivenLevelPredicate(parent: parentFolder)
        
        let orderSort = NSSortDescriptor(key: #keyPath(Bookmark.order), ascending: true)
        let folderSort = NSSortDescriptor(key: #keyPath(Bookmark.isFolder), ascending: false)
        let createdSort = NSSortDescriptor(key: #keyPath(Bookmark.created), ascending: true)
        
        let sort = [orderSort, folderSort, createdSort]
        
        guard let allBookmarks = all(where: predicate, sortDescriptors: sort, context: context),
              !allBookmarks.isEmpty else {
            return
        }
        
        for (i, bookmark) in allBookmarks.enumerated() {
            bookmark.order = Int16(i)
            // Calling this method recursively to get ordering for nested bookmarks
            if !forFavorites && bookmark.isFolder {
                migrateOrder(parentFolder: bookmark, forFavorites: forFavorites, context: context)
            }
        }
        
        DataController.save(context: context)
    }
    
    // TODO: Migration syncUUIDS still needs to be solved
    // Should only ever be used for migration from old db
    // Always uses worker context
    class func addForMigration(url: String?, title: String, customTitle: String, parentFolder: Bookmark?, isFolder: Bool?) -> Bookmark? {
        
        let site = SyncSite()
        site.title = title
        site.customTitle = customTitle
        site.location = url
        
        let bookmark = SyncBookmark()
        bookmark.isFolder = isFolder
        // bookmark.parentFolderObjectId = [parentFolder]
        bookmark.site = site
        
        return self.add(rootObject: bookmark)
    }
    
    public func delete(save: Bool = true, sendToSync: Bool = true, context: NSManagedObjectContext? = nil) {
        func deleteFromStore() {
            let context = context ?? DataController.newBackgroundContext()
            
            do {
                let objectOnContext = try context.existingObject(with: self.objectID)
                context.delete(objectOnContext)
                if save { DataController.save(context: context)}
            } catch {
                log.warning("Could not find object: \(self) on a background context.")
            }
        }
        
        if isFavorite { deleteFromStore() }
        
        if sendToSync {
            // Before we delete a folder and its children, we need to grab all children bookmarks
            // and send them to sync with `delete` action.
            if isFolder {
                removeFolderAndSendSyncRecords(uuid: syncUUID)
            } else {
                Sync.shared.sendSyncRecords(action: .delete, records: [self])
            }
        }
        
        deleteFromStore()
    }
    
    /// Removes a single Bookmark of a given URL.
    /// In case of having two bookmarks with the same url, a bookmark to delete is chosen randomly.
    public class func remove(forUrl url: URL) {
        let context = DataController.newBackgroundContext()
        let predicate = isFavoriteOrBookmarkByUrlPredicate(url: url, getFavorites: false)
        
        let record = first(where: predicate, context: context)
        record?.delete()
    }
    
    private func removeFolderAndSendSyncRecords(uuid: [Int]?) {
        if !isFolder { return }
        
        var allBookmarks = [Bookmark]()
        allBookmarks.append(self)
        
        if let allNestedBookmarks = Bookmark.getRecursiveChildren(forFolderUUID: syncUUID) {
            log.warning("All nested bookmarks of :\(String(describing: title)) folder is nil")
            
            allBookmarks.append(contentsOf: allNestedBookmarks)
        }
        
        Sync.shared.sendSyncRecords(action: .delete, records: allBookmarks)
    }
}

// TODO: Document well
// MARK: - Getters
extension Bookmark {
    fileprivate static func count(forUrl url: URL, getFavorites: Bool = false) -> Int? {
        let predicate = isFavoriteOrBookmarkByUrlPredicate(url: url, getFavorites: getFavorites)
        return count(predicate: predicate)
    }
    
    private static func isFavoriteOrBookmarkByUrlPredicate(url: URL, getFavorites: Bool) -> NSPredicate {
        let urlKeyPath = #keyPath(Bookmark.url)
        let isFavoriteKeyPath = #keyPath(Bookmark.isFavorite)
        
        return NSPredicate(format: "\(urlKeyPath) == %@ AND \(isFavoriteKeyPath) == \(NSNumber(value: getFavorites))", url.absoluteString)
    }
    
    public static func getChildren(forFolderUUID syncUUID: [Int]?, includeFolders: Bool = true,
                                   context: NSManagedObjectContext = DataController.viewContext) -> [Bookmark]? {
        guard let searchableUUID = SyncHelpers.syncDisplay(fromUUID: syncUUID) else {
            return nil
        }
        
        let syncParentDisplayUUIDKeyPath = #keyPath(Bookmark.syncParentDisplayUUID)
        let isFolderKeyPath = #keyPath(Bookmark.isFolder)
        
        var query = "\(syncParentDisplayUUIDKeyPath) == %@"
        
        if !includeFolders {
            query += " AND \(isFolderKeyPath) == false"
        }
        
        let predicate = NSPredicate(format: query, searchableUUID)
        
        return all(where: predicate, context: context)
    }
    
    static func get(parentSyncUUID parentUUID: [Int]?, context: NSManagedObjectContext?) -> Bookmark? {
        guard let searchableUUID = SyncHelpers.syncDisplay(fromUUID: parentUUID), let context = context else {
            return nil
        }
        
        let predicate = NSPredicate(format: "syncDisplayUUID == %@", searchableUUID)
        return first(where: predicate, context: context)
    }
    
    public static func getFolders(bookmark: Bookmark?, context: NSManagedObjectContext) -> [Bookmark] {
        var predicate: NSPredicate?
        if let parent = bookmark?.parentFolder {
            predicate = NSPredicate(format: "isFolder == true and parentFolder == %@", parent)
        } else {
            predicate = NSPredicate(format: "isFolder == true and parentFolder = nil")
        }
        
        return all(where: predicate) ?? []
    }
    
    static func getAllBookmarks(context: NSManagedObjectContext) -> [Bookmark] {
        let predicate = NSPredicate(format: "isFavorite == NO")
        
        return all(where: predicate) ?? []
    }
    
    /// Gets all nested bookmarks recursively.
    public static func getRecursiveChildren(forFolderUUID syncUUID: [Int]?,
                                            context: NSManagedObjectContext = DataController.viewContext) -> [Bookmark]? {
        guard let searchableUUID = SyncHelpers.syncDisplay(fromUUID: syncUUID) else {
            return nil
        }
        
        let syncParentDisplayUUIDKeyPath = #keyPath(Bookmark.syncParentDisplayUUID)
        
        let predicate = NSPredicate(format: "\(syncParentDisplayUUIDKeyPath) == %@", searchableUUID)
        
        var allBookmarks = [Bookmark]()
        
        let result = all(where: predicate, context: context)
        
        result?.forEach {
            allBookmarks.append($0)
            
            if $0.isFolder {
                if let nestedBookmarks = getRecursiveChildren(forFolderUUID: $0.syncUUID) {
                    allBookmarks.append(contentsOf: nestedBookmarks)
                }
            }
        }
        
        return allBookmarks
    }
    
    public class func frecencyQuery(context: NSManagedObjectContext, containing: String?) -> [Bookmark] {
        let fetchRequest = NSFetchRequest<Bookmark>()
        fetchRequest.fetchLimit = 5
        fetchRequest.entity = Bookmark.entity(context: context)
        
        var predicate = NSPredicate(format: "lastVisited > %@", History.ThisWeek as CVarArg)
        if let query = containing {
            predicate = NSPredicate(format: predicate.predicateFormat + " AND url CONTAINS %@", query)
        }
        fetchRequest.predicate = predicate
        
        do {
            return try context.fetch(fetchRequest)
        } catch {
            log.error(error)
        }
        return [Bookmark]()
    }
}


// MARK: - Syncable methods
extension Bookmark {
    public static func createResolvedRecord(rootObject root: SyncRecord?, save: Bool,
                                            context: NSManagedObjectContext) {
        add(rootObject: root as? SyncBookmark,
            save: save,
            sendToSync: false,
            context: context)
        
        // TODO: Saving is done asynchronously, we should return a completion handler.
        // Will probably need a refactor in Syncable protocol.
        // As for now, the return value for adding bookmark is never used.
    }

    public func updateResolvedRecord(_ record: SyncRecord?) {
        guard let bookmark = record as? SyncBookmark, let site = bookmark.site else { return }
        title = site.title
        update(customTitle: site.customTitle, url: site.location,
               newSyncOrder: bookmark.syncOrder, save: false, sendToSync: false)
        lastVisited = Date(timeIntervalSince1970: (Double(site.lastAccessedTime ?? 0) / 1000.0))
        syncParentUUID = bookmark.parentFolderObjectId
        created = record?.syncNativeTimestamp
        // No auto-save, must be handled by caller if desired
    }
    
    public func deleteResolvedRecord(save: Bool, context: NSManagedObjectContext?) {
        delete(save: save, sendToSync: false, context: context)
    }
    
    public func asDictionary(deviceId: [Int]?, action: Int?) -> [String: Any] {
        return SyncBookmark(record: self, deviceId: deviceId, action: action).dictionaryRepresentation()
    }
}

// MARK: - Comparable
extension Bookmark: Comparable {
    // Please note that for equality check `syncUUID` is used
    // but for checking if a Bookmark is less/greater than another Bookmark we check using `syncOrder`
    public static func == (lhs: Bookmark, rhs: Bookmark) -> Bool {
        return lhs.syncUUID == rhs.syncUUID
    }
    
    public static func < (lhs: Bookmark, rhs: Bookmark) -> Bool {
        return lhs.compare(rhs) == .orderedAscending
    }
    
    private func compare(_ rhs: Bookmark) -> ComparisonResult {
        
        guard let lhsSyncOrder = syncOrder, let rhsSyncOrder = rhs.syncOrder else {
            log.info("""
                Wanting to compare bookmark: \(String(describing: displayTitle)) \
                and \(String(describing: rhs.displayTitle)) but no syncOrder is set \
                in at least one of them.
                """)
            return .orderedSame
        }
        
        // Split is O(n)
        let lhsSyncOrderBits = lhsSyncOrder.split(separator: ".").compactMap { Int($0) }
        let rhsSyncOrderBits = rhsSyncOrder.split(separator: ".").compactMap { Int($0) }
        
        // Preventing going out of bounds.
        for i in 0..<min(lhsSyncOrderBits.count, rhsSyncOrderBits.count) {
            let comparison = lhsSyncOrderBits[i].compare(rhsSyncOrderBits[i])
            if comparison != .orderedSame { return comparison }
        }
        
        // We went through all numbers and everything is equal.
        // Need to check if one of arrays has more numbers because 0.0.1.1 > 0.0.1
        //
        // Alternatively, we could append zeros to make int arrays between the two objects
        // have same length. 0.0.1 vs 0.0.1.2 would convert to 0.0.1.0 vs 0.0.1.2
        return lhsSyncOrderBits.count.compare(rhsSyncOrderBits.count)
    }
}

extension Int {
    func compare(_ against: Int) -> ComparisonResult {
        if self > against { return .orderedDescending }
        if self < against { return .orderedAscending }
        return .orderedSame
    }
}
