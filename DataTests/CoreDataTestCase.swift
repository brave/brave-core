// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
@testable import Data

class CoreDataTestCase: XCTestCase {
    
    override func setUp() {
        super.setUp()
        
        NotificationCenter.default.addObserver(self, selector: #selector(contextSaved),
                                               name: NSNotification.Name.NSManagedObjectContextDidSave,
                                               object: nil)
        
        DataController.shared = InMemoryDataController()
        DataController.shared.lazyInitialization()
    }
    
    override func tearDown() {
        NotificationCenter.default.removeObserver(self)
        DataController.viewContext.reset()
        Device.sharedCurrentDeviceId = nil
        contextSaveCompletion = nil
        super.tearDown()
    }
    
    // MARK: - Handling background context reads/writes

    var contextSaveCompletion: (() -> Void)?
    
    @objc func contextSaved() {
        contextSaveCompletion?()
    }
    
    /// Waits for core data context save notification. Use this for single background context saves
    /// if you want to wait for view context to update itself. Unfortunately there is no notification
    // after changes are merged into context.
    /// Use `inverted` property if you want to verify that DB save did not happen.
    /// This is useful for early return database checks.
    func backgroundSaveAndWaitForExpectation(name: String? = nil, inverted: Bool = false, code: () -> Void) {
        let saveExpectation: XCTestExpectation? = expectation(description: name ?? UUID().uuidString)
        saveExpectation?.isInverted = inverted
        
        contextSaveCompletion = {
            saveExpectation?.fulfill()
        }
        
        code()
        
        // Long timeouts for inverted expectation increases test duration significantly, reducing it to 1 second.
        let timeout: TimeInterval = inverted ? 1 : 5
        
        if let saveExpectation = saveExpectation {
            wait(for: [saveExpectation], timeout: timeout)
        }
    }
}
