// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import BraveShared

private let log = Logger.browserLogger

/// Naming note:
/// Before sync v2 `Favorite` was named `Bookmark` and contained logic for both bookmarks and favorites.
/// Now it's renamed and few migration methods still rely on this old Bookmarks system.
/// LegacyBookmark can be either a bookmark or a favorite.
public typealias LegacyBookmark = Favorite

/// Contains methods that rely on old pre-syncv2 Bookmark system.
public struct LegacyBookmarksHelper {
    
    public static func getTopLevelLegacyBookmarks(
        _ context: NSManagedObjectContext? = nil) -> [LegacyBookmark] {
        Favorite.getTopLevelLegacyBookmarks(context)
    }
    
    public static func migrateBookmarkOrders() {
        Favorite.migrateBookmarkOrders()
    }
    
    public static func restore_1_12_Bookmarks(completion: @escaping () -> Void) {
        Favorite.restore_1_12_Bookmarks(completion: completion)
    }
}

// MARK: - 1.21 migration(Sync V2)
extension Favorite {
    fileprivate class func getTopLevelLegacyBookmarks(
        _ context: NSManagedObjectContext? = nil) -> [Favorite] {
        let predicate = NSPredicate(format: "isFavorite == NO and parentFolder = nil")
        return all(where: predicate, context: context ?? DataController.viewContext) ?? []
    }
}

// MARK: - 1.12 migration(database location change)
extension Favorite {
    /// Moving Core Data objects between different stores and contextes is risky.
    /// This structure provides us data needed to move a bookmark from one place to another.
    private struct BookmarkRestorationData {
        enum BookmarkType {
            case bookmark(url: URL)
            case favorite(url: URL)
            case folder
        }
        
        let bookmarkType: BookmarkType
        let id: NSManagedObjectID
        let parentId: NSManagedObjectID?
        let title: String
    }
    
    /// In 1.12 we moved database to new location. That migration caused some bugs for few users.
    /// This will attempt to restore bookmarks for them.
    ///
    /// Restoration must happen after Brave Sync is initialized.
    fileprivate static func restore_1_12_Bookmarks(completion: @escaping () -> Void) {
        let restorationCompleted = Preferences.Database.bookmark_v1_12_1RestorationCompleted
        
        if restorationCompleted.value { return }
        
        guard let migrationContainer = DataController.shared.oldDocumentStore else {
            log.debug("No database found in old location, skipping bookmark restoration")
            restorationCompleted.value = true
            return
        }
        
        let context = migrationContainer.viewContext
        
        guard let oldBookmarks = Favorite.all(context: context)?.sorted() else {
            // This might be some database problem. Not setting restoreation preference here
            // Trying to re-attempt on next launch.
            log.warning("Could not get bookmarks from database. Will retry restoration on next app launch.")
            return
        }
        
        if oldBookmarks.isEmpty {
            log.debug("Found database but it contains no records, skipping restoration.")
            restorationCompleted.value = true
            return
        }
        
        log.debug("Found \(oldBookmarks.count) items to restore.")
        restoreLostBookmarksInternal(oldBookmarks) {
            restorationCompleted.value = true
            completion()
        }
    }
    
    private static func restoreLostBookmarksInternal(_ bookmarksToRestore: [Favorite], completion: @escaping () -> Void) {
        var oldBookmarksData = [BookmarkRestorationData]()
        var oldFavoritesData = [BookmarkRestorationData]()
        
        for restoredBookmark in bookmarksToRestore {
            
            var bookmarkType: BookmarkRestorationData.BookmarkType?
            if restoredBookmark.isFolder {
                bookmarkType = .folder
            } else {
                // Remaining bookmark types must provide a valid URL
                guard let urlString = restoredBookmark.url, let bUrl = URL(string: urlString) else {
                    continue
                }
                
                bookmarkType = restoredBookmark.isFavorite ? .favorite(url: bUrl) : .bookmark(url: bUrl)
            }
            
            guard let type = bookmarkType else { continue }
            let bookmarkData = BookmarkRestorationData(bookmarkType: type, id: restoredBookmark.objectID,
                                                       parentId: restoredBookmark.parentFolder?.objectID,
                                                       title: restoredBookmark.displayTitle ?? "")
            
            if case .favorite(_) = type {
                oldFavoritesData.append(bookmarkData)
            } else {
                oldBookmarksData.append(bookmarkData)
            }
        }
        
        DataController.perform { context in
            log.debug("Attempting to restore \(oldFavoritesData.count) favorites.")
            Favorite.reinsertBookmarks(saveLocation: nil,
                                       bookmarksToInsertAtGivenLevel: oldFavoritesData,
                                       allBookmarks: oldFavoritesData,
                                       context: context)
            
            log.debug("Attempting to restore \(oldBookmarksData.count) bookmarks.")
            // Entry point is root level bookmarks only.
            // Nested bookmarks are added recursively in `reinsertBookmarks` method
            let bookmarksAtRootLevel = oldBookmarksData.filter { $0.parentId == nil }
            
            Favorite.reinsertBookmarks(saveLocation: nil,
                                       bookmarksToInsertAtGivenLevel: bookmarksAtRootLevel,
                                       allBookmarks: oldBookmarksData,
                                       context: context)
            
            completion()
        }
    }
    
    private static func reinsertBookmarks(saveLocation: Favorite?,
                                          bookmarksToInsertAtGivenLevel: [BookmarkRestorationData],
                                          allBookmarks: [BookmarkRestorationData],
                                          context: NSManagedObjectContext) {
        Favorite.migrateBookmarkOrders()
        bookmarksToInsertAtGivenLevel.forEach { bookmark in
            switch bookmark.bookmarkType {
            case .bookmark(let url):
                Favorite.addInternal(url: url, title: bookmark.title, isFavorite: false, context: .existing(context))
            case .favorite(let url):
                if saveLocation == nil {
                    Favorite.addInternal(url: url, title: bookmark.title, isFavorite: true,
                                         context: .existing(context))
                } else {
                    Favorite.addInternal(url: url, title: bookmark.title,
                                         isFavorite: false, context: .existing(context))
                }
                
            case .folder:
                let children = allBookmarks.filter { $0.parentId == bookmark.id }
                
                self.reinsertBookmarks(saveLocation: nil,
                                       bookmarksToInsertAtGivenLevel: children, allBookmarks: allBookmarks,
                                       context: context)
            }
        }
    }
}

// MARK: - 1.6 Migration (ordering bugs)
extension Favorite {
    fileprivate class func migrateBookmarkOrders() {
        DataController.perform { context in
            migrateOrder(forFavorites: true, context: context)
            migrateOrder(forFavorites: false, context: context)
        }
    }
    
    /// Takes all Bookmarks and Favorites from 1.6 and sets correct order for them.
    /// 1.6 had few bugs with reordering which we want to avoid, in particular non-reordered bookmarks on 1.6
    /// all have order set to 0 which makes sorting confusing.
    /// In migration we take all bookmarks using the same sorting method as on 1.6 and add a proper `order`
    /// attribute to them. The goal is to have all bookmarks with a proper unique order number set.
    private class func migrateOrder(parentFolder: Favorite? = nil,
                                    forFavorites: Bool,
                                    context: NSManagedObjectContext) {
        
        let predicate = forFavorites ?
            NSPredicate(format: "isFavorite == true") : allBookmarksOfAGivenLevelPredicate(parent: parentFolder)
        
        let orderSort = NSSortDescriptor(key: #keyPath(Favorite.order), ascending: true)
        let folderSort = NSSortDescriptor(key: #keyPath(Favorite.isFolder), ascending: false)
        let createdSort = NSSortDescriptor(key: #keyPath(Favorite.created), ascending: true)
        
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
    }
    
    private class func allBookmarksOfAGivenLevelPredicate(parent: Favorite?) -> NSPredicate {
        let isFavoriteKP = #keyPath(Favorite.isFavorite)
        let parentFolderKP = #keyPath(Favorite.parentFolder)
        
        // A bit hacky but you can't just pass 'nil' string to %@.
        let nilArgumentForPredicate = 0
        return NSPredicate(
            format: "%K == %@ AND %K == NO", parentFolderKP, parent ?? nilArgumentForPredicate, isFavoriteKP)
    }
}
