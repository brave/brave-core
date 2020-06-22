/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import CoreData
import Shared
import BraveShared
import JavaScriptCore

private let log = Logger.browserLogger

// Sync related methods for Bookmark model.
extension Bookmark {
    /// If sync is not used, we still utilize its syncOrder algorithm to determine order of bookmarks.
    /// Base order is needed to distinguish between bookmarks on different devices and platforms.
    static var baseOrder: String { return Preferences.Sync.baseSyncOrder.value }
    
    /// syncOrder for Brave < 1.8 has to be set in order to support the new ordering mechanism.
    /// Pre 1.8 bookmarks didn't have syncOrder set which makes it hard to calculate syncOrder for new
    // bookmarks, especially the ones inside of folders(nested Bookmarks syncOrder should be based on
    // its parentFolder order which we don't have in pre 1.8)
    public class func syncOrderMigration() {
        DataController.perform { context in
            let allBookmarks = getAllBookmarks(context: context)
            let bookmarksWithInvalidSyncOrder = allBookmarks.filter { $0.syncOrder == nil }
            
            if allBookmarks.count == bookmarksWithInvalidSyncOrder.count {
                updateBookmarksWithNewSyncOrder(context: context)
            }
            
            let allFavorites = all(where: NSPredicate(format: "isFavorite == YES"), context: context) ?? []
            let favoritesWithInvalidSyncOrder = allFavorites.filter { $0.syncOrder == nil }
            
            if allFavorites.count == favoritesWithInvalidSyncOrder.count {
                updateBookmarksWithNewSyncOrder(updateFavorites: true, context: context)
            }
        }
    }
    
    class func isSyncOrderValid(_ value: String) -> Bool {
        /// syncOrder must come in format x.x.x where x are numbers
        /// and it has 3 or more number components
        guard let regex = try? NSRegularExpression(pattern: "^(\\d+\\.){2,}\\d+$") else { return false }
        let range = NSRange(value.startIndex..., in: value)
        return regex.firstMatch(in: value, options: [], range: range) != nil
    }
    
    /// Sets order for all bookmarks. Needed after user joins sync group for the first time,
    // or after updating from older Brave versions(<1.8.0)
    /// Returns an array of bookmarks with updated `syncOrder`.
    @discardableResult
    class func updateBookmarksWithNewSyncOrder(parentFolder: Bookmark? = nil,
                                               updateFavorites: Bool = false,
                                               context: NSManagedObjectContext) -> [Bookmark]? {
        
        var bookmarksToSend = [Bookmark]()
        
        let predicate = updateFavorites ?
            NSPredicate(format: "isFavorite == YES") : allBookmarksOfAGivenLevelPredicate(parent: parentFolder)
        
        let orderSort = NSSortDescriptor(key: #keyPath(Bookmark.order), ascending: true)
        let createdSort = NSSortDescriptor(key: #keyPath(Bookmark.created), ascending: false)
        
        let sort = [orderSort, createdSort]
        
        guard let allBookmarks = all(where: predicate, sortDescriptors: sort, context: context) else {
            return nil
        }
        
        // Sync ordering starts with 1.
        var counter = 1
        
        for bookmark in allBookmarks {
            
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
            
            return aOrder.compare(bOrder, options: .numeric) == .orderedAscending 
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
        DataController.perform { context in
            let allBookmarks = getAllBookmarks(context: context)
            
            allBookmarks.forEach { bookmark in
                bookmark.syncOrder = nil
                // TODO: Clear syncUUIDs
                //            bookmark.syncUUID = nil
            }
            
            Preferences.Sync.baseSyncOrder.reset()
        }
    }
    
    /// We use a special String-based ordering algorithm for Bookmarks, which can't be sorted
    /// by using NSSortDescriptor.
    /// Therefore after each change of syncOrder, we recalculate position of all bookmarks on a given level
    /// and set correct `order` attribute.
    /// Thanks to this, we can utilize FetchedRequestController to handle changes within table views.
    /// Unfortunately this approach is not too performant, as each insert and update requires to update
    /// all other bookmarks on the same level too.
    class func setOrderForAllBookmarksOnGivenLevel(parent: Bookmark?, forFavorites: Bool, context: NSManagedObjectContext) {
        let predicate = forFavorites ?
            NSPredicate(format: "isFavorite == true") : allBookmarksOfAGivenLevelPredicate(parent: parent)
        
        guard let allBookmarks = all(where: predicate, context: context), allBookmarks.count > 1 else { return }
        // Bookmark implements custom sorting based on `syncOrder`
        let sortedBookmarks = allBookmarks.sorted()
        
        for (updatedOrder, bookmark) in sortedBookmarks.enumerated() {
            bookmark.order = Int16(updatedOrder)
        }
        // Saving context is handled outside of this method.
    }
}
