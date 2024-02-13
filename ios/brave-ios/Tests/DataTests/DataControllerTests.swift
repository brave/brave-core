// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
import TestHelpers
@testable import Data

class DataControllerTests: CoreDataTestCase {
  let fetchRequest = NSFetchRequest<NSFetchRequestResult>(entityName: String(describing: "Bookmark"))

  private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
    return NSEntityDescription.entity(forEntityName: String(describing: "Bookmark"), in: context)!
  }

  func testStoreIsEmpty() {
    // Checking view and background contexts with TopSite entity
    let viewContext = DataController.viewContext
    XCTAssertEqual(try! viewContext.count(for: fetchRequest), 0)

    // Checking rest of entities
    let favoriteFR = NSFetchRequest<NSFetchRequestResult>(entityName: String(describing: "Bookmark"))
    XCTAssertEqual(try! viewContext.count(for: favoriteFR), 0)

    let domainFR = NSFetchRequest<NSFetchRequestResult>(entityName: String(describing: Domain.self))
    XCTAssertEqual(try! viewContext.count(for: domainFR), 0)
  }

  func testSavingMainContext() {
    let context = DataController.viewContext

    _ = Favorite(entity: entity(for: context), insertInto: context)

    try! context.save()

    let result = try! context.fetch(fetchRequest)
    XCTAssertEqual(result.count, 1)
  }

  func testSavingBackgroundContext() {
    backgroundSaveAndWaitForExpectation {
      DataController.perform { context in
        _ = Favorite(entity: self.entity(for: context), insertInto: context)
      }
    }

    // Check if object got updated on view context(merge from parent check)
    XCTAssertEqual(try! DataController.viewContext.fetch(fetchRequest).count, 1)
  }

  func testSaveAndRemove() {
    backgroundSaveAndWaitForExpectation {
      DataController.perform { context in
        _ = Favorite(entity: self.entity(for: context), insertInto: context)
      }
    }

    let result = try! DataController.viewContext.fetch(fetchRequest)
    XCTAssertEqual(result.count, 1)

    backgroundSaveAndWaitForExpectation {
      (result.first as? Favorite)?.delete()
    }

    let newResult = try! DataController.viewContext.fetch(fetchRequest)

    XCTAssertEqual(newResult.count, 0)
  }
}
