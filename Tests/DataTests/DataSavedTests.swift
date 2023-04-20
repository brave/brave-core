// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
import TestHelpers
@testable import Data

class DataSavedTests: CoreDataTestCase {

  func testInsertWebsite() {
    let savedUrl = URL(string: "https://brave.com")!
    let amount = "1.25"

    let object = createAndWait(amount: amount, savedUrl: savedUrl)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)

    XCTAssertEqual(object.savedUrl, savedUrl.absoluteString)
    XCTAssertEqual(object.amount, amount)
  }

  func testGetExisting() {
    let unsavedUrl = URL(string: "https://wrong.example.com")!
    let savedUrl = URL(string: "https://brave.com")!

    let amount = "1.25"

    createAndWait(amount: amount, savedUrl: savedUrl)

    XCTAssertNil(DataSaved.get(with: unsavedUrl.absoluteString))
    XCTAssertNotNil(DataSaved.get(with: savedUrl.absoluteString))
  }

  func testRemove() {
    let savedUrl = URL(string: "https://brave.com")!
    let amount = "1.25"

    createAndWait(amount: amount, savedUrl: savedUrl)

    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)

    backgroundSaveAndWaitForExpectation {
      DataSaved.delete(with: savedUrl.absoluteString)
    }

    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 0)
  }

  let fetchRequest = NSFetchRequest<DataSaved>(entityName: String(describing: DataSaved.self))

  @discardableResult
  private func createAndWait(amount: String, savedUrl: URL) -> DataSaved {
    backgroundSaveAndWaitForExpectation {
      DataSaved.insert(
        savedUrl: savedUrl.absoluteString,
        amount: amount)
    }

    let predicate = NSPredicate(format: "\(#keyPath(DataSaved.savedUrl)) == %@", savedUrl.absoluteString)
    return DataSaved.first(where: predicate)!
  }
}
