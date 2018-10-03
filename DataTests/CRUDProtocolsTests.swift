// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
@testable import Data

class CRUDProtocolsTests: CoreDataTestCase {
    typealias CRUDClass = TopSite
    
    // TopSite is the simplest model we have for testing CRUD methods.
    let fetchRequest = NSFetchRequest<NSFetchRequestResult>(entityName: String(describing: CRUDClass.self))
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: String(describing: CRUDClass.self), in: context)!
    }
    
    let initialObjectsCount = 5
    
    override func setUp() {
        super.setUp()
        insertObjects(count: initialObjectsCount)
    }
    
    private func insertObjects(count: Int) {
        let context = DataController.newBackgroundContext()
        
        for i in 1...count {
            let object = CRUDClass(entity: entity(for: context), insertInto: context)
            object.title = "\(i)"
        }
        
        backgroundSaveAndWaitForExpectation {
            DataController.save(context: context)
        }
        
        // Make sure each test case has the same amount of records to work with.
        XCTAssertEqual(try! context.count(for: fetchRequest), count)
    }
    
    private var notMatchingPredicate: NSPredicate {
        return NSPredicate(format: "title = %@", "999")
    }
    
    // MARK: - Readable
    
    func testGetAll() {
        XCTAssertEqual(CRUDClass.all()?.count, initialObjectsCount)
    }
    
    func testGetAllWithPredicate() {
        let predicate = NSPredicate(format: "title = %@ OR title = %@", "1", "2")
        XCTAssertEqual(CRUDClass.all(where: predicate)?.count, 2)
        
        // Not matching predicate
        let notMatching = CRUDClass.all(where: notMatchingPredicate)
        XCTAssertNotNil(notMatching)
        XCTAssert(notMatching!.isEmpty)
    }
    
    func testGetAllWithSortDescriptor() {
        let titleSort = NSSortDescriptor(key: #keyPath(CRUDClass.title), ascending: false)
        let result = CRUDClass.all(sortDescriptors: [titleSort])?.first
        XCTAssertNotNil(result)
        XCTAssertEqual(result?.title, "5")
    }
    
    func testGetAllWithFetchLimit() {
        let limit = 3
        XCTAssertEqual(CRUDClass.all(fetchLimit: limit)?.count, 3)
    }
    
    private func firstObject(title: String) -> CRUDClass? {
        let predicate = NSPredicate(format: "\(#keyPath(CRUDClass.title)) = %@", title)
        return CRUDClass.first(where: predicate)
    }
    
    func testGetFirst() {
        let title = "1"
        let first = firstObject(title: title)
        
        XCTAssertNotNil(first)
        XCTAssertEqual(first!.title, title)
    }
    
    func testGetFirstNotMatchingPredicate() {
        let first = CRUDClass.first(where: notMatchingPredicate)
        
        XCTAssertNil(first)
    }
    
    func testCount() {
        XCTAssertEqual(CRUDClass.count(), initialObjectsCount)
    }
    
    // MARK: - Deletable
    
    func testDelete() {
        let title = "1"
        let first = firstObject(title: title)
        
        backgroundSaveAndWaitForExpectation {
            first?.delete()
        }
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), initialObjectsCount - 1)
        
        // Verify the correct object got deleted.
        let firstAfterDelete = firstObject(title: title)
        XCTAssertNil(firstAfterDelete)
    }
    
    func testDeleteAll() {
        backgroundSaveAndWaitForExpectation {
            CRUDClass.deleteAll()
        }
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 0)
    }
    
    func testDeleteAllWithPredicate() {
        let predicate = NSPredicate(format: "title = %@ OR title = %@", "1", "2")
        backgroundSaveAndWaitForExpectation {
            CRUDClass.deleteAll(predicate: predicate)
        }
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), initialObjectsCount - 2)
    }
    
    func testDeleteAllNotMatchingPredicate() {
        CRUDClass.deleteAll(predicate: notMatchingPredicate)
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), initialObjectsCount)
    }
}
