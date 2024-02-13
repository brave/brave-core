// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
@testable import Data
import Preferences

open class CoreDataTestCase: XCTestCase {

  override open func setUp() {
    super.setUp()
    DataController.shared = InMemoryDataController()
  }

  override open func tearDown() {
    DataController.viewContext.reset()
    super.tearDown()
  }

  /// Waits for core data context save notification. Use this for single background context saves
  /// if you want to wait for view context to update itself. Unfortunately there is no notification
  // after changes are merged into context.
  /// Use `inverted` property if you want to verify that DB save did not happen.
  /// This is useful for early return database checks.
  open func backgroundSaveAndWaitForExpectation(name: String? = nil, inverted: Bool = false, code: () -> Void) {
    let mergeExpectation = expectation(description: "merge")
    let saveExpectation = expectation(
      forNotification: .NSManagedObjectContextDidSave,
      object: nil
    ) { notification in
      DispatchQueue.main.async {
        DataController.viewContext.mergeChanges(fromContextDidSave: notification)
        mergeExpectation.fulfill()
      }
      return true
    }
    saveExpectation.isInverted = inverted

    code()

    // Long timeouts for inverted expectation increases test duration significantly, reducing it to 1 second.
    let timeout: TimeInterval = inverted ? 1 : 5
    wait(for: [saveExpectation, mergeExpectation], timeout: timeout)
  }
}
