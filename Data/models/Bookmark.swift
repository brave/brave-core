/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */


import UIKit
import CoreData
import Foundation
import Shared
import Storage
import BraveShared

private let log = Logger.browserLogger

public class Bookmark: NSManagedObject, WebsitePresentable, Syncable {

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
    @NSManaged public var color: String?
    
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
        created = Date()
        lastVisited = created
    }
    
    public func asDictionary(deviceId: [Int]?, action: Int?) -> [String: Any] {
        return SyncBookmark(record: self, deviceId: deviceId, action: action).dictionaryRepresentation()
    }

    public class func frc(parentFolder: Bookmark?) -> NSFetchedResultsController<NSFetchRequestResult> {
        let context = DataController.mainThreadContext
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        
        fetchRequest.entity = Bookmark.entity(context: context)
        fetchRequest.fetchBatchSize = 20

        let orderSort = NSSortDescriptor(key:"order", ascending: true)
        let folderSort = NSSortDescriptor(key:"isFolder", ascending: false)
        let createdSort = NSSortDescriptor(key:"created", ascending: true)
        fetchRequest.sortDescriptors = [orderSort, folderSort, createdSort]

        if let parentFolder = parentFolder {
            fetchRequest.predicate = NSPredicate(format: "parentFolder == %@ AND isFavorite == NO", parentFolder)
        } else {
            fetchRequest.predicate = NSPredicate(format: "parentFolder == nil AND isFavorite == NO")
        }

        return NSFetchedResultsController(fetchRequest: fetchRequest, managedObjectContext:context,
                                          sectionNameKeyPath: nil, cacheName: nil)
    }
    
    // Syncable
    public func update(syncRecord record: SyncRecord?) {
        guard let bookmark = record as? SyncBookmark, let site = bookmark.site else { return }
        title = site.title
        update(customTitle: site.customTitle, url: site.location)
        lastVisited = Date(timeIntervalSince1970:(Double(site.lastAccessedTime ?? 0) / 1000.0))
        syncParentUUID = bookmark.parentFolderObjectId
        // No auto-save, must be handled by caller if desired
    }
    
    public func update(customTitle: String?, url: String?, save: Bool = false) {
        
        // See if there has been any change
        if self.customTitle == customTitle && self.url == url {
            return
        }
        
        if let ct = customTitle, !ct.isEmpty {
            self.customTitle = customTitle
        }
        
        if let u = url, !u.isEmpty {
            self.url = url
            if let theURL = URL(string: u), let context = managedObjectContext {
                domain = Domain.getOrCreateForUrl(theURL, context: context)
            } else {
                domain = nil
            }
        }
        
        if save {
            DataController.saveContext(context: self.managedObjectContext)
        }
        
        if !isFavorite {
            Sync.shared.sendSyncRecords(action: .update, records: [self])
        }
    }

    @discardableResult public static func add(rootObject root: SyncRecord?, save: Bool, sendToSync: Bool, context: NSManagedObjectContext) -> Syncable? {
        // Explicit parentFolder to force method decision
        return add(rootObject: root as? SyncBookmark, save: save, sendToSync: sendToSync, parentFolder: nil, context: context)
    }
    
    // Should not be used for updating, modify to increase protection
    class func add(rootObject root: SyncBookmark?, save: Bool = false, sendToSync: Bool = false, parentFolder: Bookmark? = nil, color: UIColor? = nil, context: NSManagedObjectContext) -> Bookmark? {
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
        // Should probably have visual indication before reaching this point
//        if site?.location?.startsWith(WebServer.sharedInstance.base) ?? false {
//            return nil
//        }
        
        // Use new values, fallback to previous values
        bk.url = site?.location ?? bk.url
        bk.title = site?.title ?? bk.title
        bk.color = (color ?? BraveUX.GreyE).toHexString()
        bk.customTitle = site?.customTitle ?? bk.customTitle // TODO: Check against empty titles
        bk.isFavorite = bookmark?.isFavorite ?? bk.isFavorite
        bk.isFolder = bookmark?.isFolder ?? bk.isFolder
        bk.syncUUID = root?.objectId ?? bk.syncUUID ?? SyncCrypto.shared.uniqueSerialBytes(count: 16)
        
        if let location = site?.location, let url = URL(string: location) {
            bk.domain = Domain.getOrCreateForUrl(url, context: context)
        }
        
        // Must assign both, in cae parentFolder does not exist, need syncParentUUID to attach later
        bk.parentFolder = parentFolder
        bk.syncParentUUID = bookmark?.parentFolderObjectId ?? bk.syncParentUUID

        // For folders that are saved _with_ a syncUUID, there may be child bookmarks
        //  (e.g. sync sent down bookmark before parent folder)
        if bk.isFolder {
            // Find all children and attach them
            if let children = Bookmark.getChildren(forFolderUUID: bk.syncUUID, context: context) {
                
                // TODO: Setup via bk.children property instead
                children.forEach { $0.parentFolder = bk }
            }
        }
        
        if save {
            DataController.saveContext(context: context)
        }
        
        if sendToSync && !bk.isFavorite {
            // Submit to server
            Sync.shared.sendSyncRecords(action: .create, records: [bk])
        }
        
        return bk
    }
  
    // TODO: DELETE
    // Aways uses main context
    @discardableResult public class func add(url: URL?,
                       title: String?,
                       customTitle: String? = nil, // Folders only use customTitle
                       parentFolder:Bookmark? = nil,
                       isFolder: Bool = false,
                       isFavorite: Bool = false,
                       color: UIColor? = nil) -> Bookmark? {
        
        let site = SyncSite()
        site.title = title
        site.customTitle = customTitle
        site.location = url?.absoluteString
        
        let bookmark = SyncBookmark()
        bookmark.isFavorite = isFavorite
        bookmark.isFolder = isFolder
        bookmark.parentFolderObjectId = parentFolder?.syncUUID
        bookmark.site = site
        
        let context = isFavorite ? DataController.mainThreadContext : DataController.workerThreadContext
        
        // Fetching bookmarks happen on mainThreadContext but we add it on worker context to work around the 
        // duplicated bookmarks bug.
        // To avoid CoreData crashes we get the parent folder on worker context via its objectID.
        // Favorites can't be nested, this is only relevant for bookmarks.
        var folderOnWorkerContext: Bookmark?
        if let folder = parentFolder {
            folderOnWorkerContext = (try? context.existingObject(with: folder.objectID)) as? Bookmark
        } 
        
        // Using worker context here, this propogates up, and merged into main.
        // There is some odd issue with duplicates when using main thread
        return self.add(rootObject: bookmark, save: true, sendToSync: true, parentFolder: folderOnWorkerContext, color: color, context: context)
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
        
        return self.add(rootObject: bookmark, save: true, context: DataController.workerThreadContext)
    }

    public class func contains(url: URL, getFavorites: Bool = false, context: NSManagedObjectContext) -> Bool {
        var found = false
        context.performAndWait {
            if let count = get(forUrl: url, countOnly: true, getFavorites: getFavorites, context: context) as? Int {
                found = count > 0
            }
        }
        return found
    }

    public class func frecencyQuery(context: NSManagedObjectContext, containing: String?) -> [Bookmark] {
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        fetchRequest.fetchLimit = 5
        fetchRequest.entity = Bookmark.entity(context: context)
        
        var predicate = NSPredicate(format: "lastVisited > %@", History.ThisWeek as CVarArg)
        if let query = containing {
            predicate = NSPredicate(format: predicate.predicateFormat + " AND url CONTAINS %@", query)
        }
        fetchRequest.predicate = predicate

        do {
            if let results = try context.fetch(fetchRequest) as? [Bookmark] {
                return results
            }
        } catch {
            let fetchError = error as NSError
            print(fetchError)
        }
        return [Bookmark]()
    }

    public class func reorderBookmarks(frc: NSFetchedResultsController<NSFetchRequestResult>?, sourceIndexPath: IndexPath,
                                destinationIndexPath: IndexPath) {
        guard let frc = frc else { return }
        
        let dest = frc.object(at: destinationIndexPath) as! Bookmark
        let src = frc.object(at: sourceIndexPath) as! Bookmark
        
        if dest === src {
            return
        }
        
        // Warning, this could be a bottleneck, grabs ALL the bookmarks in the current folder
        // But realistically, with a batch size of 20, and most reads around 1ms, a bottleneck here is an edge case.
        // Optionally: grab the parent folder, and the on a bg thread iterate the bms and update their order. Seems like overkill.
        var bms = frc.fetchedObjects as! [Bookmark]
        bms.remove(at: bms.index(of: src)!)
        if sourceIndexPath.row > destinationIndexPath.row {
            // insert before
            bms.insert(src, at: bms.index(of: dest)!)
        } else {
            let end = bms.index(of: dest)! + 1
            bms.insert(src, at: end)
        }
        
        for i in 0..<bms.count {
            bms[i].order = Int16(i)
        }
        
        // I am stumped, I can't find the notification that animation is complete for moving.
        // If I save while the animation is happening, the rows look screwed up (draw on top of each other).
        // Adding a delay to let animation complete avoids this problem
        DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(250)) {
            DataController.saveContext(context: frc.managedObjectContext)
        }

    }
}

// TODO: Document well
// Getters
extension Bookmark {
    fileprivate static func get(forUrl url: URL, countOnly: Bool = false, getFavorites: Bool = false, context: NSManagedObjectContext) -> AnyObject? {
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        fetchRequest.entity = Bookmark.entity(context: context)
        let isFavoritePredicate = NSNumber(value: getFavorites)
        fetchRequest.predicate = NSPredicate(format: "url == %@ AND isFavorite == \(isFavoritePredicate)", url.absoluteString)
        do {
            if countOnly {
                let count = try context.count(for: fetchRequest)
                return count as AnyObject
            }
            let results = try context.fetch(fetchRequest) as? [Bookmark]
            return results?.first
        } catch {
            let fetchError = error as NSError
            print(fetchError)
        }
        return nil
    }
    
    public static func getChildren(forFolderUUID syncUUID: [Int]?, ignoreFolders: Bool = false, context: NSManagedObjectContext,
                            orderSort: Bool = false) -> [Bookmark]? {
        guard let searchableUUID = SyncHelpers.syncDisplay(fromUUID: syncUUID) else {
            return nil
        }

        // New bookmarks are added with order 0, we are looking at created date then
        let sortRules = [NSSortDescriptor(key:"order", ascending: true), NSSortDescriptor(key:"created", ascending: false)]
        let sort = orderSort ? sortRules : nil
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        fetchRequest.entity = Bookmark.entity(context: context)
        fetchRequest.predicate =  NSPredicate(format: "syncParentDisplayUUID == %@ and isFolder == %@", searchableUUID, NSNumber(value: ignoreFolders))
        fetchRequest.sortDescriptors = sort
        
        do {
            let results = try context.fetch(fetchRequest) as? [Bookmark]
            return results
        } catch {
            let fetchError = error as NSError
            print(fetchError)
        }
        return nil
    }
    
    static func get(parentSyncUUID parentUUID: [Int]?, context: NSManagedObjectContext?) -> Bookmark? {
        guard let searchableUUID = SyncHelpers.syncDisplay(fromUUID: parentUUID) else {
            return nil
        }
        
        return get(predicate: NSPredicate(format: "syncDisplayUUID == %@", searchableUUID), context: context)?.first
    }
    
    public static func getFolders(bookmark: Bookmark?, context: NSManagedObjectContext) -> [Bookmark] {
    
        var predicate: NSPredicate?
        if let parent = bookmark?.parentFolder {
            predicate = NSPredicate(format: "isFolder == true and parentFolder == %@", parent)
        } else {
            predicate = NSPredicate(format: "isFolder == true and parentFolder = nil")
        }
        
        return get(predicate: predicate, context: context) ?? [Bookmark]()
    }
    
    // TODO: Remove
    static func getAllBookmarks(context: NSManagedObjectContext) -> [Bookmark] {
        return get(predicate: NSPredicate(format: "isFavorite == NO"), context: context) ?? [Bookmark]()
    }
}

// TODO: REMOVE!! This should be located in abstraction
extension Bookmark {
    @discardableResult
    public class func remove(forUrl url: URL, save: Bool = true, context: NSManagedObjectContext) -> Bool {
        if let bm = get(forUrl: url, context: context) as? Bookmark {
            bm.remove(save: save)
            return true
        }
        return false
    }
    
    /** Removes all bookmarks. Used to reset state for bookmark UITests */
    class func removeAll() {
        let context = DataController.workerThreadContext
        
        self.getAllBookmarks(context: context).forEach {
            $0.remove(save: false)
        }
        
        DataController.saveContext(context: context)
    }
}

