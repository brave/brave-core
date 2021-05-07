// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
import Shared
@testable import Data

class TabMOTests: CoreDataTestCase {
    let fetchRequest = NSFetchRequest<TabMO>(entityName: String(describing: TabMO.self))
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: String(describing: TabMO.self), in: context)!
    }

    func testCreate() {        
        let object = createAndWait()
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        XCTAssertNotNil(object.syncUUID)
        XCTAssertNotNil(object.imageUrl)
        XCTAssertNil(object.url)
        XCTAssertEqual(object.title, Strings.newTab)
        
        // Testing default values
        XCTAssertEqual(object.order, 0)
        XCTAssertEqual(object.urlHistoryCurrentIndex, 0)
        
        XCTAssertFalse(object.isSelected)
        
        XCTAssertNil(object.color)
        XCTAssertNil(object.screenshot)
        XCTAssertNil(object.screenshotUUID)
        XCTAssertNil(object.url)
        XCTAssertNil(object.urlHistorySnapshot)
        
    }
    
    func testUpdate() {
        let newTitle = "UpdatedTitle"
        let newUrl = "http://example.com"
        
        var object = createAndWait()
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        XCTAssertNotEqual(object.title, newTitle)
        XCTAssertNotEqual(object.url, newUrl)
        
        let tabData = SavedTab(id: object.syncUUID!, title: newTitle, url: newUrl, isSelected: true, order: 10, 
                               screenshot: UIImage.sampleImage(), history: ["history1", "history2"], historyIndex: 20)
        
        backgroundSaveAndWaitForExpectation {
            TabMO.update(tabData: tabData)
        }
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        // Need to refresh context here.
         DataController.viewContext.reset()
        
        object = try! DataController.viewContext.fetch(fetchRequest).first!
        
        XCTAssertNotNil(object.syncUUID)
        XCTAssertNotNil(object.imageUrl)
        XCTAssertNotNil(object.screenshot)
        
        XCTAssertEqual(object.url, newUrl)
        XCTAssertEqual(object.title, newTitle)
        XCTAssertEqual(object.order, 10)
        XCTAssertEqual(object.urlHistoryCurrentIndex, 20)
        XCTAssertEqual(object.urlHistorySnapshot?.count, 2)
        
        XCTAssert(object.isSelected)
        
        XCTAssertNil(object.color)
        XCTAssertNil(object.screenshotUUID)
    }
    
    func testUpdateWrongId() {
        let newTitle = "UpdatedTitle"
        let newUrl = "http://example.com"
        let wrongId = "999"
        
        var object = createAndWait()
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        XCTAssertNotEqual(object.title, newTitle)
        XCTAssertNotEqual(object.url, newUrl)
        
        let tabData = SavedTab(id: wrongId, title: newTitle, url: newUrl, isSelected: true, order: 10, 
                               screenshot: UIImage.sampleImage(), history: ["history1", "history2"], historyIndex: 20)
        
        TabMO.update(tabData: tabData)
        // We can't wait for context save here, wrong id is being passed, let's fake it by waiting one second
        sleep(UInt32(1))
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        // Need to refresh context here.
        DataController.viewContext.reset()
        object = try! DataController.viewContext.fetch(fetchRequest).first!
        
        // Nothing should change
        XCTAssertNotEqual(object.title, newTitle)
        XCTAssertNotEqual(object.url, newUrl)
    }
    
    func testDelete() {
        let object = createAndWait()
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        backgroundSaveAndWaitForExpectation {
            object.delete()
        }
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 0)
    }
    
    func testImageUrl() {
        let object = createAndWait()
        
        XCTAssertEqual(object.imageUrl, URL(string: "https://imagecache.mo/\(object.syncUUID!).png"))
    }
    
    func testSaveScreenshotUUID() {
        let newUUID = UUID()
        var object = createAndWait()
        
        XCTAssertNil(object.screenshotUUID)
        backgroundSaveAndWaitForExpectation {
            TabMO.saveScreenshotUUID(newUUID, tabId: object.syncUUID)
        }
        DataController.viewContext.reset()
        
        object = try! DataController.viewContext.fetch(fetchRequest).first!
        XCTAssertNotNil(object.screenshotUUID)
    }
    
    func testSaveScreenshotUUIDWrongId() {
        let wrongId = "999"
        let newUUID = UUID()
        var object = createAndWait()
        
        XCTAssertNil(object.screenshotUUID)
        TabMO.saveScreenshotUUID(newUUID, tabId: wrongId)
        DataController.viewContext.reset()
        
        object = try! DataController.viewContext.fetch(fetchRequest).first!
        XCTAssertNil(object.screenshotUUID)
    }
    
    private func createAndUpdate(order: Int) {
        let object = createAndWait()
        
        let tabData = SavedTab(id: object.syncUUID!, title: "title\(order)", url: "url\(order)", isSelected: false, order: Int16(order),
                               screenshot: nil, history: [], historyIndex: 0)
        backgroundSaveAndWaitForExpectation {
            TabMO.update(tabData: tabData)
        }
        
    }
    
    func testGetAll() {
        createAndUpdate(order: 1)
        createAndUpdate(order: 3)
        createAndUpdate(order: 2)
        
        DataController.viewContext.refreshAllObjects()
        
        // Getting all objects and sorting them manually by order
        let objectsSortedByOrder = try! DataController.viewContext.fetch(fetchRequest).sorted(by: { $0.order < $1.order })
        
        // Verify objects were updated with correct order.
        XCTAssertEqual(objectsSortedByOrder[0].order, 1)
        XCTAssertEqual(objectsSortedByOrder[1].order, 2)
        XCTAssertEqual(objectsSortedByOrder[2].order, 3)
        
        let all = TabMO.getAll()
        XCTAssertEqual(all.count, 3)
        
        // getAll() also should return objects sorted by order
        XCTAssertEqual(all[0].syncUUID, objectsSortedByOrder[0].syncUUID)
        XCTAssertEqual(all[1].syncUUID, objectsSortedByOrder[1].syncUUID)
        XCTAssertEqual(all[2].syncUUID, objectsSortedByOrder[2].syncUUID)
    }
    
    func testGetAllNoOlderThan() {
        let staleTab1 = createAndWait(lastUpdateDate: dateFrom(string: "2021-01-01"))
        let staleTab2 = createAndWait(lastUpdateDate: dateFrom(string: "2021-01-10"))
        
        let now = Date()
        
        // Now
        let freshTab1 = createAndWait()
        // Past 3 days
        let freshTab2 = createAndWait(lastUpdateDate: now.advanced(by: -(3 * 60 * 60 * 24)))
        // 3 days in the future
        let freshTab3 = createAndWait(lastUpdateDate: now.advanced(by: 3 * 60 * 60 * 24))
        
        let all = TabMO.getAll()
        XCTAssertEqual(all.count, 5)
        
        let freshOnly = TabMO.all(noOlderThan: 7 * 60 * 60 * 24)
        XCTAssertEqual(freshOnly.count, 3)
        
        XCTAssert(freshOnly.contains(freshTab1))
        XCTAssert(freshOnly.contains(freshTab2))
        XCTAssert(freshOnly.contains(freshTab3))
        XCTAssertFalse(freshOnly.contains(staleTab1))
        XCTAssertFalse(freshOnly.contains(staleTab2))
    }
    
    func testDeleteOlderThan() {
        let staleTab1 = createAndWait(lastUpdateDate: dateFrom(string: "2021-01-01"))
        let staleTab2 = createAndWait(lastUpdateDate: dateFrom(string: "2021-01-10"))
        
        let now = Date()
        
        // Now
        let freshTab1 = createAndWait()
        // Past 3 days
        let freshTab2 = createAndWait(lastUpdateDate: now.advanced(by: -(3 * 60 * 60 * 24)))
        // 3 days in the future
        let freshTab3 = createAndWait(lastUpdateDate: now.advanced(by: 3 * 60 * 60 * 24))
        
        let all = TabMO.getAll()
        XCTAssertEqual(all.count, 5)
        
        backgroundSaveAndWaitForExpectation {
            TabMO.deleteAll(olderThan: 7 * 60 * 60 * 24)
        }
        
        let allAfterDelete = TabMO.getAll()
        XCTAssertEqual(allAfterDelete.count, 3)
        
        XCTAssert(allAfterDelete.contains(freshTab1))
        XCTAssert(allAfterDelete.contains(freshTab2))
        XCTAssert(allAfterDelete.contains(freshTab3))
        XCTAssertFalse(allAfterDelete.contains(staleTab1))
        XCTAssertFalse(allAfterDelete.contains(staleTab2))
    }
    
    func testGetFromId() {
        let wrongId = "999"
        let object = createAndWait()
        
        XCTAssertNotNil(TabMO.get(fromId: object.syncUUID!))
        XCTAssertNil(TabMO.get(fromId: wrongId))
    }
    
    @discardableResult private func createAndWait(lastUpdateDate: Date = Date()) -> TabMO {
        let uuid = UUID().uuidString

        backgroundSaveAndWaitForExpectation {
            TabMO.createInternal(uuidString: uuid, lastUpdateDate: lastUpdateDate)
        }
        
        return TabMO.get(fromId: uuid)!
    }
    
    private func dateFrom(string: String) -> Date {
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "yyyy-MM-dd"
        dateFormatter.timeZone = TimeZone(abbreviation: "GMT")!
        
        return dateFormatter.date(from: string)!
    }
}

private extension UIImage {
    class func sampleImage() -> UIImage {
        let color = UIColor.blue
        let rect = CGRect(origin: CGPoint(x: 0, y: 0), size: CGSize(width: 1, height: 1))
        UIGraphicsBeginImageContext(rect.size)
        let context = UIGraphicsGetCurrentContext()!
        
        context.setFillColor(color.cgColor)
        context.fill(rect)
        
        let image = UIGraphicsGetImageFromCurrentImageContext()
        UIGraphicsEndImageContext()
        
        return image!
    }
}
