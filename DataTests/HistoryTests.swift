// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
@testable import Data

class HistoryTests: CoreDataTestCase {
    let fetchRequest = NSFetchRequest<History>(entityName: String(describing: History.self))
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: String(describing: History.self), in: context)!
    }

    func testAdd() {
        let title = "Brave"
        let url = URL(string: "https://brave.com")!
        
        let object = createAndWait(title: title, url: url)
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 1)
        
        XCTAssertEqual(object.title, title)
        XCTAssertEqual(object.url, url.absoluteString)
        
        XCTAssertNotNil(object.visitedOn)
        XCTAssertNotNil(object.domain)
    }
    
    func testAddTwice() {
        let object = createAndWait()
        
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 1)
        XCTAssertEqual(object.domain!.visits, 1)
        
        let newObject = createAndWait()
        // Should still be one object but with 2 visits recorded.
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 1)
        XCTAssertEqual(newObject.domain!.visits, 2)
    }
    
    func testFrc() {
        let frc = History.frc()
        XCTAssertNotNil(frc.fetchRequest.sortDescriptors)
        XCTAssertNotNil(frc.fetchRequest.predicate)
        
        let firstObject = createAndWait(title: "First", url: URL(string: "http://example.com")!)
        
        // Wait a moment to make a date difference
        sleep(UInt32(2))
        let secondObject = createAndWait(title: "Second", url: URL(string: "https://brave.com")!)
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 2)
        
        XCTAssertNoThrow(try frc.performFetch())
        
        let objects = frc.fetchedObjects as! [History]
        
        XCTAssertEqual(objects.first?.url, secondObject.url)
        XCTAssertEqual(objects.last?.url, firstObject.url)
    }
    
    func testGetExisting() {
        let title = "Brave"
        let url = URL(string: "https://brave.com")!
        let wrongUrl = URL(string: "https://wrong.example.com")!
        
        _ = createAndWait(title: title, url: url)
        
        let context = DataController.workerThreadContext
        
        XCTAssertNil(History.getExisting(wrongUrl, context: context))
        XCTAssertNotNil(History.getExisting(url, context: context))
    }
    
    func testRemove() {
        let object = createAndWait()
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 1)
        
        object.remove(save: true)
        
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 0)
    }
    
    func testDeleteAll() {
        let deleteExpectation = expectation(description: "delete all")
        
        createAndWait()
        createAndWait(title: "title", url: URL(string: "https://brave.com")!)
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 2)
        
        History.deleteAll {
            deleteExpectation.fulfill()
        }
        
        waitForExpectations(timeout: 1, handler: nil)
        
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 0)
    }
    
    func testFrecencyQuery() {
        createAndWait(url: URL(string: "https://example.com/page1")!)
        createAndWait(url: URL(string: "https://example.com/page2")!)
        createAndWait(url: URL(string: "https://example.com/page3")!)
        createAndWait(url: URL(string: "https://brave.com")!)
        
        let found = History.frecencyQuery(DataController.mainThreadContext, containing: "example")
        XCTAssertEqual(found.count, 3)
        
        // Changing dates of two bookmarks to be something older than 1 week.
        found.first?.visitedOn = Date(timeIntervalSince1970: 1)
        found.last?.visitedOn = Date(timeIntervalSince1970: 1)
        DataController.saveContext(context: DataController.mainThreadContext)
        
        let found2 = History.frecencyQuery(DataController.mainThreadContext, containing: "example")
        XCTAssertEqual(found2.count, 1)
        
        let notFound = History.frecencyQuery(DataController.mainThreadContext, containing: "notfound")
        XCTAssertEqual(notFound.count, 0)
    }

    @discardableResult
    private func createAndWait(title: String = "New title", url: URL = URL(string: "https://example.com")!) -> History {
        backgroundSaveAndWaitForExpectation {
            History.add(title, url: url)
        }
        
        return try! DataController.mainThreadContext.fetch(fetchRequest).first!
    }
}
