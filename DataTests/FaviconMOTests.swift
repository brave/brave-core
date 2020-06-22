// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
import Shared
import Storage
@testable import Data

class FaviconMOTests: CoreDataTestCase {
    let exampleUrl = URL(string: "http://example.com")!
    
    // Can't use String(describing:) because the class is named FaviconMO, not Favicon.
    let fetchRequest = NSFetchRequest<FaviconMO>(entityName: "Favicon")
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: "Favicon", in: context)!
    }

    func testAdd() {
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 0)
        
        createAndWait()
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
    }
    
    func testGet() {
        let wrongUrl = "wrong.url"
        
        createAndWait()
        
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        let context = DataController.viewContext
        
        XCTAssertNil(FaviconMO.get(forFaviconUrl: wrongUrl, context: context))
        XCTAssertNotNil(FaviconMO.get(forFaviconUrl: exampleUrl.absoluteString, context: context))
        
    }
    
    @discardableResult
    private func createAndWait() -> FaviconMO? {
        let favicon = Favicon(url: exampleUrl.absoluteString)
        
        backgroundSaveAndWaitForExpectation {
            FaviconMO.add(favicon, forSiteUrl: exampleUrl, persistent: true)
        }
        
        return try! DataController.viewContext.fetch(fetchRequest).first
    }
}
