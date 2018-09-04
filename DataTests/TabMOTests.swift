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
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 1)
        
        XCTAssertNotNil(object.syncUUID)
        XCTAssertNotNil(object.imageUrl)
        XCTAssertNil(object.url)
        XCTAssertEqual(object.title, Strings.New_Tab)
        
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
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 1)
        
        XCTAssertNotEqual(object.title, newTitle)
        XCTAssertNotEqual(object.url, newUrl)
        
        let tabData = SavedTab(id: object.syncUUID!, title: newTitle, url: newUrl, isSelected: true, order: 10, 
                               screenshot: UIImage.sampleImage(), history: ["history1", "history2"], historyIndex: 20)
        
        backgroundSaveAndWaitForExpectation {
            TabMO.preserve(savedTab: tabData)
        }
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 1)
        
        // Need to refresh context here.
         DataController.mainThreadContext.reset()
        
        object = try! DataController.mainThreadContext.fetch(fetchRequest).first!
        
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
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 1)
        
        XCTAssertNotEqual(object.title, newTitle)
        XCTAssertNotEqual(object.url, newUrl)
        
        let tabData = SavedTab(id: wrongId, title: newTitle, url: newUrl, isSelected: true, order: 10, 
                               screenshot: UIImage.sampleImage(), history: ["history1", "history2"], historyIndex: 20)
        
        TabMO.preserve(savedTab: tabData)
        // We can't wait for context save here, wrong id is being passed, let's fake it by waiting one second
        sleep(UInt32(1))
        
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 1)
        
        // Need to refresh context here.
        DataController.mainThreadContext.reset()
        object = try! DataController.mainThreadContext.fetch(fetchRequest).first!
        
        // Nothing should change
        XCTAssertNotEqual(object.title, newTitle)
        XCTAssertNotEqual(object.url, newUrl)
    }
    
    func testDelete() {
        let object = createAndWait()
        
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 1)
        DataController.remove(object: object)
        
        
        // FIXME: This fails most of the times.
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 0)
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
        DataController.mainThreadContext.reset()
        
        object = try! DataController.mainThreadContext.fetch(fetchRequest).first!
        XCTAssertNotNil(object.screenshotUUID)
    }
    
    func testSaveScreenshotUUIDWrongId() {
        let wrongId = "999"
        let newUUID = UUID()
        var object = createAndWait()
        
        XCTAssertNil(object.screenshotUUID)
        TabMO.saveScreenshotUUID(newUUID, tabId: wrongId)
        DataController.mainThreadContext.reset()
        
        object = try! DataController.mainThreadContext.fetch(fetchRequest).first!
        XCTAssertNil(object.screenshotUUID)
    }
    
    private func createAndUpdate(order: Int) {
        let object = createAndWait()
        
        // There are some threading problems on the old CD stack, need to add small delay here
        sleep(UInt32(0.25))
        let tabData = SavedTab(id: object.syncUUID!, title: "title", url: "url", isSelected: false, order: Int16(order), 
                               screenshot: nil, history: [], historyIndex: 0)
        backgroundSaveAndWaitForExpectation {
            TabMO.preserve(savedTab: tabData)
        }
        
    }
    
    func testGetAll() {
        createAndUpdate(order: 1)
        createAndUpdate(order: 3)
        createAndUpdate(order: 2)
        
        // Getting all objects and sorting them manually by order
        let objectsSortedByOrder = try! DataController.mainThreadContext.fetch(fetchRequest).sorted(by: { $0.order < $1.order })
        
        let all = TabMO.getAll()
        XCTAssertEqual(all.count, 3)
        
        // getAll() also should return objects sorted by order
        XCTAssertEqual(all[0].syncUUID, objectsSortedByOrder[0].syncUUID)
        XCTAssertEqual(all[1].syncUUID, objectsSortedByOrder[1].syncUUID)
        XCTAssertEqual(all[2].syncUUID, objectsSortedByOrder[2].syncUUID)
        
        // Need to update order of each of our objects
    }
    
    func testGetFromId() {
        let context = DataController.mainThreadContext
        let wrongId = "999"
        let object = createAndWait()
        
        XCTAssertNotNil(TabMO.get(fromId: object.syncUUID!, context: context))
        XCTAssertNil(TabMO.get(fromId: wrongId, context: context))
    }
    
    @discardableResult private func createAndWait() -> TabMO {
        backgroundSaveAndWaitForExpectation {
            _ = TabMO.create()
        }
        
        return try! DataController.mainThreadContext.fetch(fetchRequest).first!
    }
}

private extension UIImage {
    class func sampleImage() -> UIImage {
        let color = UIColor.blue
        let rect = CGRect(origin: CGPoint(x: 0, y:0), size: CGSize(width: 1, height: 1))
        UIGraphicsBeginImageContext(rect.size)
        let context = UIGraphicsGetCurrentContext()!
        
        context.setFillColor(color.cgColor)
        context.fill(rect)
        
        let image = UIGraphicsGetImageFromCurrentImageContext()
        UIGraphicsEndImageContext()
        
        return image!
    }
}
