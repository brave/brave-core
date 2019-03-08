// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
@testable import Data

class CoreDataTestCase: XCTestCase {
    
    override func setUp() {
        super.setUp()
        
        // HACK: For some reason switching to in-memory database at launch causes problems with tests,
        // because some operations are still pending in the app like creating a Domain for localhost
        // attempting to restore tabs and some shields work.
        // The workaround is to detect if sqlite database is still used(which means it's first launch of the test suite)
        // wait a few seconds for it to finish its initialization, then switch the database to in-memory and run
        // all the tests.
        //
        // In the future we should make Data tests more independent of the app state.
        
        let storeType = DataController.viewContext.persistentStoreCoordinator!.persistentStores.first!.type
        if storeType == NSSQLiteStoreType {
            let waitTimeForAppInitialization: TimeInterval = 5
            
            let expectation = self.expectation(description: "App initialization wait")
            DispatchQueue.main.asyncAfter(deadline: .now() + waitTimeForAppInitialization, execute: { expectation.fulfill() })
            waitForExpectations(timeout: waitTimeForAppInitialization + 1, handler: nil)
        }
        
        NotificationCenter.default.addObserver(self, selector: #selector(contextSaved),
                                               name: NSNotification.Name.NSManagedObjectContextDidSave,
                                               object: nil)
        
        DataController.shared = InMemoryDataController()
    }
    
    override func tearDown() {
        NotificationCenter.default.removeObserver(self)
        DataController.viewContext.reset()
        Device.sharedCurrentDevice = nil
        contextSaveCompletion = nil
        super.tearDown()
    }
    
    // MARK: - Handling background context reads/writes

    var contextSaveCompletion: (() -> Void)?
    
    @objc func contextSaved() {
        contextSaveCompletion?()
    }
    
    /// Waits for core data context save notification. Use this for single background context saves if you want to wait
    /// for view context to update itself. Unfortunately there is no notification after changes are merged into context.
    func backgroundSaveAndWaitForExpectation(name: String? = nil, code: () -> Void) {
        var saveExpectation: XCTestExpectation? = expectation(description: name ?? UUID().uuidString)
        
        contextSaveCompletion = {
            saveExpectation?.fulfill()
            // Removing reference to save expectation in case it's going to be called twice.
            saveExpectation = nil
        }
        
        code()
        
        if let saveExpectation = saveExpectation {
            wait(for: [saveExpectation], timeout: 5)
        }
    }
}
