// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
@testable import Data

class BookmarkTests: CoreDataTestCase {
    let fetchRequest = NSFetchRequest<Bookmark>(entityName: String(describing: Bookmark.self))
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: String(describing: Bookmark.self), in: context)!
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
        XCTAssertEqual(objects.first?.title, "10")
        XCTAssertEqual(objects[5].title, "5")
        XCTAssertEqual(objects.last?.title, "1")
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
        let all = Bookmark.getAllBookmarks(context: DataController.viewContext)
        XCTAssertEqual(objects?.count, all.count - bookmarksNotInsideOfFolder)
    }
    
    // MARK: - Create
    
    func testSimpleCreate() {
        let url = "http://brave.com"
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
    
    func testCreateFolder() {
        let url = "http://brave.com"
        let title = "Brave"
        let folderName = "FolderName"
        
        let result = createAndWait(url: URL(string: url), title: title, customTitle: folderName, isFolder: true)
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        XCTAssertEqual(result.title, title)
        XCTAssertEqual(result.customTitle, folderName)
        XCTAssert(result.isFolder)
        XCTAssertEqual(result.displayTitle, folderName)
    }
    
    // MARK: - Read
    
    func testContains() {
        let url = URL(string: "http://brave.com")!
        let wrongUrl = URL(string: "http://wrong.brave.com")!
        createAndWait(url: url, title: nil)
        
        XCTAssert(Bookmark.contains(url: url))
        XCTAssertFalse(Bookmark.contains(url: wrongUrl))
    }
    
    func testGetChildren() {
        let folder = createAndWait(url: nil, title: nil, customTitle: "Folder", isFolder: true)
        
        let nonNestedBookmarksToAdd = 3
        insertBookmarks(amount: nonNestedBookmarksToAdd)
        
        // Few bookmarks inside our folder.
        let nestedBookmarksCount = 5
        insertBookmarks(amount: nestedBookmarksCount, parent: folder)
        
        XCTAssertEqual(Bookmark.getChildren(forFolderUUID: folder.syncUUID)?.count, nestedBookmarksCount)
    }
    
    func testGetTopLevelFolders() {
        let folder = createAndWait(url: nil, title: nil, customTitle: "Folder1", isFolder: true)
        
        insertBookmarks(amount: 3)
        createAndWait(url: nil, title: nil, customTitle: "Folder2", isFolder: true)
        
        // Adding some bookmarks and one folder to our nested folder, to check that only top level folders are fetched.
        insertBookmarks(amount: 3, parent: folder)
        createAndWait(url: nil, title: nil, customTitle: "Folder3", parentFolder: folder, isFolder: true)
        
        // 3 folders in total, 2 in root directory
        XCTAssertEqual(Bookmark.getFolders(bookmark: nil, context: DataController.viewContext).count, 2)
    }
    
    func testGetAllBookmarks() {
        let context = DataController.viewContext
        let bookmarksCount = 3
        insertBookmarks(amount: bookmarksCount)
        // Adding a favorite(non-bookmark type of bookmark)
        createAndWait(url: URL(string: "http://brave.com"), title: "Brave", isFavorite: true)
        XCTAssertEqual(Bookmark.getAllBookmarks(context: context).count, bookmarksCount)
    }
    
    // MARK: - Update
    
    func testUpdateBookmark() {
        let context = DataController.viewContext
        let url = "http://brave.com"
        let customTitle = "Brave"
        let newUrl = "http://updated.example.com"
        let newCustomTitle = "Example"
        
        let object = createAndWait(url: URL(string: url), title: "title", customTitle: customTitle)
        XCTAssertEqual(Bookmark.getAllBookmarks(context: context).count, 1)
        
        XCTAssertEqual(object.displayTitle, customTitle)
        XCTAssertEqual(object.url, url)
        
        backgroundSaveAndWaitForExpectation {
            object.update(customTitle: newCustomTitle, url: newUrl, save: true)
        }
        DataController.viewContext.refreshAllObjects()
        
        // Let's make sure not any new record was added to DB
        XCTAssertEqual(Bookmark.getAllBookmarks(context: context).count, 1)
        
        let newObject = try! DataController.viewContext.fetch(fetchRequest).first!
        
        XCTAssertNotEqual(newObject.displayTitle, customTitle)
        XCTAssertNotEqual(newObject.url, url)
        
        XCTAssertEqual(newObject.displayTitle, newCustomTitle)
        XCTAssertEqual(newObject.url, newUrl)
    }
    
    func testUpdateBookmarkNoChanges() {
        let context = DataController.viewContext
        let customTitle = "Brave"
        let url = "http://brave.com"
                
        let object = createAndWait(url: URL(string: url), title: "title", customTitle: customTitle)
        XCTAssertEqual(Bookmark.getAllBookmarks(context: context).count, 1)
        
        object.update(customTitle: customTitle, url: object.url, save: true)
        sleep(UInt32(1))
        
        // Make sure not any new record was added to DB
        XCTAssertEqual(Bookmark.getAllBookmarks(context: context).count, 1)
        
        XCTAssertEqual(object.customTitle, customTitle)
        XCTAssertEqual(object.url, url)
    }
    
    func testUpdateBookmarkBadUrl() {
        let context = DataController.viewContext
        let customTitle = "Brave"
        let url = "http://brave.com"
        let badUrl = "   " // Empty spaces cause URL(string:) to return nil
        
        let object = createAndWait(url: URL(string: url), title: "title", customTitle: customTitle)
        XCTAssertEqual(Bookmark.getAllBookmarks(context: context).count, 1)
        
        XCTAssertNotNil(object.domain)
        
        backgroundSaveAndWaitForExpectation {
            object.update(customTitle: customTitle, url: badUrl, save: true)
        }
        DataController.viewContext.refreshAllObjects()
        
        // Let's make sure not any new record was added to DB
        XCTAssertEqual(Bookmark.getAllBookmarks(context: context).count, 1)
        XCTAssertNil(object.domain)
    }
    
    func testUpdateFolder() {
        let context = DataController.viewContext
        let customTitle = "Folder"
        let newCustomTitle = "FolderUpdated"
        
        let object = createAndWait(url: nil, title: nil, customTitle: customTitle, isFolder: true)
        XCTAssertEqual(Bookmark.getAllBookmarks(context: context).count, 1)
        
        XCTAssertEqual(object.displayTitle, customTitle)
        
        backgroundSaveAndWaitForExpectation {
            object.update(customTitle: newCustomTitle, url: nil, save: true)
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
    
    // MARK: - Delete
    
    func testRemoveByUrl() {
        let url = URL(string: "http://brave.com")!
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
        let url = URL(string: "http://brave.com")!
        let title = "Brave"
        
        let site = SyncSite()
        site.title = title
        site.location = url.absoluteString
        
        let bookmark = SyncBookmark()
        bookmark.site = site
        
        backgroundSaveAndWaitForExpectation {
            Bookmark.add(rootObject: bookmark, save: true, sendToSync: true, context: DataController.newBackgroundContext())
        }
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
    }
    
    func testUpdateSyncable() {
        let url = URL(string: "http://brave.com")!
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
        
        // No CD autosave, see the method internals.
        object.update(syncRecord: syncBookmark)
        DataController.viewContext.refreshAllObjects()
        
        XCTAssertEqual(object.title, newTitle)
        XCTAssertEqual(object.url, newUrl)
        
        XCTAssertEqual(object.created, oldCreated)
        XCTAssertNotEqual(object.lastVisited, oldLastVisited)
    }
    
    func testAsDictionary() {
        let url = URL(string: "http://brave.com")!
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
        
        let found = Bookmark.frecencyQuery(context: DataController.viewContext, containing: "brave")
        // Query limit is 5
        XCTAssertEqual(found.count, 5)
        
        // Changing dates of two bookmarks to be something older than 1 week.
        // Because we added 6 bookmarks and query limit is 5, the frequency query should return 4 bookmarks.
        found.first?.lastVisited = Date(timeIntervalSince1970: 1)
        found.last?.lastVisited = Date(timeIntervalSince1970: 1)
        DataController.save(context: DataController.viewContext)
        
        let found2 = Bookmark.frecencyQuery(context: DataController.viewContext, containing: "brave")
        XCTAssertEqual(found2.count, 4)
        
        let notFound = Bookmark.frecencyQuery(context: DataController.viewContext, containing: "notfound")
        XCTAssertEqual(notFound.count, 0)
    }
    
    // MARK: - Helpers
    
    /// Wrapper around `Bookmark.create()` with context save wait expectation and fetching object from view context.
    @discardableResult 
    private func createAndWait(url: URL?, title: String?, customTitle: String? = nil, 
                               parentFolder: Bookmark? = nil, isFolder: Bool = false, isFavorite: Bool = false, color: UIColor? = nil, syncOrder: String? = nil) -> Bookmark {
        
        backgroundSaveAndWaitForExpectation {
            Bookmark.add(url: url, title: title, customTitle: customTitle, parentFolder: parentFolder,
                         isFolder: isFolder, isFavorite: isFavorite, syncOrder: syncOrder)
        }
        
        return try! DataController.viewContext.fetch(fetchRequest).first!
    }
    
    private func insertBookmarks(amount: Int, parent: Bookmark? = nil) {
        let bookmarksBeforeInsert = try! DataController.viewContext.count(for: fetchRequest)
        
        let url = "http://brave.com/"
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
        let destinationOrderBefore = (frc.object(at: destinationIndexPath)).order
        
        // CD objects we saved before will get updated after this call.
        Bookmark.reorderBookmarks(frc: frc, sourceIndexPath: sourceIndexPath, destinationIndexPath: destinationIndexPath)
        
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
        XCTAssertEqual(record.created, record.lastVisited)
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
