// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
import Shared
@testable import Data

class BraveTodaySourceMOTests: CoreDataTestCase {

    let fetchRequest = NSFetchRequest<BraveTodaySourceMO>(entityName: String(describing: BraveTodaySourceMO.self))
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: String(describing: BraveTodaySourceMO.self), in: context)!
    }
    
    func testSimpleInsert() throws {
        XCTAssertEqual(try XCTUnwrap(BraveTodaySourceMO.all()).count, 0)
        try createAndWait()
        XCTAssertEqual(try XCTUnwrap(BraveTodaySourceMO.all()).count, 1)
    }
    
    func testResetSources() throws {
        try createAndWait(enabled: true, publisherID: "123456")
        try createAndWait(enabled: true, publisherID: "654321")
        try createAndWait(enabled: true, publisherID: "456124")
        XCTAssertEqual(try XCTUnwrap(BraveTodaySourceMO.all()).count, 3)
        backgroundSaveAndWaitForExpectation {
            BraveTodaySourceMO.resetSourceSelection()
        }
        XCTAssertEqual(try XCTUnwrap(BraveTodaySourceMO.all()).count, 0)
    }
    
    func testSetEnabled() throws {
        let source = try createAndWait(enabled: true)
        XCTAssertTrue(try XCTUnwrap(BraveTodaySourceMO.getInternal(fromId: source.publisherID)).enabled)
        backgroundSaveAndWaitForExpectation {
            BraveTodaySourceMO.setEnabled(forId: source.publisherID, enabled: false)
        }
        XCTAssertFalse(try XCTUnwrap(BraveTodaySourceMO.getInternal(fromId: source.publisherID)).enabled)
    }
    
    @discardableResult
    private func createAndWait(enabled: Bool = true,
                               publisherID: String = "BravePub") throws -> BraveTodaySourceMO {
        
        backgroundSaveAndWaitForExpectation {
            BraveTodaySourceMO.insertInternal(publisherID: publisherID, enabled: enabled)
        }
        
        return try XCTUnwrap(BraveTodaySourceMO.getInternal(fromId: publisherID))
    }
}
