/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import CoreData
import Shared
import JavaScriptCore

private let log = Logger.browserLogger

// Sync related methods for Bookmark model.
extension Bookmark {
    /// Sets order for all bookmarks. Needed after user joins sync group for the first time.
    /// Returns an array of bookmarks with updated `syncOrder`.
    class func updateBookmarksWithNewSyncOrder(parentFolder: Bookmark? = nil,
                                               context: NSManagedObjectContext) -> [Bookmark]? {
        
        var bookmarksToSend = [Bookmark]()
        
        let predicate = allBookmarksOfAGivenLevelPredicate(parent: parentFolder)
        
        let orderSort = NSSortDescriptor(key: #keyPath(Bookmark.order), ascending: true)
        let createdSort = NSSortDescriptor(key: #keyPath(Bookmark.created), ascending: false)
        
        let sort = [orderSort, createdSort]
        
        guard let allBookmarks = all(where: predicate, sortDescriptors: sort, context: context) else {
            return nil
        }
        
        // Sync ordering starts with 1.
        var counter = 1
        
        for bookmark in allBookmarks where bookmark.syncOrder == nil {
            
            if let parent = parentFolder, let syncOrder = parent.syncOrder {
                let order = syncOrder + ".\(counter)"
                bookmark.syncOrder = order
            } else {
                let order = baseOrder + "\(counter)"
                bookmark.syncOrder = order
            }
            
            bookmarksToSend.append(bookmark)
            counter += 1
            
            // Calling this method recursively to get ordering for nested bookmarks
            if bookmark.isFolder {
                if let updatedNestedBookmarks = updateBookmarksWithNewSyncOrder(parentFolder: bookmark,
                                                                                context: context) {
                    bookmarksToSend.append(contentsOf: updatedNestedBookmarks)
                }
            }
        }
        
        return bookmarksToSend
    }
    
    private class func maxSyncOrder(parent: Bookmark?,
                                    forFavorites: Bool,
                                    context: NSManagedObjectContext) -> String? {
        
        let predicate = forFavorites ?
            NSPredicate(format: "isFavorite == true") : allBookmarksOfAGivenLevelPredicate(parent: parent)
        
        guard let allBookmarks = all(where: predicate, context: context) else { return nil }
        
        // New bookmarks are sometimes added to context before this method is called.
        // We need to filter out bookmarks with empty sync orders.
        let highestOrderBookmark = allBookmarks.filter { $0.syncOrder != nil }.max { a, b in
            guard let aOrder = a.syncOrder, let bOrder = b.syncOrder else { return false } // Should be never nil at this point
            
            return aOrder < bOrder
        }
        
        return highestOrderBookmark?.syncOrder
    }
    
    func newSyncOrder(forFavorites: Bool, context: NSManagedObjectContext) {
        let lastBookmarkOrder = Bookmark.maxSyncOrder(parent: parentFolder,
                                                      forFavorites: forFavorites,
                                                      context: context)
        
        // The sync lib javascript method doesn't handle cases when there are no other bookmarks on a given level.
        // We need to do it locally, there are 3 cases:
        // 1. At least one bookmark is present at a given level -> we do the JS call
        // 2. Root level, no bookmarks added -> need to use baseOrder
        // 3. Nested folder, no bookmarks -> need to get parent folder syncOrder
        if lastBookmarkOrder == nil && parentFolder == nil {
            syncOrder = Bookmark.baseOrder + "1"
        } else if let parentOrder = parentFolder?.syncOrder, lastBookmarkOrder == nil {
            syncOrder = parentOrder + ".1"
        } else {
            syncOrder = Sync.shared.getBookmarkOrder(previousOrder: lastBookmarkOrder, nextOrder: nil)
        }
    }
    
    class func removeSyncOrders() {
        let context = DataController.newBackgroundContext()
        let allBookmarks = getAllBookmarks(context: context)
        
        allBookmarks.forEach { bookmark in
            bookmark.syncOrder = nil
            // TODO: Clear syncUUIDs
            //            bookmark.syncUUID = nil
        }
        
        DataController.save(context: context)
        Sync.shared.baseSyncOrder = nil
    }
}
