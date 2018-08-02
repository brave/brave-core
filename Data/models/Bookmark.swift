/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */


import UIKit
import CoreData
import Foundation
import Shared
import Storage
import BraveShared

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
        let context = DataController.viewContext
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
            DataController.save(context: self.managedObjectContext)
        }
        
        if !isFavorite {
            Sync.shared.sendSyncRecords(action: .update, records: [self])
        }
    }

    public static func add(rootObject root: SyncRecord?, save: Bool, sendToSync: Bool, context: NSManagedObjectContext) -> Syncable? {
        add(rootObject: root as? SyncBookmark, save: save, sendToSync: sendToSync, parentFolder: nil)
        
        // TODO: Saving is done asynchronously, we should return a completion handler. 
        // Will probably need a refactor in Syncable protocol.
        // As for now, the return value for adding bookmark is never used.
        return nil
    }
    
    // Should not be used for updating, modify to increase protection
    class func add(rootObject root: SyncBookmark?, save: Bool = false, sendToSync: Bool = false, parentFolder: Bookmark? = nil, color: UIColor? = nil) {
        let context = DataController.backgroundContext
        
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
        bk.syncUUID = root?.objectId ?? bk.syncUUID ?? SyncCrypto.uniqueSerialBytes(count: 16)
        bk.created = site?.creationNativeDate ?? Date()
        bk.lastVisited = site?.lastAccessedNativeDate ?? Date()
        
        if let location = site?.location, let url = URL(string: location) {
            bk.domain = Domain.getOrCreateForUrl(url, context: context)
        }
        
        // This also sets up a parent folder
        bk.syncParentUUID = bookmark?.parentFolderObjectId ?? bk.syncParentUUID
        
        // For folders that are saved _with_ a syncUUID, there may be child bookmarks
        //  (e.g. sync sent down bookmark before parent folder)
        if bk.isFolder {
            // Find all children and attach them
            if let children = Bookmark.getChildren(forFolderUUID: bk.syncUUID) {
                
                // TODO: Setup via bk.children property instead
                children.forEach { $0.syncParentUUID = bk.syncParentUUID }
            }
        }
        
        if save {
            DataController.save(context: context)
        }
        
        if sendToSync && !bk.isFavorite {
            // Submit to server, must be on main thread
            Sync.shared.sendSyncRecords(action: .create, records: [bk])
        }
    }
  
    public class func add(url: URL?,
                       title: String?,
                       customTitle: String? = nil, // Folders only use customTitle
                       parentFolder:Bookmark? = nil,
                       isFolder: Bool = false,
                       isFavorite: Bool = false,
                       color: UIColor? = nil) {
        
        let site = SyncSite()
        site.title = title
        site.customTitle = customTitle
        site.location = url?.absoluteString
        
        let bookmark = SyncBookmark()
        bookmark.isFavorite = isFavorite
        bookmark.isFolder = isFolder
        bookmark.parentFolderObjectId = parentFolder?.syncUUID
        bookmark.site = site
        
        add(rootObject: bookmark, save: true, sendToSync: true, parentFolder: parentFolder, color: color)
    }

    public class func contains(url: URL, getFavorites: Bool = false) -> Bool {
        guard let count = count(forUrl: url, getFavorites: getFavorites) else { return false } 
        return count > 0
    }

    public class func reorderBookmarks(frc: NSFetchedResultsController<NSFetchRequestResult>?, sourceIndexPath: IndexPath,
                                destinationIndexPath: IndexPath) {
        let dest = frc?.object(at: destinationIndexPath) as! Bookmark
        let src = frc?.object(at: sourceIndexPath) as! Bookmark
        
        if dest === src {
            return
        }
        
        // Warning, this could be a bottleneck, grabs ALL the bookmarks in the current folder
        // But realistically, with a batch size of 20, and most reads around 1ms, a bottleneck here is an edge case.
        // Optionally: grab the parent folder, and the on a bg thread iterate the bms and update their order. Seems like overkill.
        var bms = frc?.fetchedObjects as! [Bookmark]
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
            DataController.save(context: frc?.managedObjectContext)
        }

    }
}

// TODO: Document well
// MARK: - Getters
extension Bookmark {
    fileprivate static func get(forUrl url: URL, countOnly: Bool = false, getFavorites: Bool = false, context: NSManagedObjectContext) -> [Bookmark]? {
        let urlKeyPath = #keyPath(Bookmark.url)
        let isFavoriteKeyPath = #keyPath(Bookmark.isFavorite)
        
        let isFavoritePredicate = getFavorites ? "YES" : "NO"
        let predicate = NSPredicate(format: "\(urlKeyPath) == %@ AND \(isFavoriteKeyPath) == \(isFavoritePredicate)", url.absoluteString)
        
        return all(where: predicate)
    }
    
    fileprivate static func count(forUrl url: URL, getFavorites: Bool = false) -> Int? {
        let urlKeyPath = #keyPath(Bookmark.url)
        let isFavoriteKeyPath = #keyPath(Bookmark.isFavorite)
        
        let isFavoritePredicate = getFavorites ? "YES" : "NO"
        let predicate = NSPredicate(format: "\(urlKeyPath) == %@ AND \(isFavoriteKeyPath) == \(isFavoritePredicate)", url.absoluteString)
        
        return count(predicate: predicate)
    }
    
    public static func getChildren(forFolderUUID syncUUID: [Int]?, ignoreFolders: Bool = false) -> [Bookmark]? {
        guard let searchableUUID = SyncHelpers.syncDisplay(fromUUID: syncUUID) else {
            return nil
        }
        
        let syncParentDisplayUUIDKeyPath = #keyPath(Bookmark.syncParentDisplayUUID)
        let isFolderKeyPath = #keyPath(Bookmark.isFolder)
                
        let predicate = NSPredicate(format: "\(syncParentDisplayUUIDKeyPath) == %@ AND \(isFolderKeyPath) == %@",  
                                    searchableUUID, ignoreFolders ? "YES" : "NO")
        
        return all(where: predicate)
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
            predicate = NSPredicate(format: "isFolder == true and parentFolder.@count = 0")
        }
        
        return all(where: predicate) ?? []
    }
    
    static func getAllBookmarks(context: NSManagedObjectContext) -> [Bookmark] {
        let predicate = NSPredicate(format: "isFavorite == NO")
        
        return all(where: predicate) ?? []
    }
}

// TODO: REMOVE!! This should be located in abstraction
extension Bookmark {
    @discardableResult
    public class func remove(forUrl url: URL, save: Bool = true, context: NSManagedObjectContext) -> Bool {
        if let bm = get(forUrl: url, context: context)?.first {
            bm.remove(save: save)
            return true
        }
        return false
    }
}

