// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import BraveShared

private let log = Logger.browserLogger

/// Moving Core Data objects between different stores and contextes is risky.
/// This structure provides us data needed to move a bookmark from one place to another.
struct BookmarkRestorationData {
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

extension Bookmark {
    /// In 1.12 we moved database to new location. That migration caused some bugs for few users.
    /// This will attempt to restore bookmarks for them.
    ///
    /// Restoration must happen after Brave Sync is initialized.
    public static func restore_1_12_Bookmarks(completion: @escaping () -> Void) {
        let restorationCompleted = Preferences.Database.bookmark_v1_12_1RestorationCompleted
        
        if restorationCompleted.value { return }
        
        guard let migrationContainer = DataController.shared.oldDocumentStore else {
            log.debug("No database found in old location, skipping bookmark restoration")
            restorationCompleted.value = true
            return
        }
        
        let context = migrationContainer.viewContext
        
        guard let oldBookmarks = Bookmark.all(context: context)?.sorted() else {
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
    
    static func restoreLostBookmarksInternal(_ bookmarksToRestore: [Bookmark], completion: @escaping () -> Void) {
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
            guard let existingBookmarks =
                Bookmark.all(where: NSPredicate(format: "isFavorite == false"), context: context),
                let existingFavorites =
                Bookmark.all(where: NSPredicate(format: "isFavorite == true"), context: context) else {
                    return
            }
            
            log.debug("Attempting to restore \(oldFavoritesData.count) favorites.")
            if existingFavorites.isEmpty {
                log.debug("No existing favorites found, adding restored favorites to homescreen.")
                Bookmark.reinsertBookmarks(saveLocation: nil,
                                           bookmarksToInsertAtGivenLevel: oldFavoritesData,
                                           allBookmarks: oldFavoritesData,
                                           context: context)
            } else if !oldFavoritesData.isEmpty {
                log.debug("Existing favorites found, adding restored favorites to 'Restored Favorites' folder.")
                Bookmark.addInternal(url: nil, title: nil, customTitle: Strings.restoredFavoritesFolderName,
                                     isFolder: true, context: .existing(context)) { objectId in
                    
                    guard let restoredFavoritesFolder = context.object(with: objectId) as? Bookmark else {
                        return
                    }
                    
                    Bookmark.reinsertBookmarks(saveLocation: restoredFavoritesFolder,
                                               bookmarksToInsertAtGivenLevel: oldFavoritesData,
                                               allBookmarks: oldFavoritesData,
                                               context: context)
                }
            }
            
            log.debug("Attempting to restore \(oldBookmarksData.count) bookmarks.")
            // Entry point is root level bookmarks only.
            // Nested bookmarks are added recursively in `reinsertBookmarks` method
            let bookmarksAtRootLevel = oldBookmarksData.filter { $0.parentId == nil }
            
            if existingBookmarks.isEmpty {
                log.debug("No existing favorites found, adding restored bookmarks at root level.")
                Bookmark.reinsertBookmarks(saveLocation: nil,
                                           bookmarksToInsertAtGivenLevel: bookmarksAtRootLevel,
                                           allBookmarks: oldBookmarksData,
                                           context: context)
            } else if !oldBookmarksData.isEmpty {
                log.debug("No existing favorites found, adding restored bookmarks to 'Restored Bookmarks' folder.")
                Bookmark.addInternal(url: nil, title: nil, customTitle: Strings.restoredBookmarksFolderName, isFolder: true, context: .existing(context)) { objectId in
                    
                    guard let restoredBookmarksFolder = context.object(with: objectId) as? Bookmark else {
                        return
                    }
                    Bookmark.reinsertBookmarks(saveLocation: restoredBookmarksFolder,
                                               bookmarksToInsertAtGivenLevel: bookmarksAtRootLevel,
                                               allBookmarks: oldBookmarksData,
                                               context: context)
                }
            }
            
            completion()
        }
    }
    
    private static func reinsertBookmarks(saveLocation: Bookmark?,
                                          bookmarksToInsertAtGivenLevel: [BookmarkRestorationData],
                                          allBookmarks: [BookmarkRestorationData],
                                          context: NSManagedObjectContext) {
        
        bookmarksToInsertAtGivenLevel.forEach { bookmark in
            switch bookmark.bookmarkType {
            case .bookmark(let url):
                Bookmark.addInternal(url: url, title: bookmark.title, parentFolder: saveLocation,
                                     context: .existing(context))
            case .favorite(let url):
                // No save location for favorites means it is added straight into homescreen.
                // Otherwise favorites are treated as regular bookmarks inside of some folder.
                if saveLocation == nil {
                    Bookmark.addInternal(url: url, title: bookmark.title, isFavorite: true,
                                         sendToSync: false, context: .existing(context))
                } else {
                    Bookmark.addInternal(url: url, title: bookmark.title, parentFolder: saveLocation,
                                         context: .existing(context))
                }
                
            case .folder:
                Bookmark.addInternal(url: nil, title: bookmark.title, parentFolder: saveLocation,
                                     isFolder: true, context: .existing(context)) { objectId in
                    
                    // For folders we search for nested bookmarks recursively.
                    guard let folder = context.object(with: objectId) as? Bookmark else { return }
                    let children = allBookmarks.filter { $0.parentId == bookmark.id }
                    
                    self.reinsertBookmarks(saveLocation: folder,
                                           bookmarksToInsertAtGivenLevel: children, allBookmarks: allBookmarks,
                                           context: context)
                }
            }
        }
    }
}
