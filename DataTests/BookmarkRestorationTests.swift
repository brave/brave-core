// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest

import CoreData
import Shared
@testable import Data

class BookmarkRestorationTests: CoreDataTestCase {
    
    let fetchRequest = NSFetchRequest<Bookmark>(entityName: "Bookmark")
    
    override func setUp() {
        super.setUp()
        // Initialize sync so it will not fire on wrong thread.
        _ = Sync.shared
    }

    func testRestorationEmptyBookmarks() {
        let count = 1
        
        createExistingBookmarks(amount: 1)
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), count)
        
        let exp = expectation(description: "restore")
        Bookmark.restoreLostBookmarksInternal([]) {
            exp.fulfill()
        }
        
        wait(for: [exp], timeout: 3)
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), count)
    }
    
    func testRestorationBookmarksOnlyNoNewBookmarks() {
        let count = 2
        restore(bookmarks: createBookmarksToRestore(bookmarksAmount: count))
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), count)
    }
    
    func testRestorationBookmarksOnlyExistingBookmarks() {
        let existingBookmarks = createExistingBookmarks(amount: 2)
        
        let count = 3
        restore(bookmarks: createBookmarksToRestore(bookmarksAmount: count))
        
        let totalCount = count + existingBookmarks.count + 1
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), totalCount)
        
        assertRestoredFolders(bookmarksFolderExists: true, favoritesFolderExists: false)
        restoredFolderCount(restoredBookmarksFolder: true, count: count)
    }
    
    func testRestorationFavoritesOnlyWithNoFavorites() {
        let count = 2
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 0)
        
        restore(bookmarks: createBookmarksToRestore(favoritesAmount: count))
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), count)
        
        let favoritesPredicate = NSPredicate(format: "isFavorite == true")
        let bookmarksPredicate = NSPredicate(format: "isFavorite == false")
        
        let allFavorites = Bookmark.all(where: favoritesPredicate)!
        let allBookmarks = Bookmark.all(where: bookmarksPredicate)!
        
        XCTAssertEqual(count, allFavorites.count)
        XCTAssertEqual(allBookmarks.count, 0)
    }
    
    func testRestorationFavoritesOnlyWithExistingFavorites() {
        let existingBookmarks = createExistingBookmarks(amount: 2, favorites: true)
        
        let count = 3
        let aa = createBookmarksToRestore(favoritesAmount: count)
        restore(bookmarks: aa)
        
        let totalCount = count + existingBookmarks.count + 1
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), totalCount)
        
        assertRestoredFolders(bookmarksFolderExists: false, favoritesFolderExists: true)
        restoredFolderCount(restoredBookmarksFolder: false, count: count)
    }
    
    func testRestorationFavoritesAndBookmarks() {
        let existingBookmarks =
            createExistingBookmarks(amount: 2, favorites: true) + createExistingBookmarks(amount: 2)
        
        let bookmarksCount = 4
        let favoritesCount = 2
        let restoredBookmarks = createBookmarksToRestore(bookmarksAmount: bookmarksCount,
                                                         favoritesAmount: favoritesCount)
        
        restore(bookmarks: restoredBookmarks)
        
        let totalCount = bookmarksCount + favoritesCount + existingBookmarks.count + 2
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), totalCount)
        
        assertRestoredFolders(bookmarksFolderExists: true, favoritesFolderExists: true)
        
        restoredFolderCount(restoredBookmarksFolder: true, count: bookmarksCount)
        restoredFolderCount(restoredBookmarksFolder: false, count: favoritesCount)
    }
    
    func testRestorationBookmarksNestedFolders() {
        let existingBookmarks = createExistingBookmarks(amount: 2)
        
        let context = DataController.viewContext
        
        let b1 = Bookmark(context: context)
        let b2 = Bookmark(context: context)
        let b3 = Bookmark(context: context)
        let b4 = Bookmark(context: context)
        
        context.reset()
        
        b1.customTitle = "1"
        b1.isFolder = true
        
        b2.customTitle = "2"
        b2.isFolder = true
        b2.parentFolder = b1
        
        b3.url = "https://super.example.com/3"
        b3.title = "Super example"
        b3.parentFolder = b2
        
        b4.url = "https://super.example.com/4"
        b4.title = "Super example"
        
        let bookmarks = [b1, b2, b3, b4]
        
        restore(bookmarks: bookmarks)
        
        let totalCount = bookmarks.count + existingBookmarks.count + 1
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), totalCount)
        
        assertRestoredFolders(bookmarksFolderExists: true, favoritesFolderExists: false)
        restoredFolderCount(restoredBookmarksFolder: true, count: 2) // Remaining bookmarks are in nested folders
    }
    
    private func restore(bookmarks: [Bookmark]) {
        let exp = expectation(description: "restore")
        backgroundSaveAndWaitForExpectation {
            Bookmark.restoreLostBookmarksInternal(bookmarks) {
                exp.fulfill()
            }
        }
        
        wait(for: [exp], timeout: 3)
    }
    
    private func restoredFolderCount(restoredBookmarksFolder: Bool, count: Int) {
        let name = restoredBookmarksFolder ? "Restored Bookmarks" : "Restored Favorites"
        let restoredFolderPredicate = NSPredicate(format: "customTitle == %@", name)
        let restoredFolder = Bookmark.first(where: restoredFolderPredicate)
        
        let contentsPredicate = NSPredicate(format: "parentFolder == %@", restoredFolder!)
        
        let children = Bookmark.all(where: contentsPredicate)
        XCTAssertEqual(children!.count, count)
    }
    
    private func assertRestoredFolders(bookmarksFolderExists: Bool, favoritesFolderExists: Bool) {
        let restoredFolderPredicate = NSPredicate(format: "customTitle == %@", "Restored Favorites")
        let restoredFolder = Bookmark.first(where: restoredFolderPredicate)
        if favoritesFolderExists {
            XCTAssertNotNil(restoredFolder)
        } else {
            XCTAssertNil(restoredFolder)
        }
        
        let restoredBookmarksPredicate = NSPredicate(format: "customTitle == %@", "Restored Bookmarks")
        let restoredBookmarksFolderFolder = Bookmark.first(where: restoredBookmarksPredicate)
        if bookmarksFolderExists {
            XCTAssertNotNil(restoredBookmarksFolderFolder)
        } else {
            XCTAssertNil(restoredBookmarksFolderFolder)
        }
    }
    
    private func createBookmarksToRestore(bookmarksAmount: Int = 0, favoritesAmount: Int = 0) -> [Bookmark] {
        let context = DataController.viewContext
        var bookmarks = [Bookmark]()
        
        for _ in 1...bookmarksAmount + favoritesAmount {
            let bookmark = Bookmark(context: context)
            bookmarks.append(bookmark)
        }
        
        // This is to drop bookmarks from current store, to pretend we took them from the old store.
        context.reset()
        
        bookmarks.enumerated().forEach { index, bookmark in
            bookmark.url = "http://example.com/\(index)"
            bookmark.title = "Example \(index)"
            if index < favoritesAmount {
                bookmark.isFavorite = true
            }
        }
                
        return bookmarks
    }
    
    /// Wrapper around `Bookmark.create()` with context save wait expectation and fetching object from view context.
    @discardableResult
    private func createAndWait(url: URL?, title: String, isFavorite: Bool = false) -> Bookmark {
        
        backgroundSaveAndWaitForExpectation {
            Bookmark.addInternal(url: url, title: title, isFavorite: isFavorite)
        }
        
        let sort = NSSortDescriptor(key: "created", ascending: false)
        
        return Bookmark.first(sortDescriptors: [sort])!
    }
    
    @discardableResult
    private func createExistingBookmarks(amount: Int, favorites: Bool = false) -> [Bookmark] {
        var bookmarks = [Bookmark]()
        
        for i in 1...amount {
            backgroundSaveAndWaitForExpectation {
                Bookmark.addInternal(url: URL(string: "http://example.com/\(i)"),
                                     title: "Existing Example \(i)", isFavorite: favorites)
            }
            
            let sort = NSSortDescriptor(key: "created", ascending: false)
            let bookmark = Bookmark.first(sortDescriptors: [sort])!
            
            bookmarks.append(bookmark)
        }
        
        return bookmarks
    }

}
