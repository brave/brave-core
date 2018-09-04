// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
@testable import Data

class DataControllerTests: CoreDataTestCase {
    // TopSite is the simplest model we have for testing DataController internals.
    let fetchRequest = NSFetchRequest<NSFetchRequestResult>(entityName: String(describing: TopSite.self))
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: String(describing: TopSite.self), in: context)!
    }
    
    func testStoreIsEmpty() {
        // Checking view and background contexts with TopSite entity
        let viewContext = DataController.mainThreadContext
        XCTAssertEqual(try! viewContext.count(for: fetchRequest), 0)
        
        let backgroundContext = DataController.workerThreadContext
        XCTAssertEqual(try! backgroundContext.count(for: fetchRequest), 0)
        
        // Checking rest of entities
        let bookmarkFR = NSFetchRequest<NSFetchRequestResult>(entityName: String(describing: Bookmark.self))
        XCTAssertEqual(try! viewContext.count(for: bookmarkFR), 0)
        
        let tabFR = NSFetchRequest<NSFetchRequestResult>(entityName: String(describing: TabMO.self))
        XCTAssertEqual(try! viewContext.count(for: tabFR), 0)
        
        // FaviconMO class name is different from its model(probably due to firefox having favicon class already)
        // Need to use hardcoded string here.
        let faviconFR = NSFetchRequest<NSFetchRequestResult>(entityName: "Favicon")
        XCTAssertEqual(try! viewContext.count(for: faviconFR), 0)
        
        let domainFR = NSFetchRequest<NSFetchRequestResult>(entityName: String(describing: Domain.self))
        XCTAssertEqual(try! viewContext.count(for: domainFR), 0)
        
        let deviceFR = NSFetchRequest<NSFetchRequestResult>(entityName: String(describing: Device.self))
        XCTAssertEqual(try! viewContext.count(for: deviceFR), 0)
        
        let historyFR = NSFetchRequest<NSFetchRequestResult>(entityName: String(describing: History.self))
        XCTAssertEqual(try! viewContext.count(for: historyFR), 0)
    }
    
    func testSavingMainContext() {
        let context = DataController.mainThreadContext
        
        _ = TopSite(entity: entity(for: context), insertInto: context)
        DataController.saveContext(context: context)
        
        let result = try! context.fetch(fetchRequest)
        XCTAssertEqual(result.count, 1)
    }
    
    func testSavingBackgroundContext() {
        let context = DataController.workerThreadContext
        
        _ = TopSite(entity: entity(for: context), insertInto: context)
        backgroundSaveAndWaitForExpectation {
            DataController.saveContext(context: context)
        }
        
        let result = try! context.fetch(fetchRequest)
        
        XCTAssertEqual(result.count, 1)
        
        // Check if object got updated on view context(merge from parent check)
        XCTAssertEqual(try! DataController.mainThreadContext.fetch(fetchRequest).count, 1)
    }
    
    func testSaveAndRemove() {
        let context = DataController.workerThreadContext
        
        _ = TopSite(entity: entity(for: context), insertInto: context)
        backgroundSaveAndWaitForExpectation {
            DataController.saveContext(context: context)
        }
        
        let result = try! DataController.mainThreadContext.fetch(fetchRequest)
        XCTAssertEqual(result.count, 1)
        
        DataController.remove(object: result.first as! TopSite)
        
        DataController.mainThreadContext.refreshAllObjects()
        let newResult = try! DataController.mainThreadContext.fetch(fetchRequest)
        
        XCTAssertEqual(newResult.count, 0)
    }
    
    func testNilContext() {
        DataController.saveContext(context: nil)
        XCTAssertEqual(try! DataController.mainThreadContext.count(for: fetchRequest), 0)
    }
    
    func testNoChangesContext() {
        let context = DataController.workerThreadContext
        DataController.saveContext(context: context)
        XCTAssertEqual(try! context.count(for: fetchRequest), 0)
    }

}
