// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
import Shared
@testable import Data

class BookmarkTests: CoreDataTestCase {
    let fetchRequest = NSFetchRequest<Bookmark>(entityName: "Bookmark")
    
    override func setUp() {
        super.setUp()
        // Initialize sync so it will not fire on wrong thread.
        _ = Sync.shared
    }
    
    // MARK: - Getters/properties
    
    func testDisplayTitle() {
        let title = "Brave"
        let customTitle = "CustomBrave"
        
        // Case 1: custom title always takes precedence over regular title
        let result1 = createAndWait(url: nil, title: title, customTitle: customTitle)
        
        XCTAssertEqual(result1.displayTitle, customTitle)
        backgroundSaveAndWaitForExpectation {
            result1.delete()
        }
        
        let result2 = createAndWait(url: nil, title: nil, customTitle: customTitle)
        XCTAssertEqual(result2.displayTitle, customTitle)
        backgroundSaveAndWaitForExpectation {
            result2.delete()
        }
        
        // Case 2: Use title if no custom title provided
        let result3 = createAndWait(url: nil, title: title)
        XCTAssertEqual(result3.displayTitle, title)
        backgroundSaveAndWaitForExpectation {
            result3.delete()
        }
        
        // Case 3: Return nil if neither title or custom title provided
        let result4 = createAndWait(url: nil, title: nil)
        XCTAssertNil(result4.displayTitle)
        backgroundSaveAndWaitForExpectation {
            result4.delete()
        }
        
        // Case 4: Titles not nil but empty
        let result5 = createAndWait(url: nil, title: title, customTitle: "")
        XCTAssertEqual(result5.displayTitle, title)
        backgroundSaveAndWaitForExpectation {
            result5.delete()
        }
        let result6 = createAndWait(url: nil, title: "", customTitle: "")
        XCTAssertNil(result6.displayTitle)
    }
    
    func testFrc() {
        let frc = Bookmark.frc(parentFolder: nil)
        let request = frc.fetchRequest
        
        XCTAssertEqual(frc.managedObjectContext, DataController.viewContext)
        XCTAssertEqual(request.fetchBatchSize, 20)
        XCTAssertEqual(request.fetchLimit, 0)
        
        XCTAssertNotNil(request.sortDescriptors)
        XCTAssertNotNil(request.predicate)
        
        let bookmarksToAdd = 10
        insertBookmarks(amount: bookmarksToAdd)
        
        XCTAssertNoThrow(try frc.performFetch()) 
        let objects = frc.fetchedObjects!
        
        XCTAssertNotNil(objects)
        XCTAssertEqual(objects.count, bookmarksToAdd)
        
        // Testing if it sorts correctly
        XCTAssertEqual(objects.first?.title, "1")
        XCTAssertEqual(objects[5].title, "6")
        XCTAssertEqual(objects.last?.title, "10")
    }
    
    func testFrcWithParentFolder() {
        let folder = createAndWait(url: URL(string: ""), title: nil, customTitle: "Folder", isFolder: true)
        
        // Few not nested bookmarks
        let nonNestedBookmarksToAdd = 3
        insertBookmarks(amount: nonNestedBookmarksToAdd)
        
        // Few bookmarks inside our folder.
        insertBookmarks(amount: 5, parent: folder)
        
        let frc = Bookmark.frc(parentFolder: folder)
        
        XCTAssertNoThrow(try frc.performFetch()) 
        let objects = frc.fetchedObjects
        
        let bookmarksNotInsideOfFolder = nonNestedBookmarksToAdd + 1 // + 1 for folder
        let all = Bookmark.getAllBookmarks()
        XCTAssertEqual(objects?.count, all.count - bookmarksNotInsideOfFolder)
    }
    
    // MARK: - Create
    
    func testSimpleCreate() {
        let url = "https://brave.com"
        let title = "Brave"
        
        let result = createAndWait(url: URL(string: url), title: title)
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        XCTAssertEqual(result.url, url)
        XCTAssertEqual(result.title, title)
        assertDefaultValues(for: result)
    }
    
    func testCreateNilUrlAndTitle() {
        let result = createAndWait(url: nil, title: nil)
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        XCTAssertNil(result.url)
        XCTAssertNil(result.title)
        assertDefaultValues(for: result)
    }
    
    func testValidateBookmark() {
        let validate = BookmarkValidation.validateBookmark
        XCTAssertTrue(validate("Brave", "https://brave.com"))
        XCTAssertTrue(validate("Brave", "https://brave.com/"))
        XCTAssertTrue(validate("Brave", "https://brave.com"))
        XCTAssertTrue(validate("Brave", "https://brave.com/"))
        XCTAssertFalse(validate(nil, "https://brave.com"))
        XCTAssertTrue(validate("Brave", nil))
        XCTAssertFalse(validate("Brave", "https"))
        
        XCTAssertTrue(validate("Brave", "javascript:"))
        XCTAssertTrue(validate("Brave", "javascript://"))
        XCTAssertFalse(validate("Brave", "javascript:(function(){})"))
        XCTAssertFalse(validate("Brave", "javascript:(function(){})()"))
        
        XCTAssertTrue(validate("Brave", "https://brave.com?query=1"))
        XCTAssertTrue(validate("Brave", "https://brave.com/?query=1"))
        XCTAssertTrue(validate("Brave", "https://brave.com?query=1&other=2"))
        XCTAssertTrue(validate("Brave", "https://brave.com/?query=1&other=2"))
        
        XCTAssertTrue(validate("Brave", "ftp://brave.com"))
        XCTAssertTrue(validate("Brave", "https://brandon@brave.com"))
        XCTAssertTrue(validate("Brave", "https://brandon@brave.com/"))
        XCTAssertTrue(validate("Brave", "https://brandon:password@brave.com"))
        XCTAssertTrue(validate("Brave", "https://brandon:password@brave.com:8080"))
        XCTAssertTrue(validate("Brave", "https://brandon@brave.com:8080"))
        XCTAssertTrue(validate("Brave", "https://brave.com:8080"))
        XCTAssertTrue(validate("Brave", "https://www.brave.com"))
        XCTAssertTrue(validate("Brave", "https://www.brave.com:8080"))
        XCTAssertTrue(validate("Brave", "https://ww2.brave.com"))
        XCTAssertTrue(validate("Brave", "https://ww2.brave.com?query=%20test%20bookmarks"))
        
        // scheme-less urls..
        XCTAssertFalse(validate("Brave", "www.brave.com"))
        XCTAssertFalse(validate("Brave", "www.brave.com:8080"))
        XCTAssertFalse(validate("Brave", "brave.com"))
        XCTAssertFalse(validate("Brave", "brandon@brave.com"))
        XCTAssertFalse(validate("Brave", "brandon:password@brave.com"))
        XCTAssertFalse(validate("Brave", "brandon@brave.com:8080"))
        XCTAssertFalse(validate("Brave", "brandon:password@brave.com:8080"))
    }
    
    func testValidateBookmarklet() {
        let validate = BookmarkValidation.validateBookmarklet
        XCTAssertTrue(validate("Brave", "javascript:void(window.close(self))"))
        XCTAssertTrue(validate("Brave", "javascript:window.open('https://brave.com')"))
        XCTAssertTrue(validate("Brave", nil))
        XCTAssertFalse(validate("Brave", "javascript:function(){}"))
        XCTAssertTrue(validate("Brave", "javascript:(function(){})()"))
        XCTAssertTrue(validate("Brave", "javascript:(function(){})"))
        
        XCTAssertFalse(validate(nil, "javascript:window.open('https://brave.com')"))
        XCTAssertFalse(validate("Brave", "javascript:"))
        XCTAssertFalse(validate("Brave", "javascript:%20function(){}"))
        XCTAssertFalse(validate("Brave", "javascript:(function(){)"))
        
        XCTAssertFalse(validate("Brave", "javascript:/"))
        XCTAssertFalse(validate("Brave", "javascript://"))
        XCTAssertFalse(validate("Brave", "javascript://(function(){})"))
        XCTAssertFalse(validate("Brave", "https:(function(){})"))
        XCTAssertFalse(validate("Brave", "https://brave.com"))
        XCTAssertFalse(validate("Brave", "https://brave.com/"))
        XCTAssertFalse(validate("Brave", "https://"))
        XCTAssertFalse(validate("Brave", "brave:some"))
        XCTAssertFalse(validate("Brave", "brave:some?query=1"))
    }
    
    func testCreateFolder() {
        let url = "https://brave.com"
        let title = "Brave"
        let folderName = "FolderName"
        
        let result = createAndWait(url: URL(string: url), title: title, customTitle: folderName, isFolder: true)
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        XCTAssertEqual(result.title, title)
        XCTAssertEqual(result.customTitle, folderName)
        XCTAssert(result.isFolder)
        XCTAssertEqual(result.displayTitle, folderName)
    }
    
    func testMassInsert() {
        for i in 1...100 {
            let url = URL(string: "https://brave.com/\(i)")!
            let title = "Brave\(i)"
            
            createAndWait(url: url, title: title)
        }
        
        let count = try! DataController.viewContext.count(for: fetchRequest)
        XCTAssertEqual(count, 100)
    }
    
    // MARK: - Read
    
    func testContains() {
        let url = URL(string: "https://brave.com")!
        let wrongUrl = URL(string: "http://wrong.brave.com")!
        createAndWait(url: url, title: nil)
        
        XCTAssert(Bookmark.contains(url: url))
        XCTAssertFalse(Bookmark.contains(url: wrongUrl))
    }
    
    func testGetNonFolderChildren() {
        let folder = createAndWait(url: nil, title: nil, customTitle: "Folder", isFolder: true)
        
        let nonFolderBookmarksCount = 3
        insertBookmarks(amount: nonFolderBookmarksCount, parent: folder)
        
        // Add folder bookmark.
        createAndWait(url: nil,
                      title: nil,
                      customTitle: "InFolder",
                      parentFolder: folder,
                      isFolder: true,
                      isFavorite: false,
                      color: nil,
                      syncOrder: nil)
        
        XCTAssertEqual(Bookmark.getChildren(forFolder: folder, includeFolders: false)?.count, nonFolderBookmarksCount)
    }
    
    func testGetTopLevelFolders() {
        let folder = createAndWait(url: nil, title: nil, customTitle: "Folder1", isFolder: true)
        
        insertBookmarks(amount: 3)
        createAndWait(url: nil, title: nil, customTitle: "Folder2", isFolder: true)
        
        // Adding some bookmarks and one folder to our nested folder, to check that only top level folders are fetched.
        insertBookmarks(amount: 3, parent: folder)
        createAndWait(url: nil, title: nil, customTitle: "Folder3", parentFolder: folder, isFolder: true)
        
        // 3 folders in total, 2 in root directory
        XCTAssertEqual(Bookmark.getTopLevelFolders().count, 2)
    }
    
    func testGetAllBookmarks() {
        let bookmarksCount = 3
        insertBookmarks(amount: bookmarksCount)
        // Adding a favorite(non-bookmark type of bookmark)
        createAndWait(url: URL(string: "https://brave.com"), title: "Brave", isFavorite: true)
        XCTAssertEqual(Bookmark.getAllBookmarks().count, bookmarksCount)
    }
    
    func testHasFavorites() {
        XCTAssertFalse(Bookmark.hasFavorites)
        
        insertBookmarks(amount: 3)
        
        XCTAssertFalse(Bookmark.hasFavorites)
        
        createAndWait(url: URL(string: "https://brave.com"), title: "Brave", isFavorite: true)
        
        XCTAssert(Bookmark.hasFavorites)
    }
    
    func testGetAllFavorites() {
        createAndWait(url: URL(string: "http://example.com/1"), title: "Example1", isFavorite: true)
        insertBookmarks(amount: 3)
        createAndWait(url: URL(string: "http://example.com/2"), title: "Example2", isFavorite: true)
        
        let allFavorites = Bookmark.allFavorites
        XCTAssertEqual(allFavorites.count, 2)
        
        XCTAssert(allFavorites.contains(where: { $0.title == "Example1" }))
        XCTAssert(allFavorites.contains(where: { $0.title == "Example2" }))
        XCTAssertFalse(allFavorites.contains(where: { $0.title == "Example3" }))
        
        createAndWait(url: URL(string: "http://example.com/3"), title: "Example3", isFavorite: true)
        XCTAssertEqual(Bookmark.allFavorites.count, 3)
    }
    
    // MARK: - Update
    
    func testUpdateBookmark() {
        let context = DataController.viewContext
        let url = "https://brave.com"
        let customTitle = "Brave"
        let newUrl = "http://updated.example.com"
        let newCustomTitle = "Example"
        
        let object = createAndWait(url: URL(string: url), title: "title", customTitle: customTitle)
        XCTAssertEqual(Bookmark.getAllBookmarks().count, 1)
        
        XCTAssertEqual(object.displayTitle, customTitle)
        XCTAssertEqual(object.url, url)
        
        backgroundSaveAndWaitForExpectation {
            object.update(customTitle: newCustomTitle, url: newUrl)
        }
        DataController.viewContext.refreshAllObjects()
        
        // Let's make sure not any new record was added to DB
        XCTAssertEqual(Bookmark.getAllBookmarks(context: context).count, 1)
        
        let newObject = try! DataController.viewContext.fetch(fetchRequest).first!
        
        XCTAssertNotEqual(newObject.title, customTitle)
        XCTAssertNotEqual(newObject.customTitle, customTitle)
        XCTAssertNotEqual(newObject.url, url)
        
        XCTAssertEqual(newObject.title, newCustomTitle)
        XCTAssertEqual(newObject.customTitle, newCustomTitle)
        XCTAssertEqual(newObject.url, newUrl)
    }
    
    func testUpdateBookmarkNoChanges() {
        let customTitle = "Brave"
        let url = "https://brave.com"
                
        let object = createAndWait(url: URL(string: url), title: "title", customTitle: customTitle)
        XCTAssertEqual(Bookmark.getAllBookmarks().count, 1)
        
        backgroundSaveAndWaitForExpectation(inverted: true) {
            object.update(customTitle: customTitle, url: object.url)
        }
        
        // Make sure not any new record was added to DB
        XCTAssertEqual(Bookmark.getAllBookmarks().count, 1)
        
        XCTAssertEqual(object.customTitle, customTitle)
        XCTAssertEqual(object.url, url)
    }
    
    func testUpdateBookmarkBadUrl() {
        let customTitle = "Brave"
        let url = "https://brave.com"
        let badUrl = "   " // Empty spaces cause URL(string:) to return nil
        
        let object = createAndWait(url: URL(string: url), title: "title", customTitle: customTitle)
        XCTAssertEqual(Bookmark.getAllBookmarks().count, 1)
        
        XCTAssertNotNil(object.domain)
        
        backgroundSaveAndWaitForExpectation {
            object.update(customTitle: customTitle, url: badUrl)
        }
        DataController.viewContext.refreshAllObjects()
        
        // Let's make sure not any new record was added to DB
        XCTAssertEqual(Bookmark.getAllBookmarks().count, 1)
        XCTAssertNil(object.domain)
    }
    
    func testUpdateFolder() {
        let context = DataController.viewContext
        let customTitle = "Folder"
        let newCustomTitle = "FolderUpdated"
        
        let object = createAndWait(url: nil, title: nil, customTitle: customTitle, isFolder: true)
        XCTAssertEqual(Bookmark.getAllBookmarks().count, 1)
        
        XCTAssertEqual(object.displayTitle, customTitle)
        
        backgroundSaveAndWaitForExpectation {
            object.update(customTitle: newCustomTitle, url: nil)
        }
        DataController.viewContext.refreshAllObjects()
        
        // Let's make sure not any new record was added to DB
        XCTAssertEqual(Bookmark.getAllBookmarks(context: context).count, 1)
        
        XCTAssertEqual(object.displayTitle, newCustomTitle)
    }
    
    func testBookmarkReorderDragDown() {
        let result = reorder(sourcePosition: 0, destinationposition: 5)
        let sourceObject = result.src
        let destinationObject = result.dest
        
        XCTAssertEqual(sourceObject.order, 5)
        XCTAssertEqual(destinationObject.order, 4)
    }
    
    func testBookmarkReorderDragUp() {
        let result = reorder(sourcePosition: 5, destinationposition: 1)
        let sourceObject = result.src
        let destinationObject = result.dest
        
        XCTAssertEqual(sourceObject.order, 1)
        XCTAssertEqual(destinationObject.order, 2)
    }
    
    func testBookmarkReorderTopToBottom() {
        let result = reorder(sourcePosition: 9, destinationposition: 0, skipOrderChangeTests: true)
        let sourceObject = result.src
        let destinationObject = result.dest
        
        XCTAssertEqual(sourceObject.order, 0)
        XCTAssertEqual(destinationObject.order, 1)
    }
    
    func testBookmarkReorderBottomToTop() {
        let result = reorder(sourcePosition: 0, destinationposition: 9)
        let sourceObject = result.src
        let destinationObject = result.dest
        
        XCTAssertEqual(sourceObject.order, 9)
        XCTAssertEqual(destinationObject.order, 8)
    }
    
    func testBookmarksReorderSameIndexPaths() {
        insertBookmarks(amount: 10)
        
        let frc = Bookmark.frc(parentFolder: nil)
        try! frc.performFetch()
        
        let sourceIndexPath = IndexPath(row: 5, section: 0)
        let destinationIndexPath = IndexPath(row: 5, section: 0)
        
        let sourceOrderBefore = (frc.object(at: sourceIndexPath)).order
        let destinationOrderBefore = (frc.object(at: destinationIndexPath)).order
        
        Bookmark.reorderBookmarks(frc: frc, sourceIndexPath: sourceIndexPath, destinationIndexPath: destinationIndexPath)
        
        // Test order haven't changed
        XCTAssertEqual((frc.object(at: sourceIndexPath)).order, sourceOrderBefore)
        XCTAssertEqual((frc.object(at: destinationIndexPath)).order, destinationOrderBefore)
    }
    
    func testBookmarksReorderNilFrc() {
        let sourceIndexPath = IndexPath(row: 0, section: 0)
        let destinationIndexPath = IndexPath(row: 5, section: 0)
        
        Bookmark.reorderBookmarks(frc: nil, sourceIndexPath: sourceIndexPath, destinationIndexPath: destinationIndexPath)
        // Assert nothing.
    }
    
    func testSyncOrderComparator() {
        var bookmarks = [Bookmark]()
        
        let syncOrders = ["1.0.10", "1.0.4", "1.0.5", "1.0.6", "1.0.7", "2.1.1", "1.0.5.1.2",
                          "1.0.1", "1.0.2", "1.0.3", "1.0.8", "1.0.9","1.0.11", "1.0.12", "1.0.13"]
        
        syncOrders.forEach {
            let bookmark = Bookmark(context: DataController.viewContext)
            bookmark.syncOrder = $0
            bookmarks.append(bookmark)
        }
        
        let expectedOrder = ["1.0.1", "1.0.2", "1.0.3", "1.0.4", "1.0.5", "1.0.5.1.2", "1.0.6", "1.0.7",
                             "1.0.8", "1.0.9", "1.0.10", "1.0.11", "1.0.12", "1.0.13", "2.1.1"]
        
        let bookmarksSorted = bookmarks.sorted()
        let syncOrdersSorted = bookmarksSorted.compactMap { $0.syncOrder }
        
        XCTAssertEqual(syncOrdersSorted, expectedOrder)
    }
    
    func testForceOverwriteFavorites() {
        let regularBookmarksCount = 3
        insertBookmarks(amount: regularBookmarksCount)
        createAndWait(url: URL(string: "http://example.com/1"), title: "Example1", isFavorite: true)
        createAndWait(url: URL(string: "http://example.com/2"), title: "Example2", isFavorite: true)
        
        let newFavorites = [(URL(string: "http://example.com/3")!, "Example3"),
                            (URL(string: "http://example.com/4")!, "Example4"),
                            (URL(string: "http://example.com/5")!, "Example5")]
        
        backgroundSaveAndWaitForExpectation {
            Bookmark.forceOverwriteFavorites(with: newFavorites)
        }
        
        let updatedFavorites = Bookmark.allFavorites
        XCTAssertEqual(updatedFavorites.count, newFavorites.count)
        
        XCTAssert(updatedFavorites.contains(where: { $0.title == "Example4" }))
        XCTAssertFalse(updatedFavorites.contains(where: { $0.title == "Example1" }))
        
        // Make sure we don't delete any normal bookmarks by accident.
        XCTAssertEqual(Bookmark.getAllBookmarks().count, regularBookmarksCount)
    }
    
    // MARK: - Delete
    
    func testRemoveByUrl() {
        let url = URL(string: "https://brave.com")!
        let wrongUrl = URL(string: "http://wrong.brave.com")!
        
        createAndWait(url: url, title: "Brave")
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        Bookmark.remove(forUrl: wrongUrl)
        sleep(UInt32(0.5))
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        backgroundSaveAndWaitForExpectation {
            Bookmark.remove(forUrl: url)
        }
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 0)
    }
    
    // MARK: - Syncable
    
    func testAddSyncable() {
        let url = URL(string: "https://brave.com")!
        let title = "Brave"
        
        let site = SyncSite()
        site.title = title
        site.location = url.absoluteString
        
        let bookmark = SyncBookmark()
        bookmark.site = site
        
        backgroundSaveAndWaitForExpectation {
            DataController.perform { context in
                Bookmark.createResolvedRecord(rootObject: bookmark, save: true, context: .existing(context))
            }
        }
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
    }
    
    func testUpdateSyncable() {
        let url = URL(string: "https://brave.com")!
        let title = "Brave"
        
        let newUrl = "http://example.com"
        let newTitle = "BraveUpdated"
        
        let site = SyncSite()
        site.title = newTitle
        site.location = newUrl
        
        let syncBookmark = SyncBookmark()
        syncBookmark.site = site
        
        let object = createAndWait(url: url, title: title)
        
        let oldCreated = object.created
        let oldLastVisited = object.lastVisited
        
        XCTAssertNotEqual(object.title, newTitle)
        XCTAssertNotEqual(object.url, newUrl)
        
        backgroundSaveAndWaitForExpectation {
            object.updateResolvedRecord(syncBookmark, context: .new(inMemory: false))
        }
        
        DataController.viewContext.refreshAllObjects()
        
        let newObject = try! DataController.viewContext.fetch(fetchRequest).first!
        
        XCTAssertEqual(newObject.title, newTitle)
        XCTAssertEqual(newObject.url, newUrl)
        
        XCTAssertEqual(newObject.created, oldCreated)
        XCTAssertNotEqual(newObject.lastVisited, oldLastVisited)
    }
    
    func testAsDictionary() {
        let url = URL(string: "https://brave.com")!
        let title = "Brave"
        
        backgroundSaveAndWaitForExpectation {
            Device.add(name: "Brave")
        }
        
        let deviceId = Device.currentDevice()?.deviceId
        
        let object = createAndWait(url: url, title: title)
        
        let dict = object.asDictionary(deviceId: deviceId, action: 0)
        
        // Just checking if keys exist. More robust tests should be placed in SyncableTests.
        XCTAssertNotNil(dict["objectId"])
        XCTAssertNotNil(dict["bookmark"])
        XCTAssertNotNil(dict["action"])
        XCTAssertNotNil(dict["objectData"])
    }
    
    func testFrecencyQuery() {
        insertBookmarks(amount: 6)
        
        let found = Bookmark.byFrecency(query: "brave")
        // Query limit is 5
        XCTAssertEqual(found.count, 5)
        
        let notFound = Bookmark.byFrecency(query: "notfound")
        XCTAssertEqual(notFound.count, 0)
    }
    
    // MARK: - Helpers
    
    func testSyncOrderValidation() {
        // Valid values
        ["0.0.1", "12.23.345.454", "1.2.3.4.5.6.7"].forEach {
            XCTAssertTrue(Bookmark.isSyncOrderValid($0))
        }
        
        // Invalid values
        [".1.2.3", "1.2.3.", "undefined", "null" , "1,2.3", "-1.0.2", "1.a.3",
         "1", "12", "1.2", "1.2..3, 1.2.43a.42"].forEach {
            XCTAssertFalse(Bookmark.isSyncOrderValid($0), "False positive for : \($0)")
        }
    }
    
    /// Wrapper around `Bookmark.create()` with context save wait expectation and fetching object from view context.
    @discardableResult 
    private func createAndWait(url: URL?, title: String?, customTitle: String? = nil, 
                               parentFolder: Bookmark? = nil, isFolder: Bool = false, isFavorite: Bool = false, color: UIColor? = nil, syncOrder: String? = nil) -> Bookmark {
        
        backgroundSaveAndWaitForExpectation {
            Bookmark.addInternal(url: url, title: title, customTitle: customTitle, parentFolder: parentFolder,
                         isFolder: isFolder, isFavorite: isFavorite, syncOrder: syncOrder)
        }
        
        let sort = NSSortDescriptor(key: "created", ascending: false)
        
        return Bookmark.first(sortDescriptors: [sort])!
    }
    
    private func insertBookmarks(amount: Int, parent: Bookmark? = nil) {
        let bookmarksBeforeInsert = try! DataController.viewContext.count(for: fetchRequest)
        
        let url = "https://brave.com/"
        for i in 1...amount {
            let title = String(i)
            createAndWait(url: URL(string: url + title), title: title, parentFolder: parent)
        }
        
        let difference = bookmarksBeforeInsert + amount
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), difference)
    }
    
    private func reorder(sourcePosition: Int, destinationposition: Int, 
                         skipOrderChangeTests: Bool = false) -> (src: Bookmark, dest: Bookmark) {
        insertBookmarks(amount: 10)
        
        let frc = Bookmark.frc(parentFolder: nil)
        try! frc.performFetch()
        
        let sourceIndexPath = IndexPath(row: sourcePosition, section: 0)
        let destinationIndexPath = IndexPath(row: destinationposition, section: 0)
        
        let sourceObject = frc.object(at: sourceIndexPath)
        let destinationObject = frc.object(at: destinationIndexPath)
        
        let sourceOrderBefore = (frc.object(at: sourceIndexPath)).order
        
        // Bookmark reordering actually do a lot of saves now so we have to wait for the context notification.
        backgroundSaveAndWaitForExpectation {
            Bookmark.reorderBookmarks(frc: frc, sourceIndexPath: sourceIndexPath, destinationIndexPath: destinationIndexPath)
        }
        
        DataController.viewContext.refresh(sourceObject, mergeChanges: false)
        
        // Test order has changed, won't work when swapping bookmarks with order = 0
        if !skipOrderChangeTests {
            XCTAssertNotEqual(sourceObject.order, sourceOrderBefore)
        }
        
        return (sourceObject, destinationObject)
    }
    
    private func assertDefaultValues(for record: Bookmark) {
        // Test awakeFromInsert()
        XCTAssertNotNil(record.created)
        XCTAssertNotNil(record.lastVisited)
        // Make sure date doesn't point to 1970-01-01
        let initialDate = Date(timeIntervalSince1970: 0)
        XCTAssertNotEqual(record.created, initialDate)
        
        // Test defaults
        XCTAssertFalse(record.isFolder)
        XCTAssertFalse(record.isFavorite)
        
        XCTAssertNil(record.parentFolder)
        XCTAssertNil(record.customTitle)
        XCTAssertNil(record.syncParentDisplayUUID)
        XCTAssertNil(record.syncParentUUID)
        
        XCTAssertNotNil(record.syncDisplayUUID)
        XCTAssertNotNil(record.children)
        XCTAssert(record.children!.isEmpty)
    }
}
