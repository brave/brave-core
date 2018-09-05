// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
@testable import Data

class DomainTests: CoreDataTestCase {
    let fetchRequest = NSFetchRequest<Domain>(entityName: String(describing: Domain.self))
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: String(describing: Domain.self), in: context)!
    }
    
    func testGetOrCreate() {
        let url = URL(string: "http://example.com")!
        let url2 = URL(string: "http://brave.com")!
        let context = DataController.viewContext
        
        XCTAssertNotNil(Domain.getOrCreateForUrl(url, context: context))
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        // Try to add the same domain again, verify no new object is created
        XCTAssertNotNil(Domain.getOrCreateForUrl(url, context: context))
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
        
        // Add another domain, verify that second object is created
        XCTAssertNotNil(Domain.getOrCreateForUrl(url2, context: context))
        XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 2)
    }

    // BRAVE TODO: Add shields unit tests after they are finished.
}
